#pragma once

#include <cstddef>
#include <cstdlib>
#include <span>

#include <GLES3/gl3.h>

#include "pfr.hpp"

/*

  Represents a buffer of memory used by the gpu.

  Includes a staging buffer where necessary to support transfers to the gpu.

  A buffer is typically an array of data to pass to the GPU.

  An example would be:

    struct example
    {
      std::array<float, 4>     pos;
      std::array<std::uint8_t> color;
    };

  for a vertex buffer with colors.

  a buffer<example> would represent an array of the example struct with the ability to allocate and push the data to the GPU.

  Utility functions are also provided to allow for querying of the data types and conversion to WEBGL enum values:

    get_type() and get_size_in_bytes()

  (which are used by webgl functions to push and interpret the supplied data)

*/


namespace plate {


template<class T>
class buffer {

public:

  ~buffer()
  {
    if (id_)
      glDeleteBuffers(1, &id_);
  }


  bool inline is_ready() const noexcept
  {
    return id_ != 0;
  }


  inline auto get_mode() const noexcept
  {
    return mode_;
  }


  T* allocate_staging(int count, int mode = 0) noexcept
  {
    count_ = count;
    mode_  = mode;

    data_.resize(count);

    return data_.data();
  }


  T* map_staging() noexcept
  {
    return data_.data();
  }


  void unmap_staging() noexcept
  {
  }


  bool upload() noexcept
  {
    if (data_.empty() || data_.size() != count_)
      return false;

    return upload(data_.data(), count_, mode_);
  }


  bool upload(const std::byte* data, int count, int mode = GL_TRIANGLES) noexcept
  {
    return upload(reinterpret_cast<const T*>(data), count, mode);
  }


  inline bool upload(std::span<T> data, int mode = GL_TRIANGLES) noexcept
  {
    return upload(data.data(), data.size(), mode);
  }


  bool upload(const T* data, int count, int mode = GL_TRIANGLES) noexcept
  {
    if (!id_)
      glGenBuffers(1, &id_);

    count_ = count;
    mode_  = mode;

    glBindBuffer(GL_ARRAY_BUFFER, id_);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(T), data, GL_STATIC_DRAW);

    return true;
  }


  void free_staging() noexcept
  {
    data_.clear();
  }


  // get the webgl type of the struct entry at position POS
  //
  //  eg: if T is struct x { std::array<float, 4> pos, std::array<std::uint8_t,4> col }
  //
  //    for POS == 0, return GL_FLOAT and POS == 1 returns GL_UNSIGNED_BYTE

  template<int POS>
  constexpr int get_type() const noexcept
  {
    using t = typename pfr::tuple_element_t<POS, T>::value_type;

    if constexpr (std::is_same_v<float,          t>) return GL_FLOAT;
    if constexpr (std::is_same_v<int,            t>) return GL_INT;
    if constexpr (std::is_same_v<unsigned int,   t>) return GL_UNSIGNED_INT;
    if constexpr (std::is_same_v<short,          t>) return GL_SHORT;
    if constexpr (std::is_same_v<unsigned short, t>) return GL_UNSIGNED_SHORT;
    if constexpr (std::is_same_v<char,           t>) return GL_BYTE;
    if constexpr (std::is_same_v<unsigned char,  t>) return GL_UNSIGNED_BYTE;

    return 0;
  }


  // get the size in bytes of the struct entry at position POS

  template<int POS>
  constexpr int get_size_in_bytes() const noexcept
  {
    using t = pfr::tuple_element_t<POS, T>;

    return sizeof(t);
  }


  // get the size in bytes of the struct up to entry at position POS

  template<int POS>
  constexpr int get_size_in_bytes_up_to() const noexcept
  {
    if constexpr (POS == 0)
      return 0;
    else
      return get_size_in_bytes<POS - 1>() + get_size_in_bytes_up_to<POS - 1>();
  }


  int count_{0};

  unsigned int id_{0};

  std::vector<T> data_;

  int mode_{0};
};
 
} // namespace plate
