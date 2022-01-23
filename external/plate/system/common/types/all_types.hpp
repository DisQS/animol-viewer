#pragma once

#include <cstdint>

#include "bfloat.hpp"

#ifdef PLATE

#else

  #include "hfloat.hpp"
  #include "128.hpp"

#endif


#ifdef MULTIPRECISION

  #include <boost/multiprecision/cpp_bin_float.hpp>

#endif


namespace plate {

#ifdef PLATE

  using uint8      = std::uint8_t;
  using uint16     = std::uint16_t;
  using uint32     = std::uint32_t;
  using uint64     = std::uint64_t;

  using uintfast8  = std::uint_fast8_t;
  using uintfast16 = std::uint_fast16_t;
  using uintfast32 = std::uint_fast32_t;
  using uintfast64 = std::uint_fast64_t;

  using int8       = std::int8_t;
  using int16      = std::int16_t;
  using int32      = std::int32_t;
  using int64      = std::int64_t;

  using intfast8   = std::int_fast8_t;
  using intfast16  = std::int_fast16_t;
  using intfast32  = std::int_fast32_t;
  using intfast64  = std::int_fast64_t;

  using float32    = float;
  using float64    = double;

  using bfloat16   = plate::bfloat;

  #ifdef MULTIPRECISION
  using uint128    = boost::multiprecision::uint128_t;
  using uint256    = boost::multiprecision::uint256_t;
  using int128     = boost::multiprecision::int128_t;
  using int256     = boost::multiprecision::int256_t;
  using float128   = boost::multiprecision::cpp_bin_float_quad;
  using float256   = boost::multiprecision::cpp_bin_float_oct;
  #endif

  using uints      = std::size_t;
  using ints       = std::make_signed<uints>::type;

#else

  using uint8      = std::uint8_t;
  using uint16     = std::uint16_t;
  using uint32     = std::uint32_t;
  using uint64     = std::uint64_t;
  using uint128    = unsigned __int128;

  using uintfast8  = std::uint_fast8_t;
  using uintfast16 = std::uint_fast16_t;
  using uintfast32 = std::uint_fast32_t;
  using uintfast64 = std::uint_fast64_t;

  using int8       = std::int8_t;
  using int16      = std::int16_t;
  using int32      = std::int32_t;
  using int64      = std::int64_t;
  using int128     = __int128;

  using intfast8   = std::int_fast8_t;
  using intfast16  = std::int_fast16_t;
  using intfast32  = std::int_fast32_t;
  using intfast64  = std::int_fast64_t;

  using float16    = plate::hfloat;
  using float32    = float;
  using float64    = double;
  using float128   = __float128;

  using bfloat16   = plate::bfloat;

  using uints      = std::size_t;
  using ints       = std::make_signed<uints>::type;

  #ifdef MULTIPRECISION
  using uint256  = boost::multiprecision::uint256_t;
  using int256   = boost::multiprecision::int256_t;
  using float256 = boost::multiprecision::cpp_bin_float_oct;
  #endif

#endif


template<class T>
constexpr std::string_view number_type_string() noexcept
{
  if constexpr(std::is_same_v<T, uint8>)    return "uint8";
  if constexpr(std::is_same_v<T, uint16>)   return "uint16";
  if constexpr(std::is_same_v<T, uint32>)   return "uint32";
  if constexpr(std::is_same_v<T, uint64>)   return "uint64";

  if constexpr(std::is_same_v<T, int8>)     return "int8";
  if constexpr(std::is_same_v<T, int16>)    return "int16";
  if constexpr(std::is_same_v<T, int32>)    return "int32";
  if constexpr(std::is_same_v<T, int64>)    return "int64";

  if constexpr(std::is_same_v<T, float32>)  return "float32";
  if constexpr(std::is_same_v<T, float64>)  return "float64";

  #ifndef PLATE
  if constexpr(std::is_same_v<T, bfloat16>) return "bfloat16";
  if constexpr(std::is_same_v<T, float16>)  return "float16";
  if constexpr(std::is_same_v<T, uint128>)  return "uint128";
  if constexpr(std::is_same_v<T, int128>)   return "int128";
  if constexpr(std::is_same_v<T, float128>) return "float128";
  #endif

  #ifdef MULTIPRECISION
  #ifdef PLATE
  if constexpr(std::is_same_v<T, uint128>)  return "uint128";
  if constexpr(std::is_same_v<T, int128>)   return "int128";
  if constexpr(std::is_same_v<T, float128>) return "float128";
  #endif
  if constexpr(std::is_same_v<T, uint256>)  return "uint256";
  if constexpr(std::is_same_v<T, int256>)   return "int256";
  if constexpr(std::is_same_v<T, float256>) return "float256";
  #endif

  return "unknown";
}

}; // namespace plate
