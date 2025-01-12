// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: opentelemetry/proto/collector/trace/v1/trace_service.proto

#include "opentelemetry/proto/collector/trace/v1/trace_service.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
namespace opentelemetry {
namespace proto {
namespace collector {
namespace trace {
namespace v1 {
constexpr ExportTraceServiceRequest::ExportTraceServiceRequest(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : resource_spans_(){}
struct ExportTraceServiceRequestDefaultTypeInternal {
  constexpr ExportTraceServiceRequestDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~ExportTraceServiceRequestDefaultTypeInternal() {}
  union {
    ExportTraceServiceRequest _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ExportTraceServiceRequestDefaultTypeInternal _ExportTraceServiceRequest_default_instance_;
constexpr ExportTraceServiceResponse::ExportTraceServiceResponse(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized){}
struct ExportTraceServiceResponseDefaultTypeInternal {
  constexpr ExportTraceServiceResponseDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~ExportTraceServiceResponseDefaultTypeInternal() {}
  union {
    ExportTraceServiceResponse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ExportTraceServiceResponseDefaultTypeInternal _ExportTraceServiceResponse_default_instance_;
}  // namespace v1
}  // namespace trace
}  // namespace collector
}  // namespace proto
}  // namespace opentelemetry
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto = nullptr;

const uint32_t TableStruct_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, resource_spans_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest)},
  { 7, -1, -1, sizeof(::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::opentelemetry::proto::collector::trace::v1::_ExportTraceServiceRequest_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::opentelemetry::proto::collector::trace::v1::_ExportTraceServiceResponse_default_instance_),
};

const char descriptor_table_protodef_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n:opentelemetry/proto/collector/trace/v1"
  "/trace_service.proto\022&opentelemetry.prot"
  "o.collector.trace.v1\032(opentelemetry/prot"
  "o/trace/v1/trace.proto\"`\n\031ExportTraceSer"
  "viceRequest\022C\n\016resource_spans\030\001 \003(\0132+.op"
  "entelemetry.proto.trace.v1.ResourceSpans"
  "\"\034\n\032ExportTraceServiceResponse2\242\001\n\014Trace"
  "Service\022\221\001\n\006Export\022A.opentelemetry.proto"
  ".collector.trace.v1.ExportTraceServiceRe"
  "quest\032B.opentelemetry.proto.collector.tr"
  "ace.v1.ExportTraceServiceResponse\"\000Bs\n)i"
  "o.opentelemetry.proto.collector.trace.v1"
  "B\021TraceServiceProtoP\001Z1go.opentelemetry."
  "io/proto/otlp/collector/trace/v1b\006proto3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_deps[1] = {
  &::descriptor_table_opentelemetry_2fproto_2ftrace_2fv1_2ftrace_2eproto,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto = {
  false, false, 560, descriptor_table_protodef_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto, "opentelemetry/proto/collector/trace/v1/trace_service.proto", 
  &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_once, descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_deps, 1, 2,
  schemas, file_default_instances, TableStruct_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto::offsets,
  file_level_metadata_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto, file_level_enum_descriptors_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto, file_level_service_descriptors_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable* descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_getter() {
  return &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto(&descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto);
namespace opentelemetry {
namespace proto {
namespace collector {
namespace trace {
namespace v1 {

// ===================================================================

class ExportTraceServiceRequest::_Internal {
 public:
};

void ExportTraceServiceRequest::clear_resource_spans() {
  resource_spans_.Clear();
}
ExportTraceServiceRequest::ExportTraceServiceRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned),
  resource_spans_(arena) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
}
ExportTraceServiceRequest::ExportTraceServiceRequest(const ExportTraceServiceRequest& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      resource_spans_(from.resource_spans_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
}

inline void ExportTraceServiceRequest::SharedCtor() {
}

ExportTraceServiceRequest::~ExportTraceServiceRequest() {
  // @@protoc_insertion_point(destructor:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void ExportTraceServiceRequest::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void ExportTraceServiceRequest::ArenaDtor(void* object) {
  ExportTraceServiceRequest* _this = reinterpret_cast< ExportTraceServiceRequest* >(object);
  (void)_this;
}
void ExportTraceServiceRequest::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void ExportTraceServiceRequest::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void ExportTraceServiceRequest::Clear() {
// @@protoc_insertion_point(message_clear_start:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  resource_spans_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ExportTraceServiceRequest::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // repeated .opentelemetry.proto.trace.v1.ResourceSpans resource_spans = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(_internal_add_resource_spans(), ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<10>(ptr));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ExportTraceServiceRequest::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // repeated .opentelemetry.proto.trace.v1.ResourceSpans resource_spans = 1;
  for (unsigned int i = 0,
      n = static_cast<unsigned int>(this->_internal_resource_spans_size()); i < n; i++) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(1, this->_internal_resource_spans(i), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  return target;
}

size_t ExportTraceServiceRequest::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated .opentelemetry.proto.trace.v1.ResourceSpans resource_spans = 1;
  total_size += 1UL * this->_internal_resource_spans_size();
  for (const auto& msg : this->resource_spans_) {
    total_size +=
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(msg);
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ExportTraceServiceRequest::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    ExportTraceServiceRequest::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ExportTraceServiceRequest::GetClassData() const { return &_class_data_; }

void ExportTraceServiceRequest::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                      const ::PROTOBUF_NAMESPACE_ID::Message& from) {
  static_cast<ExportTraceServiceRequest *>(to)->MergeFrom(
      static_cast<const ExportTraceServiceRequest &>(from));
}


void ExportTraceServiceRequest::MergeFrom(const ExportTraceServiceRequest& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  resource_spans_.MergeFrom(from.resource_spans_);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void ExportTraceServiceRequest::CopyFrom(const ExportTraceServiceRequest& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:opentelemetry.proto.collector.trace.v1.ExportTraceServiceRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ExportTraceServiceRequest::IsInitialized() const {
  return true;
}

void ExportTraceServiceRequest::InternalSwap(ExportTraceServiceRequest* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  resource_spans_.InternalSwap(&other->resource_spans_);
}

::PROTOBUF_NAMESPACE_ID::Metadata ExportTraceServiceRequest::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_getter, &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_once,
      file_level_metadata_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto[0]);
}

// ===================================================================

class ExportTraceServiceResponse::_Internal {
 public:
};

ExportTraceServiceResponse::ExportTraceServiceResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase(arena, is_message_owned) {
  // @@protoc_insertion_point(arena_constructor:opentelemetry.proto.collector.trace.v1.ExportTraceServiceResponse)
}
ExportTraceServiceResponse::ExportTraceServiceResponse(const ExportTraceServiceResponse& from)
  : ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:opentelemetry.proto.collector.trace.v1.ExportTraceServiceResponse)
}





const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ExportTraceServiceResponse::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyImpl,
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeImpl,
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ExportTraceServiceResponse::GetClassData() const { return &_class_data_; }







::PROTOBUF_NAMESPACE_ID::Metadata ExportTraceServiceResponse::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_getter, &descriptor_table_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto_once,
      file_level_metadata_opentelemetry_2fproto_2fcollector_2ftrace_2fv1_2ftrace_5fservice_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace v1
}  // namespace trace
}  // namespace collector
}  // namespace proto
}  // namespace opentelemetry
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest* Arena::CreateMaybeMessage< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest >(Arena* arena) {
  return Arena::CreateMessageInternal< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest >(arena);
}
template<> PROTOBUF_NOINLINE ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* Arena::CreateMaybeMessage< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
