#include <cassert>
#include <cstring>
#include <stdexcept>
#include <boost/crc.hpp>
#include "sst/codec.h"

using namespace std;
namespace zs = zero_switch;

// from leveldb

namespace {

constexpr const auto is_little_endian = []{
    std::uint32_t v = 1;
    return reinterpret_cast<char*>(&v)[0] == 1;
};

void EncodeFixed32(char* buf, uint32_t value) {
  if (is_little_endian) {
    memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
  }
}

void EncodeFixed64(char* buf, uint64_t value) {
  if (is_little_endian) {
    memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
    buf[4] = (value >> 32) & 0xff;
    buf[5] = (value >> 40) & 0xff;
    buf[6] = (value >> 48) & 0xff;
    buf[7] = (value >> 56) & 0xff;
  }
}

char* EncodeVarint32(char* dst, uint32_t v) {
  // Operate on characters as unsigneds
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  static const int B = 128;
  if (v < (1<<7)) {
    *(ptr++) = v;
  } else if (v < (1<<14)) {
    *(ptr++) = v | B;
    *(ptr++) = v>>7;
  } else if (v < (1<<21)) {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = v>>14;
  } else if (v < (1<<28)) {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = (v>>14) | B;
    *(ptr++) = v>>21;
  } else {
    *(ptr++) = v | B;
    *(ptr++) = (v>>7) | B;
    *(ptr++) = (v>>14) | B;
    *(ptr++) = (v>>21) | B;
    *(ptr++) = v>>28;
  }
  return reinterpret_cast<char*>(ptr);
}

char* EncodeVarint64(char* dst, uint64_t v) {
  static const int B = 128;
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  while (v >= B) {
    *(ptr++) = (v & (B-1)) | B;
    v >>= 7;
  }
  *(ptr++) = static_cast<unsigned char>(v);
  return reinterpret_cast<char*>(ptr);
}

const char* GetVarint32PtrFallback(const char* p,
                                   const char* limit,
                                   uint32_t* value) {
  uint32_t result = 0;
  for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
    uint32_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

const char* GetVarint32Ptr(const char* p,
                                  const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const unsigned char*>(p));
    if ((result & 128) == 0) {
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}

const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value) {
  uint64_t result = 0;
  for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
    uint64_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

uint32_t DecodeFixed32(const char* ptr) {
  if (is_little_endian) {
    // Load the raw bytes
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
  } else {
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
}

}

void zs::encode_variant32(std::string& buffer, const std::uint32_t val)
{
    char buf[5];
    char* ptr = EncodeVarint32(buf, val);
    buffer.append(buf, ptr - buf);
}

void zs::encode_fix32(std::string& buffer, const std::uint32_t val)
{
    char buf[4];
    EncodeFixed32(buf, val);
    buffer.append(buf, sizeof(buf));
}

void zs::encode_variant64(std::string& buffer, std::uint64_t val)
{
    char buf[8];
    char* ptr = EncodeVarint64(buf, val);
    buffer.append(buf, ptr - buf);
}

void zs::encode_fix64(std::string& buffer, std::uint64_t val)
{
    char buf[8];
    EncodeFixed64(buf, val);
    buffer.append(buf, sizeof(buf));
}

std::uint32_t zs::decode_fix32(std::string_view& buffer)
{
    assert(buffer.size() == sizeof(std::uint32_t));

    return DecodeFixed32(buffer.data());
}

std::uint64_t zs::decode_variant64(std::string_view& buffer)
{
    std::uint64_t value = 0;
    const char* beg = buffer.begin();
    const char* end = buffer.end();
    const char* p = GetVarint64Ptr(beg, end, &value);
    if (!p) {
        throw std::invalid_argument{"bad variant 64"};
    }

    buffer = std::string_view(p, end - p);
    return value;
}

std::uint32_t zs::crc32(const std::string_view buffer)
{
    boost::crc_32_type  result;
    result.process_bytes(buffer.data(), buffer.size());
    return static_cast<std::uint32_t>(result.checksum());
}
