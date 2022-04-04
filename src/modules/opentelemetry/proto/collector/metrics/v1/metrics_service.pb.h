// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: opentelemetry/proto/collector/metrics/v1/metrics_service.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3019000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3019004 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_bases.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
#include "opentelemetry/proto/metrics/v1/metrics.pb.h"
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[2]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto;
namespace opentelemetry {
namespace proto {
namespace collector {
namespace metrics {
namespace v1 {
class ExportMetricsServiceRequest;
struct ExportMetricsServiceRequestDefaultTypeInternal;
extern ExportMetricsServiceRequestDefaultTypeInternal _ExportMetricsServiceRequest_default_instance_;
class ExportMetricsServiceResponse;
struct ExportMetricsServiceResponseDefaultTypeInternal;
extern ExportMetricsServiceResponseDefaultTypeInternal _ExportMetricsServiceResponse_default_instance_;
}  // namespace v1
}  // namespace metrics
}  // namespace collector
}  // namespace proto
}  // namespace opentelemetry
PROTOBUF_NAMESPACE_OPEN
template<> ::opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest* Arena::CreateMaybeMessage<::opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest>(Arena*);
template<> ::opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse* Arena::CreateMaybeMessage<::opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace opentelemetry {
namespace proto {
namespace collector {
namespace metrics {
namespace v1 {

// ===================================================================

class ExportMetricsServiceRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest) */ {
 public:
  inline ExportMetricsServiceRequest() : ExportMetricsServiceRequest(nullptr) {}
  ~ExportMetricsServiceRequest() override;
  explicit constexpr ExportMetricsServiceRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ExportMetricsServiceRequest(const ExportMetricsServiceRequest& from);
  ExportMetricsServiceRequest(ExportMetricsServiceRequest&& from) noexcept
    : ExportMetricsServiceRequest() {
    *this = ::std::move(from);
  }

  inline ExportMetricsServiceRequest& operator=(const ExportMetricsServiceRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline ExportMetricsServiceRequest& operator=(ExportMetricsServiceRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ExportMetricsServiceRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const ExportMetricsServiceRequest* internal_default_instance() {
    return reinterpret_cast<const ExportMetricsServiceRequest*>(
               &_ExportMetricsServiceRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ExportMetricsServiceRequest& a, ExportMetricsServiceRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(ExportMetricsServiceRequest* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ExportMetricsServiceRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ExportMetricsServiceRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ExportMetricsServiceRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ExportMetricsServiceRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const ExportMetricsServiceRequest& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to, const ::PROTOBUF_NAMESPACE_ID::Message& from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ExportMetricsServiceRequest* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest";
  }
  protected:
  explicit ExportMetricsServiceRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kResourceMetricsFieldNumber = 1,
  };
  // repeated .opentelemetry.proto.metrics.v1.ResourceMetrics resource_metrics = 1;
  int resource_metrics_size() const;
  private:
  int _internal_resource_metrics_size() const;
  public:
  void clear_resource_metrics();
  ::opentelemetry::proto::metrics::v1::ResourceMetrics* mutable_resource_metrics(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::opentelemetry::proto::metrics::v1::ResourceMetrics >*
      mutable_resource_metrics();
  private:
  const ::opentelemetry::proto::metrics::v1::ResourceMetrics& _internal_resource_metrics(int index) const;
  ::opentelemetry::proto::metrics::v1::ResourceMetrics* _internal_add_resource_metrics();
  public:
  const ::opentelemetry::proto::metrics::v1::ResourceMetrics& resource_metrics(int index) const;
  ::opentelemetry::proto::metrics::v1::ResourceMetrics* add_resource_metrics();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::opentelemetry::proto::metrics::v1::ResourceMetrics >&
      resource_metrics() const;

  // @@protoc_insertion_point(class_scope:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::opentelemetry::proto::metrics::v1::ResourceMetrics > resource_metrics_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto;
};
// -------------------------------------------------------------------

class ExportMetricsServiceResponse final :
    public ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase /* @@protoc_insertion_point(class_definition:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceResponse) */ {
 public:
  inline ExportMetricsServiceResponse() : ExportMetricsServiceResponse(nullptr) {}
  explicit constexpr ExportMetricsServiceResponse(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ExportMetricsServiceResponse(const ExportMetricsServiceResponse& from);
  ExportMetricsServiceResponse(ExportMetricsServiceResponse&& from) noexcept
    : ExportMetricsServiceResponse() {
    *this = ::std::move(from);
  }

  inline ExportMetricsServiceResponse& operator=(const ExportMetricsServiceResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline ExportMetricsServiceResponse& operator=(ExportMetricsServiceResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ExportMetricsServiceResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const ExportMetricsServiceResponse* internal_default_instance() {
    return reinterpret_cast<const ExportMetricsServiceResponse*>(
               &_ExportMetricsServiceResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(ExportMetricsServiceResponse& a, ExportMetricsServiceResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(ExportMetricsServiceResponse* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ExportMetricsServiceResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ExportMetricsServiceResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ExportMetricsServiceResponse>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyFrom;
  inline void CopyFrom(const ExportMetricsServiceResponse& from) {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyImpl(this, from);
  }
  using ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeFrom;
  void MergeFrom(const ExportMetricsServiceResponse& from) {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeImpl(this, from);
  }
  public:

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceResponse";
  }
  protected:
  explicit ExportMetricsServiceResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceResponse)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ExportMetricsServiceRequest

// repeated .opentelemetry.proto.metrics.v1.ResourceMetrics resource_metrics = 1;
inline int ExportMetricsServiceRequest::_internal_resource_metrics_size() const {
  return resource_metrics_.size();
}
inline int ExportMetricsServiceRequest::resource_metrics_size() const {
  return _internal_resource_metrics_size();
}
inline ::opentelemetry::proto::metrics::v1::ResourceMetrics* ExportMetricsServiceRequest::mutable_resource_metrics(int index) {
  // @@protoc_insertion_point(field_mutable:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest.resource_metrics)
  return resource_metrics_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::opentelemetry::proto::metrics::v1::ResourceMetrics >*
ExportMetricsServiceRequest::mutable_resource_metrics() {
  // @@protoc_insertion_point(field_mutable_list:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest.resource_metrics)
  return &resource_metrics_;
}
inline const ::opentelemetry::proto::metrics::v1::ResourceMetrics& ExportMetricsServiceRequest::_internal_resource_metrics(int index) const {
  return resource_metrics_.Get(index);
}
inline const ::opentelemetry::proto::metrics::v1::ResourceMetrics& ExportMetricsServiceRequest::resource_metrics(int index) const {
  // @@protoc_insertion_point(field_get:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest.resource_metrics)
  return _internal_resource_metrics(index);
}
inline ::opentelemetry::proto::metrics::v1::ResourceMetrics* ExportMetricsServiceRequest::_internal_add_resource_metrics() {
  return resource_metrics_.Add();
}
inline ::opentelemetry::proto::metrics::v1::ResourceMetrics* ExportMetricsServiceRequest::add_resource_metrics() {
  ::opentelemetry::proto::metrics::v1::ResourceMetrics* _add = _internal_add_resource_metrics();
  // @@protoc_insertion_point(field_add:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest.resource_metrics)
  return _add;
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::opentelemetry::proto::metrics::v1::ResourceMetrics >&
ExportMetricsServiceRequest::resource_metrics() const {
  // @@protoc_insertion_point(field_list:opentelemetry.proto.collector.metrics.v1.ExportMetricsServiceRequest.resource_metrics)
  return resource_metrics_;
}

// -------------------------------------------------------------------

// ExportMetricsServiceResponse

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace v1
}  // namespace metrics
}  // namespace collector
}  // namespace proto
}  // namespace opentelemetry

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_opentelemetry_2fproto_2fcollector_2fmetrics_2fv1_2fmetrics_5fservice_2eproto