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


  void set_orthogonal(bool o, anim* a = nullptr) noexcept
  {
    if (o == use_orthogonal_)
      return;

    use_orthogonal_ = o;

    if (a)
    {
      auto old = matrix_[11];

      set(width_, height_);

      std::vector<anim::targets> t;

      t.emplace_back(&anim_w_, old, anim_w_);

      anim_w_ = old;

      a->add(std::move(t), anim::Style::LogisticMedium, 0.6);

    }
    else
      set(width_, height_);
  }


  inline bool is_orthogonal() const noexcept
  {
    return use_orthogonal_;
  }


  inline float get_m() const noexcept
  {
    return std::max(width_, height_);
  }


  void set(std::array<float, 16>& p) noexcept
  {
    matrix_base_ = p;
    matrix_      = p;
  }


  inline void set(int width, int height) noexcept
  {
    if (use_orthogonal_)
      set_orthogonal(width, height);
    else
      set_perspective(width, height);
  }


  void set_perspective(int width, int height) noexcept
  {
    width_  = width;
    height_ = height;

    float m = std::max(width_, height_);

    matrix_ = { 2 / width_, 0,            0,          0,
                0,          2 / height_,  0,          0,
                0,          0,           -1 / (8*m), -1 / (1*m),   // depth size is 2 * (8 * m)
               -1,         -1,            0,          1 };     // shift x any y by -1 (so 0,0 is at middle of canvas)

    matrix_base_ = matrix_;
    anim_w_ = matrix_[11];
  }


  void set_orthogonal(int width, int height) noexcept
  {
    width_  = width;
    height_ = height;

    float m = std::max(width_, height_);

    matrix_ = { 2 / width_, 0,            0,         0,
                0,          2 / height_,  0,         0,
                0,          0,           -1 / (8*m), 0.000001,   // no .w applied i.e, w = 1
               -1,         -1,            0,         1 };

    matrix_base_ = matrix_;
    anim_w_ = matrix_[11];
  }


  void reset() noexcept
  {
    matrix_ = matrix_base_;
    matrix_[11] = anim_w_;

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

  bool use_orthogonal_{false};

  float anim_w_;

}; // class projection
 
} // namespace plate
