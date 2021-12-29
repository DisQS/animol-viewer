#pragma once

#include <vector>
#include <span>
#include <cstdint>
  
namespace plate {

class data_store
{

public:

  enum class FREE { Yes, No };

  // constructores

  // empty

  data_store()
  {
  };


  // move

  data_store(data_store&& other)
  {
    raw      = other.raw;
    to_free  = other.to_free;
    free_cb_ = std::move(other.free_cb_);
    ptr      = other.ptr;
    v        = std::move(other.v);
    offset   = other.offset;
    sz       = other.sz;

    other.ptr     = nullptr;
    other.raw     = false;
    other.to_free = false;
  };


  // reference

  data_store(data_store& other)
  {
    raw      = other.raw;
    to_free  = other.to_free;
    free_cb_ = other.free_cb_;
    ptr      = other.ptr;
    v        = std::move(other.v);
    offset   = other.offset;
    sz       = other.sz;

    other.ptr      = nullptr;
    other.raw      = false;
    other.to_free  = false;
    other.free_cb_ = nullptr;
  };


  // vector move

  data_store(std::vector<std::byte>&& in_v) :
    v(std::move(in_v)),
    sz(v.size())
  {
  };


  // vector move with an offset

  data_store(std::vector<std::byte>&& in_v, std::uint32_t offset) :
    v(std::move(in_v)),
    offset(offset),
    sz(v.size() - offset)
  {
  };


  // vector move with an offset and a size to use (ie. ignore size of vector as not all used)

  data_store(std::vector<std::byte>&& in_v, std::uint32_t offset, std::uint32_t sz_v) : // sz_v is size of bytes used in_v
    v(std::move(in_v)),
    offset(offset),
    sz(sz_v - offset)
  {
  };


  // span (non-owning)

  data_store(std::span<std::byte> s) :
    raw(s.data()),
    sz(s.size())
  {
  };


  // raw pointer

  data_store(std::byte* ptr, std::uint32_t sz, FREE i_to_free) :
    raw(true),
    ptr(ptr),
    sz(sz)
  {
    if (i_to_free == FREE::Yes) to_free = true;
  };


  // raw pointer with a function to call to free data

  data_store(std::byte* ptr, std::uint32_t sz, std::function< void ()> free_cb) :
    raw(true),
    to_free(true),
    free_cb_(free_cb),
    ptr(ptr),
    sz(sz)
  {
  };


  ~data_store()
  {
    if (to_free && raw && ptr)
    {
      if (free_cb_)
        free_cb_();
      else
        free(ptr);
    }
  };


  // move =

  data_store& operator=(data_store&& other)
  {
    if (this != &other) [[ likely ]]
    {
      clear();

      raw      = other.raw;
      to_free  = other.to_free;
      free_cb_ = std::move(other.free_cb_);
      ptr      = other.ptr;
      v        = std::move(other.v);
      offset   = other.offset;
      sz       = other.sz;
    
      other.ptr     = nullptr;
      other.raw     = false;
      other.to_free = false;
    }

    return *this;
  };


  std::byte*           data() noexcept { return raw ? ptr + offset : v.data() + offset; };
  std::uint32_t        size() noexcept { return sz; };
  std::span<std::byte> span() noexcept { return { data(), size() }; };

  bool empty() const noexcept { return sz == 0; };


  inline void clear()
  {
    if (raw)
    {
      if (to_free && ptr)
      {
        if (free_cb_)
          free_cb_();
        else
          free(ptr);
      }
      ptr = nullptr;
    }
    else
    {
      std::vector<std::byte> new_v;
      v = std::move(new_v);
    }
  };


private:

  // data is stored using a raw pointer, or as a vector

  bool raw{false};
  bool to_free{false};

  std::function<void ()> free_cb_; // for use when a raw pointer requires a specific free technique

  std::byte*             ptr{nullptr};
  std::vector<std::byte> v;

  std::uint32_t          offset{0};
  std::uint32_t          sz{0};       // from offset
};

} // namespace plate
