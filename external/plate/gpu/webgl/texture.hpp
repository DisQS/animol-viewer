#pragma once

#include <cstddef>
#include <cstdlib>

#include <GLES3/gl3.h>

#include "../../system/log.hpp"


/*

  Represents a texture used by the gpu.

*/


namespace plate {


class texture {


public:


  texture(texture&& other)
  {
    id_          = other.id_;
    data_        = other.data_;
    total_bytes_ = other.total_bytes_;
    width_       = other.width_;
    height_      = other.height_;
    depth_       = other.depth_;

    other.id_ = 0;
  };


  texture() { };


  texture(const std::byte * data, int total_bytes, int width, int height, int depth) :
    data_(data), total_bytes_(total_bytes), width_(width), height_(height), depth_(depth)
  {
  };


  ~texture()
  {
    if (id_) glDeleteTextures(1, &id_);
  };


  texture& operator=(texture&& other)
  {
    id_          = other.id_;
    data_        = other.data_;
    total_bytes_ = other.total_bytes_;
    width_       = other.width_;
    height_      = other.height_;
    depth_       = other.depth_;

    other.id_ = 0;

    return *this;
  };


  bool upload(std::byte* data, int total_bytes, int width, int height, int depth)
  {
    data_        = data;
    total_bytes_ = total_bytes;
    width_       = width;
    height_      = height;
    depth_       = depth;

    return upload();
  };


  inline bool ready() const noexcept
  {
    return id_ != 0;
  };


  bool upload()
  {
    if (data_ == nullptr || width_ < 1 || height_ < 1 || depth_ < 1 || depth_ > 4 ||
                                                           width_ * height_ * depth_ != total_bytes_)
    {
      log_error("Bad data for texture");
      return false;
    }

    if (!id_)
      glGenTextures(1, &id_);

    if (!id_)
    {
      log_error("Unable to generate texture");
      return false;
    }

    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    switch (depth_)
    {
      case 1:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width_, height_, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data_);
        break;
      case 3:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,   width_, height_, 0, GL_RGB,   GL_UNSIGNED_BYTE, data_);
        break;
      case 4:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  width_, height_, 0, GL_RGBA,  GL_UNSIGNED_BYTE, data_);
        break;
      default:
        log_error(FMT_COMPILE("bad depth: {}"), depth_);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
  };


  inline GLuint get_id() const noexcept { return id_; };


  GLuint  id_{0};

  const std::byte* data_{nullptr};
  int total_bytes_{0};

  int width_{0};
  int height_{0};
  int depth_{0};


}; // class texture
 
} // namespace plate
