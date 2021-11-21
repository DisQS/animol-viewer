#pragma once

#include <cstddef>
#include <cstdlib>


#include "../log.hpp"

/*

  The global alpha

*/


namespace plate {

class alpha
{


public:

  float alpha_base_{1.0}; // the raw alpha
  float alpha_{1.0};      // the active alpha


  void set(float a) noexcept
  {
    alpha_base_ = a;
    alpha_      = a;
  }


  void reset() noexcept
  {
    alpha_ = alpha_base_;
    stack_size_ = 0;
  }


  void pop() noexcept
  {
    if (stack_size_)
      alpha_ = stack_[--stack_size_];
  }


  void push(float a) noexcept
  {
    add_current_to_stack();
    alpha_ = a;
  }


  void push_transform(float a) noexcept
  {
    add_current_to_stack();
    alpha_ *= a;
  }


private:


  void add_current_to_stack() noexcept
  {
    if (stack_size_ == max_stack_size_)
    {
      log_error("Past max stack size");
      return;
    }

    stack_[stack_size_++] = alpha_;
  }


  const static int max_stack_size_ = 10;
  std::array<float, max_stack_size_> stack_;
  int stack_size_{0};

}; // class alpha
 
} // namespace plate
