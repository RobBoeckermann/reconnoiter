/*
 * Copyright (c) 2007, OmniTI Computer Consulting, Inc.
 * All rights reserved.
 * Copyright (c) 2015-2017, Circonus, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name OmniTI Computer Consulting, Inc. nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <mtev_defines.h>

#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <eventer/eventer.h>
#include <mtev_conf.h>
#include <mtev_hash.h>
#include <mtev_log.h>
#include <mtev_getip.h>
#include <mtev_rest.h>
#include <mtev_json.h>
#include <mtev_stats.h>
#include <mtev_consul.h>
#include <mtev_curl.h>

#include "noit_mtev_bridge.h"
#include "stratcon_dtrace_probes.h"
#include "noit_jlog_listener.h"
#include "stratcon_datastore.h"
#include "stratcon_jlog_streamer.h"
#include "stratcon_iep.h"

static mtev_log_stream_t jlog_streamer_err = NULL;
static mtev_log_stream_t jlog_streamer_deb = NULL;
pthread_mutex_t noits_lock;
mtev_hash_table noits;
pthread_mutex_t noit_ip_by_cn_lock;
mtev_hash_table noit_ip_by_cn;
struct noit_meta {
  char *cn;
  unsigned short port;
  char target[INET6_ADDRSTRLEN];
  int storage_svc_idx;
  int transient_svc_idx;
  mtev_hash_table *tags;
  mtev_hash_table *meta;
};

static void consul_start(struct noit_meta *tgt, bool launch);

struct noit_meta *noit_meta_new(const char *cn, const char *target, unsigned short port) {
  struct noit_meta *n = calloc(1, sizeof(*n));
  n->cn = strdup(cn);
  n->port = port;
  if(target) strlcpy(n->target, target, sizeof(n->target));
  n->tags = calloc(1, sizeof(*n->tags));
  mtev_hash_init(n->tags);
  mtev_hash_dict_store(n->tags, "cn", cn);
  n->meta = calloc(1, sizeof(*n->meta));
  mtev_hash_init(n->meta);
  return n;
}
static void noit_meta_free(void *vnm) {
  struct noit_meta *n = (struct noit_meta *)vnm;
  mtev_hash_destroy(n->tags, free, free);
  free(n->tags);
  mtev_hash_destroy(n->meta, free, free);
  free(n->meta);
  free(n->cn);
  free(n);
}
static const char *extract_key(const mtev_json_object *o, const char *path) {
  char *path_copy = strdup(path);
  char *brk = NULL;
  for(const char *cp = strtok_r(path_copy, ".", &brk);
      cp; cp = strtok_r(NULL, ".", &brk)) {
    if(!mtev_json_object_is_type(o, json_type_object)) return NULL;
    o = mtev_json_object_object_get(o, cp);
  }
  return mtev_json_object_get_string(o);
}
static bool dict_is_different(mtev_hash_table *a, mtev_hash_table *b) {
  if(mtev_hash_size(a) != mtev_hash_size(b)) return true;
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  while(mtev_hash_adv(a, &iter)) {
    const char *bval = mtev_hash_dict_get(b, iter.key.str);
    if(!bval) return true;
    if(strcmp(iter.value.str, bval)) return true;
  }
  return false;
}
static void refresh_meta_from_json(const char *cn, mtev_json_object *o) {
  mtev_hash_table tags, meta;
  if(!mtev_json_object_is_type(o, json_type_object)) return;
  mtev_hash_init(&tags);
  mtev_hash_init(&meta);
#define META(key, path) do { \
  const char *_val; \
  if(NULL != (_val = extract_key(o, path))) { \
    mtev_hash_dict_store(&meta, key, _val); \
  } \
} while(0)
  META("noit_version", "version");
  META("mtev_version", "mtev_version");
  META("kernel_type", "unameRun.sysname");
  META("kernel_arch", "unameRun.machine");
  META("kernel_release", "unameRun.release");

  mtev_hash_dict_store(&tags, "cn", cn);
  mtev_json_object *modules = mtev_json_object_object_get(o, "modules");
  if(modules && mtev_json_object_is_type(modules, json_type_object)) {
    mtev_json_object_object_foreach(modules, key, val) {
      const char *type = extract_key(val, "type");
      if(!strcmp(type, "module")) {
        char tagname[128];
        snprintf(tagname, sizeof(tagname), "check:%s", key);
        mtev_hash_dict_store(&tags, tagname, "");
      }
    }
  }

  pthread_mutex_lock(&noit_ip_by_cn_lock);
  bool changed = false;
  void *vnm = NULL;
  if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn), &vnm)) {
    struct noit_meta *nm = vnm;
    if(dict_is_different(nm->tags, &tags)) {
      mtev_hash_delete_all(nm->tags, free, free);
      mtev_hash_merge_as_dict(nm->tags, &tags);
      changed = true;
    }
    if(dict_is_different(nm->meta, &meta)) {
      mtev_hash_delete_all(nm->meta, free, free);
      mtev_hash_merge_as_dict(nm->meta, &meta);
      changed = true;
    }
    if(changed) consul_start(nm, false);
  }
  pthread_mutex_unlock(&noit_ip_by_cn_lock);
  mtev_hash_destroy(&tags, free, free);
  mtev_hash_destroy(&meta, free, free);
}
static uuid_t self_stratcon_id;
static char self_stratcon_hostname[256] = "\0";
static struct sockaddr_in self_stratcon_ip;
static mtev_boolean stratcon_selfcheck_extended_id = mtev_true;

static struct timeval DEFAULT_NOIT_PERIOD_TV = { 5UL, 0UL };

void stratcon_foreach_noit(void (*f)(void *, const char *, const char *), void *c) {
  pthread_mutex_lock(&noits_lock);
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  while(mtev_hash_adv(&noits, &iter)) {
    mtev_connection_ctx_t *ctx = iter.value.ptr;
    const char *cn = mtev_hash_dict_get(ctx->config, "cn");
    f(c, cn, ctx->remote_str);
  }
  pthread_mutex_unlock(&noits_lock);
}

static stats_ns_t *iep_ns, *durable_ns;
struct jlog_stream_stats {
  stats_handle_t *total_events;
  stats_handle_t *total_bytes_read;
  stats_handle_t *last_event_age;
  stats_handle_t *connection_age;
  stats_handle_t *jlog_id;
  stats_handle_t *batch_size;
  stats_handle_t *batch_read_latency;
  stats_handle_t *batch_commit_latency;
};
mtev_hash_table noit_stats_iep; /* this is persistent */
mtev_hash_table noit_stats_durable; /* this is persistent */
mtev_hash_table noit_consul_service;

long WARNING_DELAY_MS = 30 * 1000;
long WARNING_SESSION_TIME_MS = 10 * 1000;

static void consul_meta_updater(void) {
  char *cn = eventer_aco_arg();
  char *ca_chain = NULL, *cert = NULL, *key = NULL;
  mtevL(mtev_error, "CONSUL_META_UPDATER[%s] starting\n", cn);
  eventer_aco_sleep(&(struct timeval){ 1UL, 0UL });
  while(1) {
    if(!ca_chain) {
      pthread_mutex_lock(&noits_lock);
      mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
      /* snag a copy of the sslconfig elements to use with curl */
      while(ca_chain == NULL && mtev_hash_adv(&noits, &iter)) {
        mtev_connection_ctx_t *conn = iter.value.ptr;
        if(conn && conn->sslconfig) {
          ca_chain = mtev_hash_dict_get(conn->sslconfig, "ca_chain");
          cert = mtev_hash_dict_get(conn->sslconfig, "certificate");
          key = mtev_hash_dict_get(conn->sslconfig, "key");
        }
      }
      pthread_mutex_unlock(&noits_lock);
      if(ca_chain) ca_chain = strdup(ca_chain);
      free(cert);
      if(cert) cert = strdup(cert);
      free(key);
      if(key) key = strdup(key);
    }
    char url[512];
    mtev_curl_handle_t *ch = mtev_curl_easy_aco(false);
    CURL *curl = mtev_curl_handle_get_easy_handle(ch);
    struct curl_slist *connect_to = NULL;

    pthread_mutex_lock(&noit_ip_by_cn_lock);
    void *vtgt = NULL;
    struct noit_meta *live = NULL;
    if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn), &vtgt)) {
      char connect_to_str[256];
      live = vtgt;
      snprintf(url, sizeof(url), "https://%s:%u/capa.json", live->cn, live->port);
      snprintf(connect_to_str, sizeof(connect_to_str), "%s:%u:%s:%u", live->cn, live->port, live->target, live->port);
      connect_to = curl_slist_append(connect_to, connect_to_str);
    }
    pthread_mutex_unlock(&noit_ip_by_cn_lock);

    if(live) {
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_CONNECT_TO, connect_to);
      if(ca_chain) curl_easy_setopt(curl, CURLOPT_CAINFO, ca_chain);
      if(cert && key) {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
        curl_easy_setopt(curl, CURLOPT_SSLKEY, key);
      }
      mtev_curl_perform_aco(ch);

      size_t buffer_len = 0;
      const char *buffer = mtev_curl_handle_get_buffer(ch, &buffer_len);
      if(buffer_len) {
        mtev_json_object *obj = mtev_json_tokener_parse_len(buffer, buffer_len, NULL);
        if(obj) {
          refresh_meta_from_json(cn, obj);
          MJ_DROP(obj);
        }
      }
    }

    curl_slist_free_all(connect_to);
    mtev_curl_handle_free_aco(ch);

    if(!live) break;
    eventer_aco_sleep(&(struct timeval){ 5UL, 0UL });
  }
  mtevL(mtev_error, "CONSUL_META_UPDATER[%s] ending\n", cn);
  free(ca_chain);
  free(cn);
}

/* consul actuation */
static void consul_update(const char *cn, bool is_storage, bool disconnected,
                          int64_t last_event_ms, int64_t session_duiration_ms) {
  if(mtev_consul_service_alloc_available()) {
    service_register *sr = mtev_consul_service_registry(cn);
    if(!sr) return;
    void *vtgt = NULL;
    pthread_mutex_lock(&noit_ip_by_cn_lock);
    if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn),
                          &vtgt)) {
      struct noit_meta *tgt = vtgt;
      int svc_idx = is_storage ? tgt->storage_svc_idx : tgt->transient_svc_idx;
      if(svc_idx >= 0) {
        char msg[256];
        if(disconnected) {
          mtev_consul_set_critical(sr, svc_idx, "disconnected");
        }
        else if(last_event_ms > WARNING_DELAY_MS) {
          snprintf(msg, sizeof(msg), "data delayed: delayed %fs, up %fs",
                  (double)last_event_ms/1000.0, (double)session_duiration_ms/1000.0);
          mtev_consul_set_warning(sr, svc_idx, msg);
        }
        else if(session_duiration_ms < WARNING_SESSION_TIME_MS) {
          snprintf(msg, sizeof(msg), "session unstable: delayed %fs, up %fs",
                  (double)last_event_ms/1000.0, (double)session_duiration_ms/1000.0);
          mtev_consul_set_warning(sr, svc_idx, msg);
        }
        else {
          snprintf(msg, sizeof(msg), "feed current: delayed %fs, up %fs",
                  (double)last_event_ms/1000.0, (double)session_duiration_ms/1000.0);
          mtev_consul_set_passing(sr, svc_idx, msg);
        }
      }
    }
    pthread_mutex_unlock(&noit_ip_by_cn_lock);
  }
}
static void consul_start(struct noit_meta *tgt, bool launch) {
  if(mtev_consul_service_alloc_available()) {
    if(strlen(tgt->target)) { /* we only start if we have an address */
      mtev_consul_service *service =
        mtev_consul_service_alloc("noit", tgt->cn, tgt->target, tgt->port,
            tgt->tags, false, tgt->meta, false);
      mtev_consul_service_check_https(service, "CAPA", "/capa.json", "GET", tgt->cn, true, 10, NULL, 60*30);
      if(stratcon_datastore_get_enabled()) {
        tgt->storage_svc_idx = mtev_consul_service_check_push(service, "Storage Feed", 10, 0);
      }
      if(stratcon_iep_get_enabled()) {
        tgt->transient_svc_idx = mtev_consul_service_check_push(service, "Alerting Feed", 10, 0);
      }
      mtev_consul_register(service);
      service_register *sr = mtev_consul_service_registry(tgt->cn);
      if(sr) {
        if(tgt->storage_svc_idx >= 0) mtev_consul_set_critical(sr, tgt->storage_svc_idx, "unknown");
        if(tgt->transient_svc_idx >= 0) mtev_consul_set_critical(sr, tgt->transient_svc_idx, "unknown");
      }
      if(launch) eventer_aco_start(consul_meta_updater, strdup(tgt->cn));
    }
  }
}
static void consul_reset(const char *cn) {
  if(mtev_consul_service_registry_available()) {
    service_register *sr = mtev_consul_service_registry(cn);
    if(sr) {
      struct noit_meta *meta = NULL;
      void *vtgt = NULL;
      pthread_mutex_lock(&noit_ip_by_cn_lock);
      if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn),
                            &vtgt)) {
        meta = (struct noit_meta *)vtgt;
        strlcpy(meta->target, "unknown", sizeof(meta->target));
        meta->port = 0;
        consul_start(meta, false);
      } else {
        meta = noit_meta_new(cn, "unknown", 0);
        mtev_hash_store(&noit_ip_by_cn, meta->cn, strlen(meta->cn), meta);
        consul_start(meta, true);
      }
      pthread_mutex_unlock(&noit_ip_by_cn_lock);
    } else {
      mtevL(mtev_error, "No consul service instance %s\n", cn);
    }
  }
}
static void consul_stop(void *cn) {
  if(mtev_consul_service_registry_available()) {
    service_register *sr = mtev_consul_service_registry(cn);
    if(sr) {
      mtev_consul_service_register_deregister(sr);
    }
  }
}
static mtev_hook_return_t
consul_proxy_hook(void *closure, const char *id, int family, struct sockaddr *addr, bool up) {
  if(!strncmp(id, "noit/", 5)) {
    int len;
    struct noit_meta tgt;
    const char *cn = id + 5;
    void *vtgt = NULL;
    switch(family) {
      case AF_INET:
        len = sizeof(struct sockaddr_in);
        struct in_addr addr_any = { .s_addr = INADDR_ANY };
        if(0 == memcmp(&addr_any, &((struct sockaddr_in *)addr)->sin_addr, sizeof(addr_any))) {
          inet_ntop(family, &self_stratcon_ip.sin_addr, tgt.target, len);
        }
        else {
          inet_ntop(family, &((struct sockaddr_in *)addr)->sin_addr, tgt.target, len);
        }
        tgt.port = htons(((struct sockaddr_in *)addr)->sin_port);
        break;
      case AF_INET6:
        len = sizeof(struct sockaddr_in6);
        inet_ntop(family, &((struct sockaddr_in6 *)addr)->sin6_addr, tgt.target, len);
        tgt.port = htons(((struct sockaddr_in6 *)addr)->sin6_port);
        cn = NULL;
        break;
      default:
        cn = NULL;
        break;
    }
    if(cn) {
      struct noit_meta *meta;
      bool launch = false;
      pthread_mutex_lock(&noit_ip_by_cn_lock);
      if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn), &vtgt)) {
        meta = (struct noit_meta *)vtgt;
        memcpy(meta->target, tgt.target, sizeof(meta->target));
        meta->port = tgt.port;
      } else {
        meta = noit_meta_new(cn, tgt.target, tgt.port);
        mtev_hash_store(&noit_ip_by_cn, meta->cn, strlen(meta->cn), meta);
        launch = true;
      }
      if(up) {
        consul_start(meta, launch);
        pthread_mutex_unlock(&noit_ip_by_cn_lock);
      } else {
        pthread_mutex_unlock(&noit_ip_by_cn_lock);
        consul_reset(cn);
      }
    }
  }
  return MTEV_HOOK_CONTINUE;
}

static jlog_streamer_stats_t *
stats_alloc(stats_ns_t *parent, const char *cn) {
  jlog_streamer_stats_t *h = calloc(1, sizeof(*h));
  stats_ns_t *ns = mtev_stats_ns(parent, cn);
  stats_ns_add_tag(ns, "broker-cn", cn);
  h->jlog_id = stats_register(ns, "jlog_id", STATS_TYPE_UINT32);
  h->total_events = stats_register(ns, "events", STATS_TYPE_UINT64);
  stats_handle_units(h->total_events, STATS_UNITS_MESSAGES);
  h->total_bytes_read = stats_register(ns, "inoctets", STATS_TYPE_UINT64);
  stats_handle_units(h->total_bytes_read, STATS_UNITS_BYTES);
  h->last_event_age = stats_register(ns, "delay", STATS_TYPE_DOUBLE);
  stats_handle_units(h->last_event_age, STATS_UNITS_SECONDS);
  h->connection_age = stats_register(ns, "uptime", STATS_TYPE_DOUBLE);
  stats_handle_units(h->connection_age, STATS_UNITS_SECONDS);
  h->batch_size = stats_register(ns, "batch_size", STATS_TYPE_HISTOGRAM);
  stats_handle_units(h->batch_size, STATS_UNITS_MESSAGES);
  h->batch_read_latency = stats_register(ns, "batch_read_latency", STATS_TYPE_HISTOGRAM);
  stats_handle_tagged_name(h->batch_read_latency, "batch_latency");
  stats_handle_add_tag(h->batch_read_latency, "feed-stage", "read");
  stats_handle_units(h->batch_read_latency, STATS_UNITS_SECONDS);
  h->batch_commit_latency = stats_register(ns, "batch_commit_latency", STATS_TYPE_HISTOGRAM);
  stats_handle_tagged_name(h->batch_commit_latency, "batch_latency");
  stats_handle_add_tag(h->batch_commit_latency, "feed-stage", "commit");
  stats_handle_units(h->batch_commit_latency, STATS_UNITS_SECONDS);
  return h;
}

static jlog_streamer_stats_t *
fetch_stats_for_feed(const char *cn, bool durable) {
  jlog_streamer_stats_t *h;
  if(cn == NULL) return NULL;
  mtev_hash_table *tbl = durable ? &noit_stats_durable : &noit_stats_iep;
  pthread_mutex_lock(&noit_ip_by_cn_lock);
  void *vs;
  if(mtev_hash_retrieve(tbl, cn, strlen(cn), &vs)) {
    pthread_mutex_unlock(&noit_ip_by_cn_lock);
    return (jlog_streamer_stats_t *)vs;
  }

  h = stats_alloc(durable ? durable_ns : iep_ns, cn);
  mtev_hash_store(tbl, strdup(cn), strlen(cn), h);
  pthread_mutex_unlock(&noit_ip_by_cn_lock);
  return h;
}

static void stats_clear(jlog_streamer_stats_t *h) {
  stats_handle_clear(h->jlog_id);
  stats_handle_clear(h->total_events);
  stats_handle_clear(h->total_bytes_read);
  stats_handle_clear(h->last_event_age);
  stats_handle_clear(h->connection_age);
  stats_handle_clear(h->batch_size);
  stats_handle_clear(h->batch_read_latency);
  stats_handle_clear(h->batch_commit_latency);
}

static const char *feed_type_to_str(int jlog_feed_cmd) {
  switch(jlog_feed_cmd) {
    case NOIT_JLOG_DATA_FEED: return "durable/storage";
    case NOIT_JLOG_DATA_TEMP_FEED: return "transient/iep";
  }
  return "unknown";
}

static const char *jlog_state_to_str(int state) {
  switch(state) {
    case JLOG_STREAMER_WANT_INITIATE: return "initiate"; break;
    case JLOG_STREAMER_WANT_COUNT: return "waiting for next batch"; break;
    case JLOG_STREAMER_WANT_ERROR: return "waiting for error"; break;
    case JLOG_STREAMER_WANT_HEADER: return "reading header"; break;
    case JLOG_STREAMER_WANT_BODY: return "reading body"; break;
    case JLOG_STREAMER_IS_ASYNC: return "asynchronously processing"; break;
    case JLOG_STREAMER_WANT_CHKPT: return "checkpointing"; break;
  }
  return "unknown";
}

static void change_state(jlog_streamer_ctx_t *ctx, mtev_connection_ctx_t *nctx, int new_state, struct timeval *now) {
  if (N_L_S_ON(jlog_streamer_deb)) {
    mtevL(jlog_streamer_deb, "changing state (type: %s) from \"%s\" to \"%s\" - [%s] [%s]\n",
          feed_type_to_str(ntohl(ctx->jlog_feed_cmd)),
          jlog_state_to_str(ctx->state),
          jlog_state_to_str(new_state),
          (nctx && nctx->remote_str) ? nctx->remote_str : "(null)",
          (nctx && nctx->remote_cn) ? nctx->remote_cn : "(null)");
  }
  if(now) ctx->state_change = *now;
  ctx->state = new_state;
}

#define GET_EXPECTED_CN(nctx, cn) do { \
  void *vcn; \
  cn = NULL; \
  if(nctx->config && \
     mtev_hash_retrieve(nctx->config, "cn", 2, &vcn)) { \
     cn = vcn; \
  } \
} while(0)
#define GET_FEEDTYPE(nctx, feedtype) do { \
  jlog_streamer_ctx_t *_jctx = nctx->consumer_ctx; \
  feedtype = "unknown"; \
  if(_jctx->push == stratcon_datastore_push) \
    feedtype = "storage"; \
  else if(_jctx->push == stratcon_iep_line_processor) \
    feedtype = "iep"; \
} while(0)

static int
remote_str_sort(const void *a, const void *b) {
  int rv;
  mtev_connection_ctx_t * const *actx = a;
  mtev_connection_ctx_t * const *bctx = b;
  jlog_streamer_ctx_t *ajctx = (*actx)->consumer_ctx;
  jlog_streamer_ctx_t *bjctx = (*bctx)->consumer_ctx;
  rv = strcmp((*actx)->remote_str, (*bctx)->remote_str);
  if(rv) return rv;
  return (ajctx->jlog_feed_cmd < bjctx->jlog_feed_cmd) ? -1 :
           ((ajctx->jlog_feed_cmd == bjctx->jlog_feed_cmd) ? 0 : 1);
}
static void
nc_print_noit_conn_brief(mtev_console_closure_t ncct,
                          mtev_connection_ctx_t *ctx) {
  jlog_streamer_ctx_t *jctx = ctx->consumer_ctx;
  struct timeval now, diff, session_duration;
  char cmdbuf[4096];
  const char *feedtype = "unknown";
  const char *lasttime = "never";
  const char *config_cn = NULL;
  void *vcn;
  if(ctx->last_connect.tv_sec != 0) {
    time_t r = ctx->last_connect.tv_sec;
    struct tm tbuf, *tm;
    tm = gmtime_r(&r, &tbuf);
    strftime(cmdbuf, sizeof(cmdbuf), "%Y-%m-%d %H:%M:%S UTC", tm);
    lasttime = cmdbuf;
  }
  nc_printf(ncct, "%s [%s]:\n", ctx->remote_str,
            ctx->remote_cn ? "connected" :
                             (ctx->retry_event ? "disconnected" :
                                                   "connecting"));
  if(ctx->config &&
     mtev_hash_retrieve(ctx->config, "cn", strlen("cn"), &vcn)) {
     config_cn = vcn;
  }
  if(config_cn) nc_printf(ncct, "\tcn: %s\n", config_cn);
  nc_printf(ncct, "\tLast connect: %s\n", lasttime);
  if(ctx->e) {
    char buff[128];
    const char *addrstr = NULL;
    struct sockaddr_in6 addr6;
    socklen_t len = sizeof(addr6);
    if(getsockname(eventer_get_fd(ctx->e), (struct sockaddr *)&addr6, &len) == 0) {
      unsigned short port = 0;
      if(addr6.sin6_family == AF_INET) {
        addrstr = inet_ntop(addr6.sin6_family,
                            &((struct sockaddr_in *)&addr6)->sin_addr,
                            buff, sizeof(buff));
        memcpy(&port, &(&addr6)->sin6_port, sizeof(port));
        port = ntohs(port);
      }
      else if(addr6.sin6_family == AF_INET6) {
        addrstr = inet_ntop(addr6.sin6_family, &addr6.sin6_addr,
                            buff, sizeof(buff));
        port = ntohs(addr6.sin6_port);
      }
      if(addrstr != NULL)
        nc_printf(ncct, "\tLocal address is %s:%u\n", buff, port);
      else
        nc_printf(ncct, "\tLocal address not interpretable\n");
    }
    else {
      nc_printf(ncct, "\tLocal address error[%d]: %s\n",
                eventer_get_fd(ctx->e), strerror(errno));
    }
  }
  feedtype = feed_type_to_str(ntohl(jctx->jlog_feed_cmd));
  nc_printf(ncct, "\tJLog event streamer [%s]\n", feedtype);
  mtev_gettimeofday(&now, NULL);
  if(ctx->timeout_event) {
    struct timeval te = eventer_get_whence(ctx->timeout_event);
    sub_timeval(te, now, &diff);
    nc_printf(ncct, "\tTimeout scheduled for %lld.%06us\n",
              (long long)diff.tv_sec, (unsigned int) diff.tv_usec);
  }
  if(ctx->retry_event) {
    struct timeval re = eventer_get_whence(ctx->retry_event);
    sub_timeval(re, now, &diff);
    nc_printf(ncct, "\tNext attempt in %lld.%06us\n",
              (long long)diff.tv_sec, (unsigned int) diff.tv_usec);
  }
  else if(ctx->remote_cn) {
    nc_printf(ncct, "\tRemote CN: '%s'\n",
              ctx->remote_cn ? ctx->remote_cn : "???");
    if(ctx->consumer_callback == stratcon_jlog_recv_handler) {
      struct timeval last;
      double session_duration_seconds;
      const char *state = "unknown";

      switch(jctx->state) {
        case JLOG_STREAMER_WANT_INITIATE: state = "initiate"; break;
        case JLOG_STREAMER_WANT_COUNT: state = "waiting for next batch"; break;
        case JLOG_STREAMER_WANT_ERROR: state = "waiting for error"; break;
        case JLOG_STREAMER_WANT_HEADER: state = "reading header"; break;
        case JLOG_STREAMER_WANT_BODY: state = "reading body"; break;
        case JLOG_STREAMER_IS_ASYNC: state = "asynchronously processing"; break;
        case JLOG_STREAMER_WANT_CHKPT: state = "checkpointing"; break;
      }
      last.tv_sec = jctx->header.tv_sec;
      last.tv_usec = jctx->header.tv_usec;
      sub_timeval(now, last, &diff);
      sub_timeval(now, ctx->last_connect, &session_duration);
      session_duration_seconds = session_duration.tv_sec +
                                 (double)session_duration.tv_usec/1000000.0;
      nc_printf(ncct, "\tState: %s\n"
                      "\tNext checkpoint: [%08x:%08x]\n"
                      "\tLast event: %lld.%06us ago\n"
                      "\tEvents this session: %llu (%0.2f/s)\n"
                      "\tOctets this session: %llu (%0.2f/s)\n",
                state,
                jctx->header.chkpt.log, jctx->header.chkpt.marker,
                (long long)diff.tv_sec, (unsigned int)diff.tv_usec,
                jctx->total_events,
                (double)jctx->total_events/session_duration_seconds,
                jctx->total_bytes_read,
                (double)jctx->total_bytes_read/session_duration_seconds);
    }
    else {
      nc_printf(ncct, "\tUnknown type.\n");
    }
  }
}

jlog_streamer_ctx_t *
stratcon_jlog_streamer_datastore_ctx_alloc(void) {
  jlog_streamer_ctx_t *ctx;
  ctx = stratcon_jlog_streamer_ctx_alloc();
  ctx->jlog_feed_cmd = htonl(NOIT_JLOG_DATA_FEED);
  ctx->push = stratcon_datastore_push;
  return ctx;
}
jlog_streamer_ctx_t *
stratcon_jlog_streamer_ctx_alloc(void) {
  jlog_streamer_ctx_t *ctx;
  ctx = calloc(1, sizeof(*ctx));
  return ctx;
}

void
jlog_streamer_ctx_free(void *cl) {
  jlog_streamer_ctx_t *ctx = cl;
  if(ctx->buffer) free(ctx->buffer);
  free(ctx);
}

#define Eread(a,b) eventer_read(e, (a), (b), &mask)
static int
__read_on_ctx(eventer_t e, jlog_streamer_ctx_t *ctx, int *newmask) {
  int len, mask;
  while(ctx->bytes_read < ctx->bytes_expected) {
    len = Eread(ctx->buffer + ctx->bytes_read,
                ctx->bytes_expected - ctx->bytes_read);
    if(len < 0) {
      *newmask = mask;
      return -1;
    }
    /* if we get 0 inside SSL, and there was a real error, we
     * will actually get a -1 here.
     * if(len == 0) return ctx->bytes_read;
     */
    ctx->total_bytes_read += len;
    if(ctx->stats) stats_set(ctx->stats->total_bytes_read, STATS_TYPE_UINT64, &ctx->total_bytes_read);
    ctx->bytes_read += len;
  }
  mtevAssert(ctx->bytes_read == ctx->bytes_expected);
  return ctx->bytes_read;
}
#define FULLREAD(e,ctx,size) do { \
  int mask, len; \
  if(!ctx->bytes_expected) { \
    ctx->bytes_expected = size; \
    if(ctx->buffer) free(ctx->buffer); \
    ctx->buffer = malloc(size + 1); \
    if(ctx->buffer == NULL) { \
      mtevL(jlog_streamer_err, "malloc(%lu) failed.\n", (long unsigned int)size + 1); \
      goto socket_error; \
    } \
    ctx->buffer[size] = '\0'; \
  } \
  len = __read_on_ctx(e, ctx, &mask); \
  if(len < 0) { \
    if(errno == EAGAIN) return mask | EVENTER_EXCEPTION; \
    const char *error = NULL; \
    /* libmtev's SSL layer uses EIO to indicate SSL-related errors. */ \
    if(errno == EIO) { \
      eventer_ssl_ctx_t *sslctx = eventer_get_eventer_ssl_ctx(e); \
      if(sslctx) error = eventer_ssl_get_last_error(sslctx); \
    } \
    if(! error) error = strerror(errno); \
    mtevL(jlog_streamer_err, "[%s] [%s] SSL read error: %s\n", nctx->remote_str ? nctx->remote_str : "(null)", \
          nctx->remote_cn ? nctx->remote_cn : "(null)", \
          error); \
    goto socket_error; \
  } \
  ctx->bytes_read = 0; \
  ctx->bytes_expected = 0; \
  if(len != size) { \
    mtevL(jlog_streamer_err, "[%s] [%s] SSL short read [%d] (%d/%lu).  Reseting connection.\n", \
          nctx->remote_str ? nctx->remote_str : "(null)", \
          nctx->remote_cn ? nctx->remote_cn : "(null)", \
          ctx->state, len, (long unsigned int)size); \
    goto socket_error; \
  } \
} while(0)

int
stratcon_jlog_recv_handler(eventer_t e, int mask, void *closure,
                           struct timeval *now) {
  mtev_connection_ctx_t *nctx = closure;
  jlog_streamer_ctx_t *ctx = nctx->consumer_ctx;
  jlog_streamer_ctx_t dummy;
  int len;
  jlog_id n_chkpt;
  const char *cn_expected, *feedtype;
  GET_EXPECTED_CN(nctx, cn_expected);
  GET_FEEDTYPE(nctx, feedtype);
  (void)feedtype;
  (void)cn_expected;

  if(mask & EVENTER_EXCEPTION || nctx->wants_shutdown) {
    if(write(eventer_get_fd(e), e, 0) == -1)
      mtevL(jlog_streamer_err, "[%s] [%s] socket error: %s\n", nctx->remote_str ? nctx->remote_str : "(null)", 
            nctx->remote_cn ? nctx->remote_cn : "(null)", strerror(errno));
 socket_error:
    change_state(ctx, nctx, JLOG_STREAMER_WANT_INITIATE, now);
    ctx->count = 0;
    ctx->needs_chkpt = 0;
    ctx->bytes_read = 0;
    ctx->bytes_expected = 0;
    if(ctx->buffer) free(ctx->buffer);
    ctx->buffer = NULL;
    nctx->schedule_reattempt(nctx, now);
    if(ctx->stats) stats_clear(ctx->stats);
    nctx->close(nctx, e);
    return 0;
  }

  mtev_connection_update_timeout(nctx);
  if(ctx->stats == NULL && feedtype) {
    if(!strcmp(feedtype, "iep"))
      ctx->stats = fetch_stats_for_feed(cn_expected, false);
    else if(!strcmp(feedtype, "storage"))
      ctx->stats = fetch_stats_for_feed(cn_expected, true);
  }
  if(ctx->stats) {
    struct timeval diff;
    sub_timeval(*now, nctx->last_connect, &diff);
    double last_event = (double)diff.tv_sec + (double)diff.tv_usec / 1000000.0;
    stats_set(ctx->stats->connection_age, STATS_TYPE_DOUBLE, &last_event);
  }
  while(1) {
    switch(ctx->state) {
      case JLOG_STREAMER_WANT_INITIATE:
        len = eventer_write(e, &ctx->jlog_feed_cmd,
                            sizeof(ctx->jlog_feed_cmd), &mask);
        if(len < 0) {
          if(errno == EAGAIN) return mask | EVENTER_EXCEPTION;
          mtevL(jlog_streamer_err, "[%s] [%s] initiating stream failed -> %d/%s.\n", 
                nctx->remote_str ? nctx->remote_str : "(null)", nctx->remote_cn ? nctx->remote_cn : "(null)", errno, strerror(errno));
          goto socket_error;
        }
        if(len != sizeof(ctx->jlog_feed_cmd)) {
          mtevL(jlog_streamer_err, "[%s] [%s] short write [%d/%d] on initiating stream.\n", 
                nctx->remote_str ? nctx->remote_str : "(null)", nctx->remote_cn ? nctx->remote_cn : "(null)",
                (int)len, (int)sizeof(ctx->jlog_feed_cmd));
          goto socket_error;
        }
        change_state(ctx, nctx, JLOG_STREAMER_WANT_COUNT, now);
        break;

      case JLOG_STREAMER_WANT_ERROR:
        FULLREAD(e, ctx, 0 - ctx->count);
        mtevL(jlog_streamer_err, "[%s] [%s] %.*s\n", nctx->remote_str ? nctx->remote_str : "(null)",
              nctx->remote_cn ? nctx->remote_cn : "(null)", 0 - ctx->count, ctx->buffer);
        free(ctx->buffer); ctx->buffer = NULL;
        goto socket_error;
        break;

      case JLOG_STREAMER_WANT_COUNT:
        FULLREAD(e, ctx, sizeof(uint32_t));
        memcpy(&dummy.count, ctx->buffer, sizeof(uint32_t));
        ctx->count = ntohl(dummy.count);
        ctx->needs_chkpt = 0;
        free(ctx->buffer); ctx->buffer = NULL;
        STRATCON_STREAM_COUNT(eventer_get_fd(e), (char *)feedtype,
                                   nctx->remote_str, (char *)cn_expected,
                                   ctx->count);
        if(ctx->count < 0)
          change_state(ctx, nctx, JLOG_STREAMER_WANT_ERROR, now);
        else
          change_state(ctx, nctx, JLOG_STREAMER_WANT_HEADER, now);
        break;

      case JLOG_STREAMER_WANT_HEADER:
        if(ctx->count == 0) {
          change_state(ctx, nctx, JLOG_STREAMER_WANT_COUNT, now);
          break;
        }
        FULLREAD(e, ctx, sizeof(ctx->header));
        memcpy(&dummy.header, ctx->buffer, sizeof(ctx->header));
        ctx->header.chkpt.log = ntohl(dummy.header.chkpt.log);
        ctx->header.chkpt.marker = ntohl(dummy.header.chkpt.marker);
        ctx->header.tv_sec = ntohl(dummy.header.tv_sec);
        ctx->header.tv_usec = ntohl(dummy.header.tv_usec);
        ctx->header.message_len = ntohl(dummy.header.message_len);
        STRATCON_STREAM_HEADER(eventer_get_fd(e), (char *)feedtype,
                                    nctx->remote_str, (char *)cn_expected,
                                    ctx->header.chkpt.log, ctx->header.chkpt.marker,
                                    ctx->header.tv_sec, ctx->header.tv_usec,
                                    ctx->header.message_len);
        free(ctx->buffer); ctx->buffer = NULL;
        if(ctx->stats) {
          struct timeval diff, last = { .tv_sec = ctx->header.tv_sec, .tv_usec = ctx->header.tv_usec };
          sub_timeval(*now, last, &diff);
          double last_event = (double)diff.tv_sec + (double)diff.tv_usec / 1000000.0;
          stats_set(ctx->stats->last_event_age, STATS_TYPE_DOUBLE, &last_event);
        }
        change_state(ctx, nctx, JLOG_STREAMER_WANT_BODY, now);
        break;

      case JLOG_STREAMER_WANT_BODY:
        FULLREAD(e, ctx, (unsigned long)ctx->header.message_len);
        STRATCON_STREAM_BODY(eventer_get_fd(e), (char *)feedtype,
                                  nctx->remote_str, (char *)cn_expected,
                                  ctx->header.chkpt.log, ctx->header.chkpt.marker,
                                  ctx->header.tv_sec, ctx->header.tv_usec,
                                  ctx->buffer);
        if(ctx->header.message_len > 0) {
          ctx->needs_chkpt = 1;
          ctx->push(DS_OP_INSERT, &nctx->r.remote, nctx->remote_cn,
                    ctx->buffer, NULL);
        }
        else if(ctx->buffer)
          free(ctx->buffer);
        /* Don't free the buffer, it's used by the datastore process. */
        ctx->buffer = NULL;
        ctx->count--;
        ctx->total_events++;
        if(ctx->stats) {
          stats_set_hist_intscale(ctx->stats->batch_size, ctx->count, 0, 1);
          stats_set(ctx->stats->total_events, STATS_TYPE_UINT64, &ctx->total_events);
        }
        if(ctx->count == 0 && ctx->needs_chkpt) {
          eventer_t completion_e;
          eventer_remove_fde(e);
          completion_e = eventer_alloc_copy(e);
          nctx->e = completion_e;
          eventer_set_mask(completion_e, EVENTER_READ | EVENTER_WRITE | EVENTER_EXCEPTION);
          /* register read latency */
          if(ctx->stats) {
            /* register batch size and latency */
            struct timeval diff;
            sub_timeval(*now, ctx->state_change, &diff);
            stats_set_hist_intscale(ctx->stats->batch_read_latency, diff.tv_sec * 1000000 + diff.tv_usec, -6, 1);
          }
          change_state(ctx, nctx, JLOG_STREAMER_IS_ASYNC, now);
          ctx->push(DS_OP_CHKPT, &nctx->r.remote, nctx->remote_cn,
                    NULL, completion_e);
          mtevL(jlog_streamer_deb, "stratcon_jlog_recv_handler: Pushing %s batch async [%s] [%s]: [%u/%u]\n",
                feed_type_to_str(ntohl(ctx->jlog_feed_cmd)),
                nctx->remote_str ? nctx->remote_str : "(null)",
                nctx->remote_cn ? nctx->remote_cn : "(null)",
                ctx->header.chkpt.log, ctx->header.chkpt.marker);
          mtev_connection_disable_timeout(nctx);
          return 0;
        }
        else if(ctx->count == 0)
          change_state(ctx, nctx, JLOG_STREAMER_WANT_CHKPT, now);
        else
          change_state(ctx, nctx, JLOG_STREAMER_WANT_HEADER, now);
        break;

      case JLOG_STREAMER_IS_ASYNC:
        if(ctx->stats) {
          /* register batch size and latency */
          struct timeval diff;
          sub_timeval(*now, ctx->state_change, &diff);
          stats_set_hist_intscale(ctx->stats->batch_commit_latency, diff.tv_sec * 1000000 + diff.tv_usec, -6, 1);
        }
        change_state(ctx, nctx, JLOG_STREAMER_WANT_CHKPT, now); /* falls through */
      case JLOG_STREAMER_WANT_CHKPT:
        mtevL(jlog_streamer_deb, "stratcon_jlog_recv_handler: Pushing %s checkpoint [%s] [%s]: [%u/%u]\n",
              feed_type_to_str(ntohl(ctx->jlog_feed_cmd)),
              nctx->remote_str ? nctx->remote_str : "(null)",
              nctx->remote_cn ? nctx->remote_cn : "(null)",
              ctx->header.chkpt.log, ctx->header.chkpt.marker);
        n_chkpt.log = htonl(ctx->header.chkpt.log);
        n_chkpt.marker = htonl(ctx->header.chkpt.marker);

        /* screw short writes.  I'd rather die than not write my data! */
        len = eventer_write(e, &n_chkpt, sizeof(jlog_id), &mask);
        if(len < 0) {
          if(errno == EAGAIN) {
            mtevL(jlog_streamer_deb, "stratcon_jlog_recv_handler: failed eventer write: got EAGAIN\n");
            return mask | EVENTER_EXCEPTION;
          }
          mtevL(jlog_streamer_deb, "stratcon_jlog_recv_handler: failed eventer write %d (%s) - goto socket_error\n",
                errno, strerror(errno));
          goto socket_error;
        }
        if(len != sizeof(jlog_id)) {
          mtevL(jlog_streamer_err, "[%s] [%s] short write on checkpointing stream.\n", 
            nctx->remote_str ? nctx->remote_str : "(null)",
            nctx->remote_cn ? nctx->remote_cn : "(null)");
          goto socket_error;
        }
        STRATCON_STREAM_CHECKPOINT(eventer_get_fd(e), (char *)feedtype,
                                        nctx->remote_str, (char *)cn_expected,
                                        ctx->header.chkpt.log, ctx->header.chkpt.marker);
        if(ctx->stats) stats_set(ctx->stats->jlog_id, STATS_TYPE_UINT32, &ctx->header.chkpt.log);
        change_state(ctx, nctx, JLOG_STREAMER_WANT_COUNT, now);
        break;
      default:
        break;
    }
  }
  /* never get here */
}


int
stratcon_find_noit_ip_by_cn(const char *cn, char *ip, int len) {
  int rv = -1;
  void *vip;
  pthread_mutex_lock(&noit_ip_by_cn_lock);
  if(mtev_hash_retrieve(&noit_ip_by_cn, cn, strlen(cn), &vip)) {
    int new_len;
    struct noit_meta *new_ip = (struct noit_meta *)vip;
    new_len = strlen(new_ip->target);
    strlcpy(ip, new_ip->target, len);
    if(new_len >= len) rv = new_len+1;
    else rv = 0;
  }
  pthread_mutex_unlock(&noit_ip_by_cn_lock);
  return rv;
}
void
stratcon_jlog_streamer_recache_noit() {
  int di, cnt;
  mtev_conf_section_t *noit_configs;
  noit_configs = mtev_conf_get_sections_read(MTEV_CONF_ROOT, "//noits//noit", &cnt);
  pthread_mutex_lock(&noit_ip_by_cn_lock);
  mtev_hash_delete_all(&noit_ip_by_cn, consul_stop, noit_meta_free);
  for(di=0; di<cnt; di++) {
    char address[64];
    if(mtev_conf_env_off(noit_configs[di], NULL)) continue;
    if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/@address",
                                 address, sizeof(address))) {
      int port = 43191;
      mtev_conf_get_int32(noit_configs[di], "self::node()/@port", &port);
      char expected_cn[256];
      if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/config/cn",
                                 expected_cn, sizeof(expected_cn))) {
        struct noit_meta *tgt = noit_meta_new(expected_cn, address, port);
        mtev_hash_store(&noit_ip_by_cn,
                        tgt->cn, strlen(tgt->cn),
                        tgt);
        consul_start(tgt, true);
      }
    }
  }
  mtev_conf_release_sections_read(noit_configs, cnt);
  pthread_mutex_unlock(&noit_ip_by_cn_lock);
}
void
stratcon_jlog_streamer_reload(const char *toplevel) {
  /* flush and repopulate the cn cache */
  stratcon_jlog_streamer_recache_noit();
  if(!stratcon_datastore_get_enabled()) return;
  stratcon_streamer_connection(toplevel, NULL, "noit",
                               stratcon_jlog_recv_handler,
                               (void *(*)())stratcon_jlog_streamer_datastore_ctx_alloc,
                               NULL,
                               jlog_streamer_ctx_free);
}

char *
stratcon_console_noit_opts(mtev_console_closure_t ncct,
                           mtev_console_state_stack_t *stack,
                           mtev_console_state_t *dstate,
                           int argc, char **argv, int idx) {
  if(argc == 1) {
    mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
    const char *key_id;
    int klen, i = 0;
    void *vconn, *vcn;
    mtev_connection_ctx_t *ctx;
    mtev_hash_table dedup;

    mtev_hash_init(&dedup);

    pthread_mutex_lock(&noits_lock);
    while(mtev_hash_next(&noits, &iter, &key_id, &klen, &vconn)) {
      ctx = (mtev_connection_ctx_t *)vconn;
      vcn = NULL;
      if(ctx->config && mtev_hash_retrieve(ctx->config, "cn", 2, &vcn) &&
         !mtev_hash_store(&dedup, vcn, strlen(vcn), NULL)) {
        if(!strncmp(vcn, argv[0], strlen(argv[0]))) {
          if(idx == i) {
            pthread_mutex_unlock(&noits_lock);
            mtev_hash_destroy(&dedup, NULL, NULL);
            return strdup(vcn);
          }
          i++;
        }
      }
      if(ctx->remote_str &&
         !mtev_hash_store(&dedup, ctx->remote_str, strlen(ctx->remote_str), NULL)) {
        if(!strncmp(ctx->remote_str, argv[0], strlen(argv[0]))) {
          if(idx == i) {
            pthread_mutex_unlock(&noits_lock);
            mtev_hash_destroy(&dedup, NULL, NULL);
            return strdup(ctx->remote_str);
          }
          i++;
        }
      }
    }
    pthread_mutex_unlock(&noits_lock);
    mtev_hash_destroy(&dedup, NULL, NULL);
  }
  if(argc == 2)
    return mtev_console_opt_delegate(ncct, stack, dstate, argc-1, argv+1, idx);
  return NULL;
}
static int
stratcon_console_show_noits(mtev_console_closure_t ncct,
                            int argc, char **argv,
                            mtev_console_state_t *dstate,
                            void *closure) {
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  const char *key_id, *ecn;
  int klen, n = 0, i;
  void *vconn;
  mtev_connection_ctx_t **ctx;

  if(closure != (void *)0 && argc == 0) {
    nc_printf(ncct, "takes an argument\n");
    return 0;
  }
  if(closure == (void *)0 && argc > 0) {
    nc_printf(ncct, "takes no arguments\n");
    return 0;
  }
  pthread_mutex_lock(&noits_lock);
  ctx = malloc(sizeof(*ctx) * mtev_hash_size(&noits));
  while(mtev_hash_next(&noits, &iter, &key_id, &klen,
                       &vconn)) {
    ctx[n] = (mtev_connection_ctx_t *)vconn;
    if(argc == 0 ||
       !strcmp(ctx[n]->remote_str, argv[0]) ||
       (ctx[n]->config && mtev_hash_retr_str(ctx[n]->config, "cn", 2, &ecn) &&
        !strcmp(ecn, argv[0]))) {
      mtev_connection_ctx_ref(ctx[n]);
      n++;
    }
  }
  pthread_mutex_unlock(&noits_lock);
  qsort(ctx, n, sizeof(*ctx), remote_str_sort);
  for(i=0; i<n; i++) {
    nc_print_noit_conn_brief(ncct, ctx[i]);
    mtev_connection_ctx_deref(ctx[i]);
  }
  free(ctx);
  return 0;
}

static void
emit_noit_info_metrics(struct timeval *now, const char *uuid_str,
                       mtev_connection_ctx_t *nctx) {
  struct timeval last, session_duration, diff;
  uint64_t session_duration_ms, last_event_ms;
  jlog_streamer_ctx_t *jctx = nctx->consumer_ctx;
  char str[1024], *wr;
  int len;
  const char *cn_expected;
  const char *feedtype = "unknown";

  GET_FEEDTYPE(nctx, feedtype);
  if(NULL != (wr = strchr(feedtype, '/'))) feedtype = wr+1;

  GET_EXPECTED_CN(nctx, cn_expected);
  if(!cn_expected) return;

  snprintf(str, sizeof(str), "M\t%lu.%03lu\t%s\t%s`%s`",
           (long unsigned int)now->tv_sec,
           (long unsigned int)now->tv_usec/1000UL,
           uuid_str, cn_expected, feedtype);
  wr = str + strlen(str);
  len = sizeof(str) - (wr - str);


  /* Now we write NAME TYPE VALUE into wr each time and push it */
#define push_noit_m_str(name, value) do { \
  snprintf(wr, len, "%s\ts\t%s\n", name, value); \
  stratcon_datastore_push(DS_OP_INSERT, \
                          (struct sockaddr *)&self_stratcon_ip, \
                          self_stratcon_hostname, strdup(str), NULL); \
  stratcon_iep_line_processor(DS_OP_INSERT, \
                              (struct sockaddr *)&self_stratcon_ip, \
                              self_stratcon_hostname, strdup(str), NULL); \
} while(0)
#define push_noit_m_u64(name, value) do { \
  snprintf(wr, len, "%s\tL\t%llu\n", name, (long long unsigned int)value); \
  stratcon_datastore_push(DS_OP_INSERT, \
                          (struct sockaddr *)&self_stratcon_ip, \
                          self_stratcon_hostname, strdup(str), NULL); \
  stratcon_iep_line_processor(DS_OP_INSERT, \
                              (struct sockaddr *)&self_stratcon_ip, \
                              self_stratcon_hostname, strdup(str), NULL); \
} while(0)

  last.tv_sec = jctx->header.tv_sec;
  last.tv_usec = jctx->header.tv_usec;
  sub_timeval(*now, last, &diff);
  last_event_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;
  sub_timeval(*now, nctx->last_connect, &session_duration);
  session_duration_ms = session_duration.tv_sec * 1000 +
                        session_duration.tv_usec / 1000;

  if(feedtype && (!strcmp(feedtype, "iep") || !strcmp(feedtype, "storage"))) {
    consul_update(cn_expected, !strcmp(feedtype, "storage"),
                  nctx->retry_event, last_event_ms, session_duration_ms);
  }
  push_noit_m_str("state", nctx->remote_cn ? "connected" :
                             (nctx->retry_event ? "disconnected" :
                                                  "connecting"));
  push_noit_m_u64("last_event_age_ms", last_event_ms);
  push_noit_m_u64("session_length_ms", session_duration_ms);
}
static int
periodic_noit_metrics(eventer_t e, int mask, void *closure,
                      struct timeval *now) {
  mtev_connection_ctx_t **ctxs;
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  const char *key_id;
  void *vconn;
  int klen, n = 0, i;
  char str[1024];
  char uuid_str[128], tmp_uuid_str[UUID_STR_LEN+1];
  struct timeval epoch, diff;
  uint64_t uptime = 0;
  char ip_str[128];

  inet_ntop(AF_INET, &self_stratcon_ip.sin_addr, ip_str,
            sizeof(ip_str));

  uuid_str[0] = '\0';
  mtev_uuid_unparse_lower(self_stratcon_id, tmp_uuid_str);
  if(stratcon_selfcheck_extended_id) {
    strlcat(uuid_str, ip_str, sizeof(uuid_str)-37);
    strlcat(uuid_str, "`selfcheck`selfcheck`", sizeof(uuid_str)-37);
  }
  strlcat(uuid_str, tmp_uuid_str, sizeof(uuid_str));

#define PUSH_BOTH(type, str) do { \
  stratcon_datastore_push(type, \
                          (struct sockaddr *)&self_stratcon_ip, \
                          self_stratcon_hostname, str, NULL); \
  stratcon_iep_line_processor(type, \
                              (struct sockaddr *)&self_stratcon_ip, \
                              self_stratcon_hostname, str, NULL); \
} while(0)

  if(closure == NULL) {
    /* Only do this the first time we get called */
    snprintf(str, sizeof(str), "C\t%lu.%03lu\t%s\t%s\tstratcon\t%s\n",
             (long unsigned int)now->tv_sec,
             (long unsigned int)now->tv_usec/1000UL, uuid_str, ip_str,
             self_stratcon_hostname);
    PUSH_BOTH(DS_OP_INSERT, strdup(str));
  }

  pthread_mutex_lock(&noits_lock);
  ctxs = malloc(sizeof(*ctxs) * mtev_hash_size(&noits));
  while(mtev_hash_next(&noits, &iter, &key_id, &klen,
                       &vconn)) {
    ctxs[n] = (mtev_connection_ctx_t *)vconn;
    mtev_connection_ctx_ref(ctxs[n]);
    n++;
  }
  pthread_mutex_unlock(&noits_lock);

  snprintf(str, sizeof(str), "S\t%lu.%03lu\t%s\tG\tA\t0\tok %d noits\n",
           (long unsigned int)now->tv_sec,
           (long unsigned int)now->tv_usec/1000UL, uuid_str, n);
  PUSH_BOTH(DS_OP_INSERT, strdup(str));

  if(eventer_get_epoch(&epoch) != 0)
    memcpy(&epoch, now, sizeof(epoch));
  sub_timeval(*now, epoch, &diff);
  uptime = diff.tv_sec;
  snprintf(str, sizeof(str), "M\t%lu.%03lu\t%s\tuptime\tL\t%llu\n",
           (long unsigned int)now->tv_sec,
           (long unsigned int)now->tv_usec/1000UL,
           uuid_str, (long long unsigned int)uptime);
  PUSH_BOTH(DS_OP_INSERT, strdup(str));

  for(i=0; i<n; i++) {
    emit_noit_info_metrics(now, uuid_str, ctxs[i]);
    mtev_connection_ctx_deref(ctxs[i]);
  }
  free(ctxs);
  PUSH_BOTH(DS_OP_CHKPT, NULL);

  eventer_add_in(periodic_noit_metrics, (void *)0x1, DEFAULT_NOIT_PERIOD_TV);
  return 0;
}

static int
rest_show_noits_json(mtev_http_rest_closure_t *restc,
                     int npats, char **pats) {
  const char *jsonstr;
  struct json_object *doc, *nodes, *node;
  mtev_hash_table seen;
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  char path[256];
  const char *key_id;
  const char *type = NULL, *want_cn = NULL;
  int klen, n = 0, i, di, cnt;
  void *vconn;
  mtev_connection_ctx_t **ctxs;
  mtev_conf_section_t *noit_configs;
  struct timeval now, diff, last;
  mtev_http_request *req = mtev_http_session_request(restc->http_ctx);

  mtev_hash_init(&seen);

  type = mtev_http_request_querystring(req, "type");
  want_cn = mtev_http_request_querystring(req, "cn");

  mtev_gettimeofday(&now, NULL);

  pthread_mutex_lock(&noits_lock);
  ctxs = malloc(sizeof(*ctxs) * mtev_hash_size(&noits));
  while(mtev_hash_next(&noits, &iter, &key_id, &klen,
                       &vconn)) {
    ctxs[n] = (mtev_connection_ctx_t *)vconn;
    mtev_connection_ctx_ref(ctxs[n]);
    n++;
  }
  pthread_mutex_unlock(&noits_lock);
  qsort(ctxs, n, sizeof(*ctxs), remote_str_sort);

  doc = json_object_new_object(); 
  nodes = json_object_new_array();
  json_object_object_add(doc, "nodes", nodes);
  
  for(i=0; i<n; i++) {
    char buff[256];
    char remote_dedup_key[2048];
    void *vcn;
    const char *config_cn = NULL;
    const char *feedtype = "unknown", *state = "unknown";
    mtev_connection_ctx_t *ctx = ctxs[i];
    jlog_streamer_ctx_t *jctx = ctx->consumer_ctx;

    if(ctx->config &&
       mtev_hash_retrieve(ctx->config, "cn", strlen("cn"), &vcn)) {
      config_cn = vcn;
    }
    feedtype = feed_type_to_str(ntohl(jctx->jlog_feed_cmd));

    /* If the user requested a specific type and we're not it, skip. */
    if(type && strcmp(feedtype, type)) {
        mtev_connection_ctx_deref(ctx);
        continue;
    }
    /* If the user wants a specific CN... limit to that. */
    if(want_cn && (!ctx->remote_cn || strcmp(want_cn, ctx->remote_cn))) {
        mtev_connection_ctx_deref(ctx);
        continue;
    }

    node = json_object_new_object();
    snprintf(buff, sizeof(buff), "%llu.%06d",
             (long long unsigned)ctx->last_connect.tv_sec,
             (int)ctx->last_connect.tv_usec);
    if(config_cn) {
      json_object_object_add(node, "cn", json_object_new_string(config_cn));
    }
    json_object_object_add(node, "last_connect", json_object_new_string(buff));
    json_object_object_add(node, "state",
         json_object_new_string(ctx->remote_cn ?
                                  "connected" :
                                  (ctx->retry_event ? "disconnected" :
                                                      "connecting")));
    if(ctx->e) {
      char buff[128];
      const char *addrstr = NULL;
      struct sockaddr_in6 addr6;
      socklen_t len = sizeof(addr6);
      if(getsockname(eventer_get_fd(ctx->e), (struct sockaddr *)&addr6, &len) == 0) {
        unsigned short port = 0;
        if(addr6.sin6_family == AF_INET) {
          addrstr = inet_ntop(addr6.sin6_family,
                              &((struct sockaddr_in *)&addr6)->sin_addr,
                              buff, sizeof(buff));
          memcpy(&port, &(&addr6)->sin6_port, sizeof(port));
          port = ntohs(port);
        }
        else if(addr6.sin6_family == AF_INET6) {
          addrstr = inet_ntop(addr6.sin6_family, &addr6.sin6_addr,
                              buff, sizeof(buff));
          port = ntohs(addr6.sin6_port);
        }
        if(addrstr != NULL) {
          snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff),
                   ":%u", port);
          json_object_object_add(node, "local", json_object_new_string(buff));
        }
      }
    }
    snprintf(remote_dedup_key, sizeof(remote_dedup_key), "%s:%s",
             config_cn ? config_cn : "", ctx->remote_str);
    mtev_hash_replace(&seen, strdup(remote_dedup_key), strlen(remote_dedup_key),
                      0, free, NULL);
    json_object_object_add(node, "remote", json_object_new_string(ctx->remote_str));
    json_object_object_add(node, "type", json_object_new_string(feedtype));
    if(ctx->retry_event) {
      struct timeval re = eventer_get_whence(ctx->retry_event);
      sub_timeval(re, now, &diff);
      snprintf(buff, sizeof(buff), "%llu.%06d",
               (long long unsigned)diff.tv_sec, (int)diff.tv_usec);
      json_object_object_add(node, "next_attempt", json_object_new_string(buff));
    }
    else if(ctx->remote_cn) {
      if(ctx->remote_cn)
        json_object_object_add(node, "remote_cn", json_object_new_string(ctx->remote_cn));
  
      switch(jctx->state) {
        case JLOG_STREAMER_WANT_INITIATE: state = "initiate"; break;
        case JLOG_STREAMER_WANT_COUNT: state = "waiting for next batch"; break;
        case JLOG_STREAMER_WANT_ERROR: state = "waiting for error"; break;
        case JLOG_STREAMER_WANT_HEADER: state = "reading header"; break;
        case JLOG_STREAMER_WANT_BODY: state = "reading body"; break;
        case JLOG_STREAMER_IS_ASYNC: state = "asynchronously processing"; break;
        case JLOG_STREAMER_WANT_CHKPT: state = "checkpointing"; break;
      }
      json_object_object_add(node, "state", json_object_new_string(state));
      snprintf(buff, sizeof(buff), "%08x:%08x", 
               jctx->header.chkpt.log, jctx->header.chkpt.marker);
      json_object_object_add(node, "checkpoint", json_object_new_string(buff));
      snprintf(buff, sizeof(buff), "%llu",
               (unsigned long long)jctx->total_events);
      json_object_object_add(node, "session_events", json_object_new_string(buff));
      snprintf(buff, sizeof(buff), "%llu",
               (unsigned long long)jctx->total_bytes_read);
      json_object_object_add(node, "session_bytes", json_object_new_string(buff));
  
      sub_timeval(now, ctx->last_connect, &diff);
      snprintf(buff, sizeof(buff), "%lld.%06d",
               (long long)diff.tv_sec, (int)diff.tv_usec);
      json_object_object_add(node, "session_duration", json_object_new_string(buff));
  
      if(jctx->header.tv_sec) {
        last.tv_sec = jctx->header.tv_sec;
        last.tv_usec = jctx->header.tv_usec;
        snprintf(buff, sizeof(buff), "%llu.%06d",
                 (unsigned long long)last.tv_sec, (int)last.tv_usec);
        json_object_object_add(node, "last_event", json_object_new_string(buff));
        sub_timeval(now, last, &diff);
        snprintf(buff, sizeof(buff), "%lld.%06d",
                 (long long)diff.tv_sec, (int)diff.tv_usec);
        json_object_object_add(node, "last_event_age", json_object_new_string(buff));
      }
    }
    json_object_array_add(nodes, node);
    mtev_connection_ctx_deref(ctx);
  }
  free(ctxs);

  if(!type || !strcmp(type, "configured")) {
    snprintf(path, sizeof(path), "//noits//noit");
    noit_configs = mtev_conf_get_sections_read(MTEV_CONF_ROOT, path, &cnt);
    for(di=0; di<cnt; di++) {
      char address[64], port_str[32], remote_str[98], remote_dedup_key[2048];
      char expected_cn_buff[256], *expected_cn = NULL;
      if(mtev_conf_env_off(noit_configs[di], NULL)) continue;
      if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/config/cn",
                                 expected_cn_buff, sizeof(expected_cn_buff)))
        expected_cn = expected_cn_buff;
      if(want_cn && (!expected_cn || strcmp(want_cn, expected_cn))) continue;
      if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/@address",
                                 address, sizeof(address))) {
        void *v;
        if(!mtev_conf_get_stringbuf(noit_configs[di], "self::node()/@port",
                                   port_str, sizeof(port_str)))
          strlcpy(port_str, "43191", sizeof(port_str));

        /* If the user wants a specific CN... limit to that. */
        if(want_cn && (!expected_cn || strcmp(want_cn, expected_cn))) {
          continue;
        }

        snprintf(remote_dedup_key, sizeof(remote_dedup_key), "%s:%s:%s",
                 expected_cn ? expected_cn : "", address, port_str);
        snprintf(remote_str, sizeof(remote_str), "%s:%s", address, port_str);
        if(!mtev_hash_retrieve(&seen, remote_dedup_key, strlen(remote_dedup_key), &v)) {
          node = json_object_new_object();
          json_object_object_add(node, "remote", json_object_new_string(remote_str));
          json_object_object_add(node, "type", json_object_new_string("configured"));
          if(expected_cn)
            json_object_object_add(node, "cn", json_object_new_string(expected_cn));
          json_object_array_add(nodes, node);
        }
      }
    }
    mtev_conf_release_sections_read(noit_configs, cnt);
  }
  mtev_hash_destroy(&seen, free, NULL);

  mtev_http_response_ok(restc->http_ctx, "application/json");
  jsonstr = json_object_to_json_string(doc);
  mtev_http_response_append(restc->http_ctx, jsonstr, strlen(jsonstr));
  mtev_http_response_append(restc->http_ctx, "\n", 1);
  json_object_put(doc);
  mtev_http_response_end(restc->http_ctx);
  return 0;
}
static int
rest_show_noits(mtev_http_rest_closure_t *restc,
                int npats, char **pats) {
  xmlDocPtr doc;
  xmlNodePtr root;
  mtev_hash_table *hdrs, seen;
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  char path[256];
  const char *key_id, *accepthdr;
  const char *type = NULL, *want_cn = NULL;
  int klen, n = 0, i, di, cnt;
  void *vconn;
  mtev_connection_ctx_t **ctxs;
  mtev_conf_section_t *noit_configs;
  struct timeval now, diff, last;
  xmlNodePtr node;
  mtev_http_request *req = mtev_http_session_request(restc->http_ctx);

  mtev_hash_init(&seen);

  if(npats == 1 && !strcmp(pats[0], ".json"))
    return rest_show_noits_json(restc, npats, pats);

  hdrs = mtev_http_request_headers_table(req);
  if(mtev_hash_retr_str(hdrs, "accept", strlen("accept"), &accepthdr)) {
    char buf[256], *brkt, *part;
    strlcpy(buf, accepthdr, sizeof(buf));
    for(part = strtok_r(buf, ",", &brkt);
        part;
        part = strtok_r(NULL, ",", &brkt)) {
      while(*part && isspace(*part)) part++;
      if(!strcmp(part, "application/json")) {
        return rest_show_noits_json(restc, npats, pats);
      }
    }
  }

  type = mtev_http_request_querystring(req, "type");
  want_cn = mtev_http_request_querystring(req, "cn");

  mtev_gettimeofday(&now, NULL);

  pthread_mutex_lock(&noits_lock);
  ctxs = malloc(sizeof(*ctxs) * mtev_hash_size(&noits));
  while(mtev_hash_next(&noits, &iter, &key_id, &klen,
                       &vconn)) {
    ctxs[n] = (mtev_connection_ctx_t *)vconn;
    mtev_connection_ctx_ref(ctxs[n]);
    n++;
  }
  pthread_mutex_unlock(&noits_lock);
  qsort(ctxs, n, sizeof(*ctxs), remote_str_sort);

  doc = xmlNewDoc((xmlChar *)"1.0");
  root = xmlNewDocNode(doc, NULL, (xmlChar *)"noits", NULL);
  xmlDocSetRootElement(doc, root);

  for(i=0; i<n; i++) {
    char buff[256];
    char remote_dedup_key[2048];
    void *vcn;
    const char *feedtype = "unknown", *state = "unknown";
    const char *config_cn = NULL;
    mtev_connection_ctx_t *ctx = ctxs[i];
    jlog_streamer_ctx_t *jctx = ctx->consumer_ctx;

    if(ctx->config &&
       mtev_hash_retrieve(ctx->config, "cn", strlen("cn"), &vcn)) {
      config_cn = vcn;
    }

    feedtype = feed_type_to_str(ntohl(jctx->jlog_feed_cmd));

    /* If the user requested a specific type and we're not it, skip. */
    if(type && strcmp(feedtype, type)) {
        mtev_connection_ctx_deref(ctx);
        continue;
    }
    /* If the user wants a specific CN... limit to that. */
    if(want_cn && (!ctx->remote_cn || strcmp(want_cn, ctx->remote_cn))) {
        mtev_connection_ctx_deref(ctx);
        continue;
    }

    node = xmlNewNode(NULL, (xmlChar *)"noit");
    snprintf(buff, sizeof(buff), "%llu.%06d",
             (long long unsigned)ctx->last_connect.tv_sec,
             (int)ctx->last_connect.tv_usec);
    if(config_cn) xmlSetProp(node, (xmlChar *)"cn", (xmlChar *)config_cn);
    xmlSetProp(node, (xmlChar *)"last_connect", (xmlChar *)buff);
    xmlSetProp(node, (xmlChar *)"state", ctx->remote_cn ?
               (xmlChar *)"connected" :
               (ctx->retry_event ? (xmlChar *)"disconnected" :
                                    (xmlChar *)"connecting"));
    if(ctx->e) {
      char buff[128];
      const char *addrstr = NULL;
      struct sockaddr_in6 addr6;
      socklen_t len = sizeof(addr6);
      if(getsockname(eventer_get_fd(ctx->e), (struct sockaddr *)&addr6, &len) == 0) {
        unsigned short port = 0;
        if(addr6.sin6_family == AF_INET) {
          addrstr = inet_ntop(addr6.sin6_family,
                              &((struct sockaddr_in *)&addr6)->sin_addr,
                              buff, sizeof(buff));
          memcpy(&port, &(&addr6)->sin6_port, sizeof(port));
          port = ntohs(port);
        }
        else if(addr6.sin6_family == AF_INET6) {
          addrstr = inet_ntop(addr6.sin6_family, &addr6.sin6_addr,
                              buff, sizeof(buff));
          port = ntohs(addr6.sin6_port);
        }
        if(addrstr != NULL) {
          snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff),
                   ":%u", port);
          xmlSetProp(node, (xmlChar *)"local", (xmlChar *)buff);
        }
      }
    }
    snprintf(remote_dedup_key, sizeof(remote_dedup_key), "%s:%s",
             config_cn ? config_cn : "", ctx->remote_str);
    mtev_hash_replace(&seen, strdup(remote_dedup_key), strlen(remote_dedup_key),
                      0, free, NULL);
    xmlSetProp(node, (xmlChar *)"remote", (xmlChar *)ctx->remote_str);
    xmlSetProp(node, (xmlChar *)"type", (xmlChar *)feedtype);
    if(ctx->retry_event) {
      struct timeval re = eventer_get_whence(ctx->retry_event);
      sub_timeval(re, now, &diff);
      snprintf(buff, sizeof(buff), "%llu.%06d",
               (long long unsigned)diff.tv_sec, (int)diff.tv_usec);
      xmlSetProp(node, (xmlChar *)"next_attempt", (xmlChar *)buff);
    }
    else if(ctx->remote_cn) {
      if(ctx->remote_cn)
        xmlSetProp(node, (xmlChar *)"remote_cn", (xmlChar *)ctx->remote_cn);
  
      switch(jctx->state) {
        case JLOG_STREAMER_WANT_INITIATE: state = "initiate"; break;
        case JLOG_STREAMER_WANT_COUNT: state = "waiting for next batch"; break;
        case JLOG_STREAMER_WANT_ERROR: state = "waiting for error"; break;
        case JLOG_STREAMER_WANT_HEADER: state = "reading header"; break;
        case JLOG_STREAMER_WANT_BODY: state = "reading body"; break;
        case JLOG_STREAMER_IS_ASYNC: state = "asynchronously processing"; break;
        case JLOG_STREAMER_WANT_CHKPT: state = "checkpointing"; break;
      }
      xmlSetProp(node, (xmlChar *)"state", (xmlChar *)state);
      snprintf(buff, sizeof(buff), "%08x:%08x", 
               jctx->header.chkpt.log, jctx->header.chkpt.marker);
      xmlSetProp(node, (xmlChar *)"checkpoint", (xmlChar *)buff);
      snprintf(buff, sizeof(buff), "%llu",
               (unsigned long long)jctx->total_events);
      xmlSetProp(node, (xmlChar *)"session_events", (xmlChar *)buff);
      snprintf(buff, sizeof(buff), "%llu",
               (unsigned long long)jctx->total_bytes_read);
      xmlSetProp(node, (xmlChar *)"session_bytes", (xmlChar *)buff);
  
      sub_timeval(now, ctx->last_connect, &diff);
      snprintf(buff, sizeof(buff), "%lld.%06d",
               (long long)diff.tv_sec, (int)diff.tv_usec);
      xmlSetProp(node, (xmlChar *)"session_duration", (xmlChar *)buff);
  
      if(jctx->header.tv_sec) {
        last.tv_sec = jctx->header.tv_sec;
        last.tv_usec = jctx->header.tv_usec;
        snprintf(buff, sizeof(buff), "%llu.%06d",
                 (unsigned long long)last.tv_sec, (int)last.tv_usec);
        xmlSetProp(node, (xmlChar *)"last_event", (xmlChar *)buff);
        sub_timeval(now, last, &diff);
        snprintf(buff, sizeof(buff), "%lld.%06d",
                 (long long)diff.tv_sec, (int)diff.tv_usec);
        xmlSetProp(node, (xmlChar *)"last_event_age", (xmlChar *)buff);
      }
    }

    xmlAddChild(root, node);
    mtev_connection_ctx_deref(ctx);
  }
  free(ctxs);

  if(!type || !strcmp(type, "configured")) {
    snprintf(path, sizeof(path), "//noits//noit");
    noit_configs = mtev_conf_get_sections_read(MTEV_CONF_ROOT, path, &cnt);
    for(di=0; di<cnt; di++) {
      char address[64], port_str[32], remote_str[98], remote_dedup_key[2048];
      char expected_cn_buff[256], *expected_cn = NULL;
      if(mtev_conf_env_off(noit_configs[di], NULL)) continue;
      if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/config/cn",
                                 expected_cn_buff, sizeof(expected_cn_buff)))
        expected_cn = expected_cn_buff;
      if(want_cn && (!expected_cn || strcmp(want_cn, expected_cn))) continue;
      if(mtev_conf_get_stringbuf(noit_configs[di], "self::node()/@address",
                                 address, sizeof(address))) {
        void *v;
        if(!mtev_conf_get_stringbuf(noit_configs[di], "self::node()/@port",
                                   port_str, sizeof(port_str)))
          strlcpy(port_str, "43191", sizeof(port_str));

        /* If the user wants a specific CN... limit to that. */
        if(want_cn && (!expected_cn || strcmp(want_cn, expected_cn))) {
          continue;
        }

        snprintf(remote_dedup_key, sizeof(remote_dedup_key), "%s:%s:%s",
                 expected_cn ? expected_cn : "", address, port_str);
        snprintf(remote_str, sizeof(remote_str), "%s:%s", address, port_str);
        if(!mtev_hash_retrieve(&seen, remote_dedup_key, strlen(remote_dedup_key), &v)) {
          node = xmlNewNode(NULL, (xmlChar *)"noit");
          xmlSetProp(node, (xmlChar *)"remote", (xmlChar *)remote_str);
          xmlSetProp(node, (xmlChar *)"type", (xmlChar *)"configured");
          if(expected_cn)
            xmlSetProp(node, (xmlChar *)"cn", (xmlChar *)expected_cn);
          xmlAddChild(root, node);
        }
      }
    }
    mtev_conf_release_sections_read(noit_configs, cnt);
  }
  mtev_hash_destroy(&seen, free, NULL);

  mtev_http_response_ok(restc->http_ctx, "text/xml");
  mtev_http_response_xml(restc->http_ctx, doc);
  mtev_http_response_end(restc->http_ctx);
  xmlFreeDoc(doc);
  return 0;
}
static int
stratcon_add_noit(const char *target, unsigned short port,
                  const char *cn) {
  int cnt;
  char path[256];
  char port_str[6];
  mtev_conf_section_t *noit_configs, parent;
  xmlNodePtr newnoit, config, cnnode;

  mtevAssert(target != NULL);
  if(strlen(target) > 0) {
    snprintf(path, sizeof(path),
             "//noits//noit[@address=\"%s\" and @port=\"%d\"]", target, port);
    noit_configs = mtev_conf_get_sections_read(MTEV_CONF_ROOT, path, &cnt);
    mtev_conf_release_sections_read(noit_configs, cnt);
    if(cnt != 0) return -1;
  }
  if(cn) {
    snprintf(path, sizeof(path),
             "//noits//noit/config/cn[text()=\"%s\"]", cn);
    noit_configs = mtev_conf_get_sections_read(MTEV_CONF_ROOT, path, &cnt);
    mtev_conf_release_sections_read(noit_configs, cnt);
    if(cnt != 0) return -1;
  }

  parent = mtev_conf_get_section_write(MTEV_CONF_ROOT, "//noits//include//noits");
  if(mtev_conf_section_is_empty(parent)) {
    mtev_conf_release_section_write(parent);
    parent = mtev_conf_get_section_write(MTEV_CONF_ROOT, "//noits");
  }
  if(mtev_conf_section_is_empty(parent)) {
    mtev_conf_release_section_write(parent);
    return -1;
  }
  snprintf(port_str, sizeof(port_str), "%d", port);
  newnoit = xmlNewNode(NULL, (xmlChar *)"noit");
  xmlSetProp(newnoit, (xmlChar *)"address", (xmlChar *)target);
  xmlSetProp(newnoit, (xmlChar *)"port", (xmlChar *)port_str);
  xmlAddChild(mtev_conf_section_to_xmlnodeptr(parent), newnoit);
  if(cn) {
    config = xmlNewNode(NULL, (xmlChar *)"config");
    cnnode = xmlNewNode(NULL, (xmlChar *)"cn");
    xmlNodeAddContent(cnnode, (xmlChar *)cn);
    xmlAddChild(config, cnnode);
    xmlAddChild(newnoit, config);
    pthread_mutex_lock(&noit_ip_by_cn_lock);
    struct noit_meta *tgt = noit_meta_new(cn, target, port);
    mtev_hash_replace(&noit_ip_by_cn, tgt->cn, strlen(tgt->cn),
                      tgt, NULL, noit_meta_free);
    consul_start(tgt, true);
    pthread_mutex_unlock(&noit_ip_by_cn_lock);
  }
  if(stratcon_datastore_get_enabled())
    stratcon_streamer_connection(NULL, cn ? cn : target, "noit",
                                 stratcon_jlog_recv_handler,
                                 (void *(*)())stratcon_jlog_streamer_datastore_ctx_alloc,
                                 NULL,
                                 jlog_streamer_ctx_free);
  if(stratcon_iep_get_enabled())
    stratcon_streamer_connection(NULL, cn ? cn : target, "noit",
                                 stratcon_jlog_recv_handler,
                                 (void *(*)())stratcon_jlog_streamer_iep_ctx_alloc,
                                 NULL,
                                 jlog_streamer_ctx_free);
  mtev_conf_release_section_write(parent);
  return 1;
}
static int
stratcon_remove_noit(const char *target, unsigned short port, const char *cn) {
  mtev_hash_iter iter = MTEV_HASH_ITER_ZERO;
  const char *key_id;
  int klen, i, cnt = 0;
  void *vconn;
  mtev_conf_section_t *noit_configs;
  char path[256];
  char remote_str[256];

  if(cn && *cn == '\0') cn = NULL;

  snprintf(remote_str, sizeof(remote_str), "%s:%d", target, port);

  /* A sweep through the config of all noits with this address,
   * possibly limited by CN if specified. */
  snprintf(path, sizeof(path),
           "//noits//noit[@address=\"%s\" and @port=\"%d\"]", target, port);
  noit_configs = mtev_conf_get_sections_write(MTEV_CONF_ROOT, path, &cnt);
  for(i=0; i<cnt; i++) {
    char expected_cn[256];
    if(mtev_conf_get_stringbuf(noit_configs[i], "self::node()/config/cn",
                               expected_cn, sizeof(expected_cn))) {
      if(!cn || !strcmp(cn, expected_cn)) {
        pthread_mutex_lock(&noit_ip_by_cn_lock);
        mtev_hash_delete(&noit_ip_by_cn, expected_cn, strlen(expected_cn),
                         consul_stop, noit_meta_free);
        pthread_mutex_unlock(&noit_ip_by_cn_lock);
      } else continue;
    }
    else if(cn) continue;
    CONF_REMOVE(noit_configs[i]);
    xmlUnlinkNode(mtev_conf_section_to_xmlnodeptr(noit_configs[i]));
    xmlFreeNode(mtev_conf_section_to_xmlnodeptr(noit_configs[i]));
  }
  mtev_conf_release_sections_write(noit_configs, cnt);

  /* A sweep through the config of all noits with this CN */
  if(cn) {
    snprintf(path, sizeof(path),
             "//noits/noit[self::node()/config/cn/text()=\"%s\"]", cn);
    noit_configs = mtev_conf_get_sections_write(MTEV_CONF_ROOT, path, &cnt);
    for(i=0; i<cnt; i++) {
      pthread_mutex_lock(&noit_ip_by_cn_lock);
      mtev_hash_delete(&noit_ip_by_cn, cn, strlen(cn), consul_stop, noit_meta_free);
      pthread_mutex_unlock(&noit_ip_by_cn_lock);
      CONF_REMOVE(noit_configs[i]);
      xmlUnlinkNode(mtev_conf_section_to_xmlnodeptr(noit_configs[i]));
      xmlFreeNode(mtev_conf_section_to_xmlnodeptr(noit_configs[i]));
    }
    mtev_conf_release_sections_write(noit_configs, cnt);
  }

  int n = 0;
  mtev_connection_ctx_t **ctx = malloc(sizeof(*ctx) * mtev_hash_size(&noits));
  pthread_mutex_lock(&noits_lock);
  while(mtev_hash_next(&noits, &iter, &key_id, &klen,
                       &vconn)) {
    const char *expected_cn;
    mtev_connection_ctx_t *nctx = (mtev_connection_ctx_t *)vconn;
    /* If the noit matches the CN... or the remote_str, pop it */
    if((cn &&
        mtev_hash_retr_str(nctx->config, "cn", strlen("cn"), &expected_cn) &&
        !strcmp(expected_cn, cn)) ||
       !strcmp(nctx->remote_str, remote_str)) {
      ctx[n] = nctx;
      mtev_connection_ctx_ref(ctx[n]);
      n++;
    }
  }
  pthread_mutex_unlock(&noits_lock);
  for(i=0; i<n; i++) {
    mtev_connection_ctx_dealloc(ctx[i]); /* once for the record */
    mtev_connection_ctx_deref(ctx[i]);   /* once for the aboce inc32 */
  }
  free(ctx);
  return n;
}
static int
rest_set_noit(mtev_http_rest_closure_t *restc,
              int npats, char **pats) {
  const char *cn = NULL;
  mtev_http_session_ctx *ctx = restc->http_ctx;
  mtev_http_request *req = mtev_http_session_request(ctx);
  unsigned short port = 43191;
  if(npats < 1 || npats > 2)
    mtev_http_response_server_error(ctx, "text/xml");
  if(npats == 2) port = atoi(pats[1]);
  cn = mtev_http_request_querystring(req, "cn");
  if(stratcon_add_noit(pats[0], port, cn) >= 0)
    mtev_http_response_ok(ctx, "text/xml");
  else
    mtev_http_response_standard(ctx, 409, "EXISTS", "text/xml");
  if(mtev_conf_write_file(NULL) != 0)
    mtevL(jlog_streamer_err, "local config write failed\n");
  mtev_conf_mark_changed();
  mtev_http_response_end(ctx);
  return 0;
}
static int
rest_delete_noit(mtev_http_rest_closure_t *restc,
                 int npats, char **pats) {
  mtev_http_session_ctx *ctx = restc->http_ctx;
  mtev_http_request *req = mtev_http_session_request(ctx);
  unsigned short port = 43191;
  if(npats < 1 || npats > 2)
    mtev_http_response_server_error(ctx, "text/xml");
  if(npats == 2) port = atoi(pats[1]);

  const char *want_cn = mtev_http_request_querystring(req, "cn");
  if(stratcon_remove_noit(pats[0], port, want_cn) >= 0)
    mtev_http_response_ok(ctx, "text/xml");
  else
    mtev_http_response_not_found(ctx, "text/xml");
  if(mtev_conf_write_file(NULL) != 0)
    mtevL(jlog_streamer_err, "local config write failed\n");
  mtev_conf_mark_changed();
  mtev_http_response_end(ctx);
  return 0;
}
static int
stratcon_console_conf_noits(mtev_console_closure_t ncct,
                            int argc, char **argv,
                            mtev_console_state_t *dstate,
                            void *closure) {
  char *cp, target[128];
  unsigned short port = 43191;
  int adding = (int)(intptr_t)closure;
  char *cn = NULL;
  if(argc != 1 && argc != 2)
    return -1;

  if(argc == 2) cn = argv[1];

  cp = strchr(argv[0], ':');
  if(cp) {
    strlcpy(target, argv[0], MIN(sizeof(target), cp-argv[0]+1));
    port = atoi(cp+1);
  }
  else strlcpy(target, argv[0], sizeof(target));
  if(adding) {
    if(stratcon_add_noit(target, port, cn) >= 0) {
      nc_printf(ncct, "Added noit at %s:%d\n", target, port);
    }
    else {
      nc_printf(ncct, "Failed to add noit at %s:%d\n", target, port);
    }
  }
  else {
    if(stratcon_remove_noit(target, port, cn) >= 0) {
      nc_printf(ncct, "Removed noit at %s:%d\n", target, port);
    }
    else {
      nc_printf(ncct, "Failed to remove noit at %s:%d\n", target, port);
    }
  }
  return 0;
}

static void
register_console_streamer_commands() {
  mtev_console_state_t *tl;
  cmd_info_t *showcmd, *confcmd, *conftcmd, *conftnocmd;

  tl = mtev_console_state_initial();
  showcmd = mtev_console_state_get_cmd(tl, "show");
  mtevAssert(showcmd && showcmd->dstate);
  confcmd = mtev_console_state_get_cmd(tl, "configure");
  conftcmd = mtev_console_state_get_cmd(confcmd->dstate, "terminal");
  conftnocmd = mtev_console_state_get_cmd(conftcmd->dstate, "no");

  mtev_console_state_add_cmd(conftcmd->dstate,
    NCSCMD("noit", stratcon_console_conf_noits, NULL, NULL, (void *)1));
  mtev_console_state_add_cmd(conftnocmd->dstate,
    NCSCMD("noit", stratcon_console_conf_noits, NULL, NULL, (void *)0));

  mtev_console_state_add_cmd(showcmd->dstate,
    NCSCMD("noit", stratcon_console_show_noits,
           stratcon_console_noit_opts, NULL, (void *)1));
  mtev_console_state_add_cmd(showcmd->dstate,
    NCSCMD("noits", stratcon_console_show_noits, NULL, NULL, NULL));
}

int
stratcon_streamer_connection(const char *toplevel, const char *destination,
                             const char *type,
                             eventer_func_t handler,
                             void *(*handler_alloc)(void), void *handler_ctx,
                             void (*handler_free)(void *)) {
  return mtev_connections_from_config(&noits, &noits_lock,
                                      toplevel, destination, type,
                                      handler, handler_alloc, handler_ctx,
                                      handler_free);
}

mtev_reverse_acl_decision_t
mtev_reverse_socket_allow_noits(const char *id, mtev_acceptor_closure_t *ac) {
  if(!strncmp(id, "noit/", 5)) return MTEV_ACL_ALLOW;
  return MTEV_ACL_ABSTAIN;
}

void
stratcon_jlog_streamer_init(const char *toplevel) {
  struct timeval whence = DEFAULT_NOIT_PERIOD_TV;
  struct in_addr remote;
  char uuid_str[UUID_STR_LEN + 1];

  jlog_streamer_err = mtev_log_stream_find("error/stratcon_jlog_streamer");
  jlog_streamer_deb = mtev_log_stream_find("debug/stratcon_jlog_streamer");
  if(!jlog_streamer_err) jlog_streamer_err = mtev_error;
  if(!jlog_streamer_deb) jlog_streamer_deb = mtev_debug;

  mtev_reverse_socket_acl(mtev_reverse_socket_allow_noits);
  pthread_mutex_init(&noits_lock, NULL);
  pthread_mutex_init(&noit_ip_by_cn_lock, NULL);
  eventer_name_callback("stratcon_jlog_recv_handler",
                        stratcon_jlog_recv_handler);
  register_console_streamer_commands();
  stratcon_jlog_streamer_reload(toplevel);
  stratcon_streamer_connection(toplevel, "", "noit", NULL, NULL, NULL, NULL);
  mtevAssert(mtev_http_rest_register_auth(
    "GET", "/noits/", "^show(.json)?$", rest_show_noits,
             mtev_http_rest_client_cert_auth
  ) == 0);
  mtevAssert(mtev_http_rest_register_auth(
    "PUT", "/noits/", "^set/([^/:]+)$", rest_set_noit,
             mtev_http_rest_client_cert_auth
  ) == 0);
  mtevAssert(mtev_http_rest_register_auth(
    "PUT", "/noits/", "^set/([^/:]*):(\\d+)$", rest_set_noit,
             mtev_http_rest_client_cert_auth
  ) == 0);
  mtevAssert(mtev_http_rest_register_auth(
    "DELETE", "/noits/", "^delete/([^/:]+)$", rest_delete_noit,
             mtev_http_rest_client_cert_auth
  ) == 0);
  mtevAssert(mtev_http_rest_register_auth(
    "DELETE", "/noits/", "^delete/([^/:]*):(\\d+)$", rest_delete_noit,
             mtev_http_rest_client_cert_auth
  ) == 0);

  mtev_uuid_clear(self_stratcon_id);

  self_stratcon_ip.sin_family = AF_INET;
  remote.s_addr = 0x08080808;
  mtev_getip_ipv4(remote, &self_stratcon_ip.sin_addr);

  if(mtev_conf_get_stringbuf(MTEV_CONF_ROOT, "/stratcon/@id",
                             uuid_str, sizeof(uuid_str)) &&
     mtev_uuid_parse(uuid_str, self_stratcon_id) == 0) {
    int32_t period;
    mtev_conf_get_boolean(MTEV_CONF_ROOT, "/stratcon/@extended_id",
                          &stratcon_selfcheck_extended_id);
    /* If a UUID was provided for stratcon itself, we will report metrics
     * on a large variety of things (including all noits).
     */
    if(mtev_conf_get_int32(MTEV_CONF_ROOT, "/stratcon/@metric_period", &period) &&
       period > 0) {
      DEFAULT_NOIT_PERIOD_TV.tv_sec = period / 1000;
      DEFAULT_NOIT_PERIOD_TV.tv_usec = (period % 1000) * 1000;
    }
    gethostname(self_stratcon_hostname, sizeof(self_stratcon_hostname));
    eventer_add_in(periodic_noit_metrics, NULL, whence);
    stats_ns_t *ns = mtev_stats_ns(NULL, "stratcon");
    stats_ns_replace_tag(ns, "stratcon-id", uuid_str);
  }
}

void
stratcon_jlog_streamer_init_globals(void) {
  mtev_hash_init(&noits);
  mtev_hash_init(&noit_ip_by_cn);
  mtev_hash_init(&noit_stats_iep);
  mtev_hash_init(&noit_stats_durable);
  stats_ns_t *ns = mtev_stats_ns(NULL, "stratcon");
  stats_ns_t *fns = mtev_stats_ns(ns, "feed");
  iep_ns = mtev_stats_ns(fns, "iep");
  stats_ns_add_tag(iep_ns, "feed-type", "iep");
  durable_ns = mtev_stats_ns(fns, "storage");
  stats_ns_add_tag(durable_ns, "feed-type", "storage");
  mtev_reverse_proxy_changed_hook_register("stratcon", consul_proxy_hook, NULL);
}

