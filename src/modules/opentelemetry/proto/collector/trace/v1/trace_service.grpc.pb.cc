// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: opentelemetry/proto/collector/trace/v1/trace_service.proto

#include "opentelemetry/proto/collector/trace/v1/trace_service.pb.h"
#include "opentelemetry/proto/collector/trace/v1/trace_service.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace opentelemetry {
namespace proto {
namespace collector {
namespace trace {
namespace v1 {

static const char* TraceService_method_names[] = {
  "/opentelemetry.proto.collector.trace.v1.TraceService/Export",
};

std::unique_ptr< TraceService::Stub> TraceService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< TraceService::Stub> stub(new TraceService::Stub(channel, options));
  return stub;
}

TraceService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_Export_(TraceService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status TraceService::Stub::Export(::grpc::ClientContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest& request, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_Export_, context, request, response);
}

void TraceService::Stub::async::Export(::grpc::ClientContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest* request, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Export_, context, request, response, std::move(f));
}

void TraceService::Stub::async::Export(::grpc::ClientContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest* request, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_Export_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse>* TraceService::Stub::PrepareAsyncExportRaw(::grpc::ClientContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_Export_, context, request);
}

::grpc::ClientAsyncResponseReader< ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse>* TraceService::Stub::AsyncExportRaw(::grpc::ClientContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncExportRaw(context, request, cq);
  result->StartCall();
  return result;
}

TraceService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      TraceService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< TraceService::Service, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](TraceService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest* req,
             ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* resp) {
               return service->Export(ctx, req, resp);
             }, this)));
}

TraceService::Service::~Service() {
}

::grpc::Status TraceService::Service::Export(::grpc::ServerContext* context, const ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest* request, ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace opentelemetry
}  // namespace proto
}  // namespace collector
}  // namespace trace
}  // namespace v1

