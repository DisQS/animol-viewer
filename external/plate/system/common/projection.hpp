#pragma once

#include <cstddef>
#include <cstdlib>

#include <GLES3/gl3.h>

#include "../../gpu/gpu.hpp"

/*

  The global projection matrix

*/


namespace plate {


class projection
{


public:

  using proj_store = std::array<float, 16>;

  proj_store matrix_base_; // the raw projection matrix
  proj_store matrix_;      // the active projection matrix

  float width_;
  float height_;


  void set(std::array<float, 16>& p) noexcept
  {
    matrix_base_ = p;
    matrix_      = p;
  }


  void set(int width, int height) noexcept
  {
    width_  = width;
    height_ = height;

    float m = std::max(width_, height_);

    matrix_ = { 2 / width_, 0,            0,         0,
                0,          2 / height_,  0,         0,
                0,          0,           -0.8f / m, -1.0f / m,
               -1,         -1,            0.3,       1 };

    matrix_base_ = matrix_;
  }


  void reset() noexcept
  {
    matrix_ = matrix_base_;
    stack_size_ = 0;
  }


  void pop() noexcept
  {
    if (stack_size_)
      matrix_ = stack_[--stack_size_];
  }


  void push(const proj_store& p) noexcept
  {
    add_current_to_stack();
    matrix_ = p;
  }


  void push_transform(const proj_store& t) noexcept
  {
    add_current_to_stack();

    gpu::matrix44_mult(matrix_, stack_[stack_size_ - 1], t);
  }


private:


  void add_current_to_stack() noexcept
  {
    if (stack_size_ == max_stack_size_)
    {
      log_error("Past max stack size");
      return;
    }

    stack_[stack_size_++] = matrix_;
  }


  const static int max_stack_size_ = 10;
  std::array<proj_store, max_stack_size_> stack_;
  int stack_size_{0};

}; // class projection
 
} // namespace plate
