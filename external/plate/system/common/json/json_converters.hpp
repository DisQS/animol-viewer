#pragma once


#include <string>
#include <string_view>
#include <optional>
#include <map>
#include <charconv>
#include <cmath>
#include <span>
#include <sstream>

#include "pfr.hpp"
#include "magic_enum.hpp"
#include "fast_float/fast_float.h"

#include "../types/all_types.hpp"

#ifndef PLATE
  #include <arpa/inet.h>

  #include <quadmath.h>

  #include "../time/time.hpp"
  #include "../time/zone.hpp"

  #include "../uuid/uuid.hpp"
#endif

/*
    For converting from json types: string, number, true and false

    to basic types:

      int[8-128], uints[8-128] floats[16-128], bfloats[16] and string and string_view

    and common types:

      uuid, time::point, zone_t, sockaddr_storage

    interface is either:

      UT => user_type   uv => user_value
      JT => json type   jv => json_value

      std::optional<UT> convert<UT>(const auto& jv)

      or

      bool convert(auto& uv, const auto& jv)
*/


namespace plate {

  // this template is specialised for all the types we want
  //
  // includes enum support

  template<class UT, class JT>
  std::optional<UT> convert( [[maybe_unused]] const JT& s) noexcept
  {
    if constexpr(std::is_enum<UT>() && std::is_same_v<std::string_view, JT>)
    {
      auto uv = magic_enum::enum_cast<UT>(s);

      if (uv.has_value())
        return { uv.value() };
      else
        return std::nullopt;
    }
    else
      return std::nullopt;
  }

  // this template provides the alternative get form of the above get

  template<class UT>
  bool convert(UT& v, const auto& s)
  {
    if (auto temp = convert<UT>(s))
    {
      v = std::move(*temp);

      return true;
    }
    else
      return false;
  }


  // enums

//  template<class E, std::enable_if_t<std::is_enum<E>::value>>
//  std::optional<E> convert<E>(const std::string_view& s) noexcept
//  {
//    log_debug("got an enum");
//    return { (E)0 };
//  }

  // string_view

  template<>
  std::optional<std::string_view> convert<std::string_view>(const std::string_view& s) noexcept
  {
    return {s};
  }

  // string

  template<>
  std::optional<std::string> convert<std::string>(const std::string_view& s) noexcept
  {
    return {std::string(s)};
  }


  #ifndef PLATE

  // std::vector<std::byte>

  template<>
  std::optional<std::vector<std::byte>> convert<std::vector<std::byte>>(const std::string_view& s) noexcept
  {
    // decode from base64

    std::size_t base64_len;
    std::vector<std::byte> d(s.size());

    if (base64_decode(s.data(), s.size(), reinterpret_cast<char*>(d.data()), &base64_len, 0) != 1)
      return std::nullopt;

    d.resize(base64_len);

    return {std::move(d)};
  }


  // uuid


  template<>
  std::optional<uuid> convert<uuid>(const std::string_view& s) noexcept // from base64 encoded string
  {
    return uuid::uuid_from_base64(s);
  }


  // iso date time


  template<>
  std::optional<time::point> convert<time::point>(const std::string_view& s) noexcept
  {
    time::date d;

    if (d.set_to_string(s))
      return { d };
    else
      return std::nullopt;
  }


  template<>
  std::optional<time::packed_point> convert<time::packed_point>(const std::string_view& s) noexcept
  {
    time::date d;

    if (d.set_to_string(s))
      return { time::packed_point{ d } };
    else
      return std::nullopt;
  }


  template<>
  std::optional<zone_t> convert<zone_t>(const std::string_view& s) noexcept
  {
    zone_t r;

    if (auto it = zone::id.find(s); it != zone::id.end())
    {
      std::memcpy(&r, &(it->second), sizeof(r));
      return { r };
    }

    return std::nullopt;
  }


  // ip address


  template<>
  std::optional<sockaddr_storage> convert<sockaddr_storage>(const std::string_view& s) noexcept
  {
    // form is either ipv4.add:port or [ipv6::addr]:port

    sockaddr_storage ss;

    if (s.size() > 256)
      return std::nullopt;

    // find ip addres : port seperator

    auto sep = s.find_last_of(':');

    if (sep == std::string_view::npos)
      return std::nullopt;

    // copy address part into a temp null terminated string

    char temp[256];

    if (s[0] == '[') // ipv6
    {
      if (s[sep-1] != ']')
        return std::nullopt;

      std::memcpy(temp, &s[1], sep-2);
      temp[sep-1] = 0;

      sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(&ss);

      if (inet_pton(AF_INET6, temp, &(ipv6->sin6_addr)) != 1)
        return std::nullopt;

      ss.ss_family = AF_INET6;

      if (!set(s.substr(sep+1), ipv6->sin6_port))
        return std::nullopt;

      ipv6->sin6_port = htons(ipv6->sin6_port);
    }
    else // ipv4
    {
      std::memcpy(temp, &s[0], sep);
      temp[sep] = 0;

      sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(&ss);

      if (inet_pton(AF_INET, temp, &(ipv4->sin_addr)) != 1)
        return std::nullopt;

      ss.ss_family = AF_INET;

      if (!set(s.substr(sep+1), ipv4->sin_port))
        return std::nullopt;

      ipv4->sin_port = htons(ipv4->sin_port);
    }

    return {ss};
  }
  #endif



  // bool


  template<>
  std::optional<bool> convert<bool>(const bool& s) noexcept
  {
    return { s };
  }


  template<>
  std::optional<bool> convert<bool>(const std::string_view& s) noexcept
  {
    if (s == "false" || s == "0")
      return { false };

    if (s == "true"  || s == "1")
      return { true };

    return std::nullopt;
  }


  // integer


  #ifndef PLATE
  template<>
  std::optional<__int128_t> convert<__int128_t>(const std::string_view& s) noexcept
  {
    // FIXME only parses up to int64..!

    int64 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }
  #endif


  template<>
  std::optional<int64> convert<int64>(const std::string_view& s) noexcept
  {
    int64 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }


  template<>
  std::optional<int32> convert<int32>(const std::string_view& s) noexcept
  {
    int32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }


  template<>
  std::optional<int16> convert<int16>(const std::string_view& s) noexcept
  {
    int32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end() || r > 32'767 || r < -32'768)
      return std::nullopt;
    else
      return { static_cast<int16>(r) };
  }


  template<>
  std::optional<int8> convert<int8>(const std::string_view& s) noexcept
  {
    int32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end() || r > 127 || r < -128)
      return std::nullopt;
    else
      return { static_cast<int8>(r) };
  }


  // unsigned integer

 
  #ifndef PLATE
  template<>
  std::optional<unsigned __int128> convert<unsigned __int128>(const std::string_view& s) noexcept
  {
    unsigned __int128 r = 0;

    auto sc = s;

    if (sc.size() > 19) // probably bigger than a uint64_t
    {
      uint64 r1;

      if (auto [p, ec] = std::from_chars(sc.begin(), sc.begin() + 19, r1); ec != std::errc())
        return std::nullopt;
      
      r = static_cast<unsigned __int128>(r1) * std::pow(10, (sc.size() - 19));

      sc = sc.substr(19);
    }

    uint64 r2;

    if (auto [p, ec] = std::from_chars(sc.begin(), sc.begin(), r2); ec != std::errc())
      return std::nullopt;

    r += r2;

    return {r};
  }
  #endif


  template<>
  std::optional<uint64> convert<uint64>(const std::string_view& s) noexcept
  {
    uint64 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }


  template<>
  std::optional<uint32> convert<uint32>(const std::string_view& s) noexcept
  {
    uint32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return {r};
  }


  template<>
  std::optional<uint16> convert<uint16>(const std::string_view& s) noexcept
  {
    uint32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end() || r > 65'535)
      return std::nullopt;
    else
      return { static_cast<uint16>(r) };
  }


  template<>
  std::optional<uint8> convert<uint8>(const std::string_view& s) noexcept
  {
    uint32 r;
    if (auto [p, ec] = std::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end() || r > 255)
      return std::nullopt;
    else
      return { static_cast<uint8>(r) };
  }


  // float


  #ifndef PLATE
  template<>
  std::optional<float128> convert<float128>(const std::string_view& s) noexcept
  {
    float128 r;
    char* e;

    std::string ss(s);

    r = strtoflt128(ss.c_str(), &e);

    if ((e - ss.data()) == static_cast<int32>(ss.size()))
      return {r};

    return std::nullopt;
  }
  #endif


  template<>
  std::optional<float64> convert<float64>(const std::string_view& s) noexcept
  {
    float64 r;
    if (auto [p, ec] = fast_float::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }


  template<>
  std::optional<float32> convert<float32>(const std::string_view& s) noexcept
  {
    float64 r;
    if (auto [p, ec] = fast_float::from_chars(s.begin(), s.end(), r); ec != std::errc() || p != s.end())
      return std::nullopt;
    else
      return { r };
  }


  template<>
  std::optional<bfloat16> convert<bfloat16>(const std::string_view& s) noexcept
  {
    if (auto t = convert<float32>(s); t)
    {
      bfloat16 r = *t;
      return {r};
    }

    return std::nullopt;
  }


  #ifndef PLATE
  template<>
  std::optional<float16> convert<float16>(const std::string_view& s) noexcept
  {
    if (auto t = convert<float32>(s); t)
    {
      float16 r = *t;
      return {r};
    }

    return std::nullopt;
  }
  #endif


  // extract integer from float64


  bool is_whole(const float64& d) noexcept
  {
    return (d == ceil(d));
  }


  template<>
  std::optional<int64> convert<int64>(const float64& d) noexcept
  {
    if (!is_whole(d))
      return std::nullopt;
    return { static_cast<int64>(d) };
  }


  template<>
  std::optional<int32> convert<int32>(const float64& d) noexcept
  {
    if (!is_whole(d) || d < -2'147'483'648 || d > 2'147'483'647)
      return std::nullopt;
    return { static_cast<int32>(d) };
  }


  template<>
  std::optional<int16> convert<int16>(const float64& d) noexcept
  {
    if (!is_whole(d) || d < -32'768 || d > 32'767)
      return std::nullopt;
    return { static_cast<int16>(d) };
  }


  template<>
  std::optional<int8> convert<int8>(const float64& d) noexcept
  {
    if (!is_whole(d) || d < -128 || d > 127)
      return std::nullopt;
    return { static_cast<int8>(d) };
  }


  // extract unsigned integer from float64


  template<>
  std::optional<uint64> convert<uint64>(const float64& d) noexcept
  {
    if (!is_whole(d))
      return std::nullopt;
    return { static_cast<uint64>(d) };
  }


  template<>
  std::optional<uint32> convert<uint32>(const float64& d) noexcept
  {
    if (!is_whole(d) || d > 4'294'967'295)
      return std::nullopt;
    return { static_cast<uint32>(d) };
  }


  template<>
  std::optional<uint16> convert<uint16>(const float64& d) noexcept
  {
    if (!is_whole(d) || d > 65'535)
      return std::nullopt;
    else
      return { static_cast<uint16>(d) };
  }


  template<>
  std::optional<uint8> convert<uint8>(const float64& d) noexcept
  {
    if (!is_whole(d) || d > 255)
      return std::nullopt;
    else
      return { static_cast<uint8>(d) };
  }


  // extract float from float64


  template<>
  std::optional<float64> convert<float64>(const float64& d) noexcept
  {
    float64 f = d;
    return {f};
  }


  template<>
  std::optional<float32> convert<float32>(const float64& d) noexcept
  {
    float32 f = d;
    return {f};
  }


  #ifndef PLATE
  template<>
  std::optional<float16> convert<float16>(const float64& d) noexcept
  {
    float16 h = d;
    return {h};
  }
  #endif


  template<>
  std::optional<bfloat16> convert<bfloat16>(const float64& d) noexcept
  {
    bfloat16 b = d;
    return {b};
  }


  template<>
  std::optional<bool> convert<bool>(const float64& d) noexcept
  {
    return {d != 0.0};
  }


} // namespace plate
