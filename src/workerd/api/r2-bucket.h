// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include "r2-rpc.h"

#include <workerd/jsg/jsg.h>
#include <workerd/api/http.h>
#include <workerd/api/r2-api.capnp.h>
#include <capnp/compat/json.h>
#include <workerd/util/http-util.h>

namespace workerd::api::public_beta {

class R2MultipartUpload;

class R2Bucket: public jsg::Object {
  // A capability to an R2 Bucket.

protected:
  struct friend_tag_t {};

  struct FeatureFlags {
    FeatureFlags(CompatibilityFlags::Reader featureFlags);

    bool listHonorsIncludes;
  };

public:
  explicit R2Bucket(CompatibilityFlags::Reader featureFlags, uint clientIndex)
      : featureFlags(featureFlags), clientIndex(clientIndex) {}
  // `clientIndex` is what to pass to IoContext::getHttpClient() to get an HttpClient
  // representing this namespace.

  explicit R2Bucket(FeatureFlags featureFlags, uint clientIndex, kj::String bucket, friend_tag_t)
      : featureFlags(featureFlags), clientIndex(clientIndex), adminBucket(kj::mv(bucket)) {}

  struct Range {
    jsg::Optional<double> offset;
    jsg::Optional<double> length;
    jsg::Optional<double> suffix;

    JSG_STRUCT(offset, length, suffix);
  };

  struct Conditional {
    jsg::Optional<jsg::NonCoercible<kj::String>> etagMatches;
    jsg::Optional<jsg::NonCoercible<kj::String>> etagDoesNotMatch;
    jsg::Optional<kj::Date> uploadedBefore;
    jsg::Optional<kj::Date> uploadedAfter;
    jsg::Optional<bool> secondsGranularity;

    JSG_STRUCT(etagMatches, etagDoesNotMatch, uploadedBefore, uploadedAfter, secondsGranularity);
  };

  struct GetOptions {
    jsg::Optional<kj::OneOf<Conditional, jsg::Ref<Headers>>> onlyIf;
    jsg::Optional<kj::OneOf<Range, jsg::Ref<Headers>>> range;

    JSG_STRUCT(onlyIf, range);
  };

  struct Checksums {
    jsg::Optional<kj::Array<kj::byte>> md5;
    jsg::Optional<kj::Array<kj::byte>> sha1;
    jsg::Optional<kj::Array<kj::byte>> sha256;
    jsg::Optional<kj::Array<kj::byte>> sha384;
    jsg::Optional<kj::Array<kj::byte>> sha512;

    JSG_STRUCT(md5, sha1, sha256, sha384, sha512);

    Checksums clone() const;
  };

  struct HttpMetadata {
    static HttpMetadata fromRequestHeaders(jsg::Lock& js, Headers& h);

    jsg::Optional<kj::String> contentType;
    jsg::Optional<kj::String> contentLanguage;
    jsg::Optional<kj::String> contentDisposition;
    jsg::Optional<kj::String> contentEncoding;
    jsg::Optional<kj::String> cacheControl;
    jsg::Optional<kj::Date> cacheExpiry;

    JSG_STRUCT(contentType, contentLanguage, contentDisposition,
                contentEncoding, cacheControl, cacheExpiry);

    HttpMetadata clone() const;
  };

  struct PutOptions {
    jsg::Optional<kj::OneOf<Conditional, jsg::Ref<Headers>>> onlyIf;
    jsg::Optional<kj::OneOf<HttpMetadata, jsg::Ref<Headers>>> httpMetadata;
    jsg::Optional<jsg::Dict<kj::String>> customMetadata;
    jsg::Optional<kj::OneOf<kj::Array<kj::byte>, jsg::NonCoercible<kj::String>>> md5;
    jsg::Optional<kj::OneOf<kj::Array<kj::byte>, jsg::NonCoercible<kj::String>>> sha1;
    jsg::Optional<kj::OneOf<kj::Array<kj::byte>, jsg::NonCoercible<kj::String>>> sha256;
    jsg::Optional<kj::OneOf<kj::Array<kj::byte>, jsg::NonCoercible<kj::String>>> sha384;
    jsg::Optional<kj::OneOf<kj::Array<kj::byte>, jsg::NonCoercible<kj::String>>> sha512;

    JSG_STRUCT(onlyIf, httpMetadata, customMetadata, md5, sha1, sha256, sha384, sha512);
  };

  struct MultipartOptions {
    jsg::Optional<kj::OneOf<HttpMetadata, jsg::Ref<Headers>>> httpMetadata;
    jsg::Optional<jsg::Dict<kj::String>> customMetadata;

    JSG_STRUCT(httpMetadata, customMetadata);
  };

  struct UploadedPart {
    int partNumber;
    kj::String etag;

    JSG_STRUCT(partNumber, etag);
  };

  class HeadResult: public jsg::Object {
  public:
    HeadResult(kj::String name, kj::String version, double size,
               kj::String etag, Checksums checksums, kj::Date uploaded, jsg::Optional<HttpMetadata> httpMetadata,
               jsg::Optional<jsg::Dict<kj::String>> customMetadata, jsg::Optional<Range> range):
        name(kj::mv(name)), version(kj::mv(version)), size(size), etag(kj::mv(etag)),
        checksums(kj::mv(checksums)), uploaded(uploaded), httpMetadata(kj::mv(httpMetadata)),
        customMetadata(kj::mv(customMetadata)), range(kj::mv(range)) {}

    kj::String getName() const { return kj::str(name); }
    kj::String getVersion() const { return kj::str(version); }
    double getSize() const { return size; }
    kj::String getEtag() const { return kj::str(etag); }
    kj::String getHttpEtag() const { return kj::str('"', etag, '"'); }
    Checksums getChecksums() const { return checksums.clone(); }
    kj::Date getUploaded() const { return uploaded; }

    jsg::Optional<HttpMetadata> getHttpMetadata() const {
      return httpMetadata.map([](const HttpMetadata& m) { return m.clone(); });
    }

    const jsg::Optional<jsg::Dict<kj::String>> getCustomMetadata() const {
      return customMetadata.map([](const jsg::Dict<kj::String>& m) {
        return jsg::Dict<kj::String>{
          .fields = KJ_MAP(f, m.fields) {
            return jsg::Dict<kj::String>::Field{
              .name = kj::str(f.name), .value = kj::str(f.value)
            };
          },
        };
      });
    }


    jsg::Optional<Range> getRange() { return range; }

    void writeHttpMetadata(jsg::Lock& js, Headers& headers);

    JSG_RESOURCE_TYPE(HeadResult) {
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(key, getName);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(version, getVersion);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(size, getSize);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(etag, getEtag);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(httpEtag, getHttpEtag);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(checksums, getChecksums);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(uploaded, getUploaded);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(httpMetadata, getHttpMetadata);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(customMetadata, getCustomMetadata);
      JSG_LAZY_READONLY_INSTANCE_PROPERTY(range, getRange);
      JSG_METHOD(writeHttpMetadata);
    }

  protected:
    kj::String name;
    kj::String version;
    double size;
    kj::String etag;
    Checksums checksums;
    kj::Date uploaded;
    jsg::Optional<HttpMetadata> httpMetadata;
    jsg::Optional<jsg::Dict<kj::String>> customMetadata;

    jsg::Optional<Range> range;
    friend class R2Bucket;
  };

  class GetResult: public HeadResult {
  public:
    GetResult(kj::String name, kj::String version, double size,
              kj::String etag, Checksums checksums, kj::Date uploaded, jsg::Optional<HttpMetadata> httpMetadata,
              jsg::Optional<jsg::Dict<kj::String>> customMetadata, jsg::Optional<Range> range,
              jsg::Ref<ReadableStream> body)
      : HeadResult(
          kj::mv(name), kj::mv(version), size, kj::mv(etag), kj::mv(checksums), uploaded,
          kj::mv(KJ_ASSERT_NONNULL(httpMetadata)), kj::mv(KJ_ASSERT_NONNULL(customMetadata)), range),
          body(kj::mv(body)) {}

    jsg::Ref<ReadableStream> getBody() {
      return body.addRef();
    }

    bool getBodyUsed() {
      return body->isDisturbed();
    }

    jsg::Promise<kj::Array<kj::byte>> arrayBuffer(jsg::Lock& js);
    jsg::Promise<kj::String> text(jsg::Lock& js);
    jsg::Promise<jsg::Value> json(jsg::Lock& js);
    jsg::Promise<jsg::Ref<Blob>> blob(jsg::Lock& js);

    JSG_RESOURCE_TYPE(GetResult) {
      JSG_INHERIT(HeadResult);
      JSG_READONLY_PROTOTYPE_PROPERTY(body, getBody);
      JSG_READONLY_PROTOTYPE_PROPERTY(bodyUsed, getBodyUsed);
      JSG_METHOD(arrayBuffer);
      JSG_METHOD(text);
      JSG_METHOD(json);
      JSG_METHOD(blob);
    }
  private:
    jsg::Ref<ReadableStream> body;
  };

  struct ListResult {
    kj::Array<jsg::Ref<HeadResult>> objects;
    bool truncated;
    jsg::Optional<kj::String> cursor;
    kj::Array<kj::String> delimitedPrefixes;

    JSG_STRUCT(objects, truncated, cursor, delimitedPrefixes);
  };

  struct ListOptions {
    jsg::Optional<int> limit;
    jsg::Optional<jsg::NonCoercible<kj::String>> prefix;
    jsg::Optional<jsg::NonCoercible<kj::String>> cursor;
    jsg::Optional<jsg::NonCoercible<kj::String>> delimiter;
    jsg::Optional<jsg::NonCoercible<kj::String>> startAfter;
    jsg::Optional<kj::Array<jsg::NonCoercible<kj::String>>> include;

    JSG_STRUCT(limit, prefix, cursor, delimiter, startAfter, include);
  };

  jsg::Promise<kj::Maybe<jsg::Ref<HeadResult>>> head(jsg::Lock& js, kj::String key,
      const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType);

  jsg::Promise<kj::OneOf<kj::Maybe<jsg::Ref<GetResult>>, jsg::Ref<HeadResult>>> get(
      jsg::Lock& js, kj::String key, jsg::Optional<GetOptions> options,
      const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType);
  jsg::Promise<kj::Maybe<jsg::Ref<HeadResult>>> put(jsg::Lock& js,
      kj::String key, kj::Maybe<R2PutValue> value, jsg::Optional<PutOptions> options,
      const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType);
  jsg::Promise<jsg::Ref<R2MultipartUpload>> createMultipartUpload(
    jsg::Lock& js,
    kj::String key,
    jsg::Optional<MultipartOptions> options,
    const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType
  );
  jsg::Promise<void> delete_(jsg::Lock& js, kj::OneOf<kj::String, kj::Array<kj::String>> keys,
      const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType);
  jsg::Promise<ListResult> list(jsg::Lock& js, jsg::Optional<ListOptions> options,
      const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType);

  JSG_RESOURCE_TYPE(R2Bucket) {
    JSG_METHOD(head);
    JSG_METHOD(get);
    JSG_METHOD(put);
    JSG_METHOD(createMultipartUpload);
    JSG_METHOD_NAMED(delete, delete_);
    JSG_METHOD(list);
  }

  struct UnwrappedConditional {
    UnwrappedConditional(jsg::Lock& js, Headers& h);
    UnwrappedConditional(const Conditional& c);

    kj::Maybe<kj::String> etagMatches;
    kj::Maybe<kj::String> etagDoesNotMatch;
    kj::Maybe<kj::Date> uploadedBefore;
    kj::Maybe<kj::Date> uploadedAfter;
    bool secondsGranularity = false;
  };

protected:
  kj::Maybe<kj::StringPtr> adminBucketName() const {
    return adminBucket;
  }

private:
  FeatureFlags featureFlags;
  uint clientIndex;
  kj::Maybe<kj::String> adminBucket;

  friend class R2Admin;
  friend class R2MultipartUpload;
};

enum class OptionalMetadata: uint16_t {
  Http = static_cast<uint8_t>(R2ListRequest::IncludeField::HTTP),
  Custom = static_cast<uint8_t>(R2ListRequest::IncludeField::CUSTOM),
};

template <typename T>
concept HeadResultT = std::is_base_of_v<R2Bucket::HeadResult, T>;

template <HeadResultT T, typename... Args>
static jsg::Ref<T> parseObjectMetadata(R2HeadResponse::Reader responseReader,
    kj::ArrayPtr<const OptionalMetadata> expectedOptionalFields, Args&&... args) {
  // optionalFieldsExpected is initialized by default to HTTP + CUSTOM if the user doesn't specify
  // anything. If they specify the empty array, then nothing is returned.
  kj::Date uploaded =
      kj::UNIX_EPOCH + responseReader.getUploadedMillisecondsSinceEpoch() * kj::MILLISECONDS;

  jsg::Optional<R2Bucket::HttpMetadata> httpMetadata;
  if (responseReader.hasHttpFields()) {
    R2Bucket::HttpMetadata m;

    auto httpFields = responseReader.getHttpFields();
    if (httpFields.hasContentType()) {
      m.contentType = kj::str(httpFields.getContentType());
    }
    if (httpFields.hasContentDisposition()) {
      m.contentDisposition = kj::str(httpFields.getContentDisposition());
    }
    if (httpFields.hasContentEncoding()) {
      m.contentEncoding = kj::str(httpFields.getContentEncoding());
    }
    if (httpFields.hasContentLanguage()) {
      m.contentLanguage = kj::str(httpFields.getContentLanguage());
    }
    if (httpFields.hasCacheControl()) {
      m.cacheControl = kj::str(httpFields.getCacheControl());
    }
    if (httpFields.getCacheExpiry() != 0xffffffffffffffff) {
      m.cacheExpiry =
          kj::UNIX_EPOCH + httpFields.getCacheExpiry() * kj::MILLISECONDS;
    }

    httpMetadata = kj::mv(m);
  } else if (std::find(expectedOptionalFields.begin(), expectedOptionalFields.end(),
      OptionalMetadata::Http) != expectedOptionalFields.end()) {
    // HTTP metadata was asked for but the object didn't have anything.
    httpMetadata = R2Bucket::HttpMetadata{};
  }

  jsg::Optional<jsg::Dict<kj::String>> customMetadata;
  if (responseReader.hasCustomFields()) {
    customMetadata = jsg::Dict<kj::String> {
      .fields = KJ_MAP(field, responseReader.getCustomFields()) {
        jsg::Dict<kj::String>::Field item;
        item.name = kj::str(field.getK());
        item.value = kj::str(field.getV());
        return item;
      }
    };
  } else if (std::find(expectedOptionalFields.begin(), expectedOptionalFields.end(),
      OptionalMetadata::Custom) != expectedOptionalFields.end()) {
    // Custom metadata was asked for but the object didn't have anything.
    customMetadata = jsg::Dict<kj::String>{};
  }

  jsg::Optional<R2Bucket::Range> range;

  if (responseReader.hasRange()) {
    auto rangeBuilder = responseReader.getRange();
    range = R2Bucket::Range {
      .offset = static_cast<double>(rangeBuilder.getOffset()),
      .length = static_cast<double>(rangeBuilder.getLength()),
    };
  }

  R2Bucket::Checksums checksums;

  if (responseReader.hasChecksums()) {
    R2Checksums::Reader checksumsBuilder = responseReader.getChecksums();
    if (checksumsBuilder.hasMd5()) {
      checksums.md5 = kj::heapArray(checksumsBuilder.getMd5());
    }
    if (checksumsBuilder.hasSha1()) {
      checksums.sha1 = kj::heapArray(checksumsBuilder.getSha1());
    }
    if (checksumsBuilder.hasSha256()) {
      checksums.sha256 = kj::heapArray(checksumsBuilder.getSha256());
    }
    if (checksumsBuilder.hasSha384()) {
      checksums.sha384 = kj::heapArray(checksumsBuilder.getSha384());
    }
    if (checksumsBuilder.hasSha512()) {
      checksums.sha512 = kj::heapArray(checksumsBuilder.getSha512());
    }
  }

  return jsg::alloc<T>(kj::str(responseReader.getName()),
      kj::str(responseReader.getVersion()), responseReader.getSize(),
      kj::str(responseReader.getEtag()), kj::mv(checksums), uploaded, kj::mv(httpMetadata),
      kj::mv(customMetadata), range, kj::fwd<Args>(args)...);
}

template <HeadResultT T, typename... Args>
static kj::Maybe<jsg::Ref<T>> parseObjectMetadata(kj::StringPtr action, R2Result& r2Result,
    const jsg::TypeHandler<jsg::Ref<R2Error>>& errorType, Args&&... args) {
  if (r2Result.objectNotFound()) {
    return nullptr;
  }
  if (!r2Result.preconditionFailed()) {
    r2Result.throwIfError(action, errorType);
  }

  // Non-list operations always return these.
  std::array expectedFieldsOwned = { OptionalMetadata::Http, OptionalMetadata::Custom };
  kj::ArrayPtr<OptionalMetadata> expectedFields = {
    expectedFieldsOwned.data(), expectedFieldsOwned.size()
  };

  capnp::MallocMessageBuilder responseMessage;
  capnp::JsonCodec json;
  // Annoyingly our R2GetResponse alias isn't emitted.
  json.handleByAnnotation<R2HeadResponse>();
  auto responseBuilder = responseMessage.initRoot<R2HeadResponse>();
  json.decode(KJ_ASSERT_NONNULL(r2Result.metadataPayload), responseBuilder);

  return parseObjectMetadata<T>(responseBuilder, expectedFields, kj::fwd<Args>(args)...);
}

}  // namespace workerd::api::public_beta

