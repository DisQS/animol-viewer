#pragma once

#include <cstddef>
#include <cstdlib>

#include <GLES3/gl3.h>

#include "../gpu.hpp"


namespace plate {


class scissor {

public:

  void push(gpu::int_box box) noexcept
  {
    if (enabled_) // scissor into a scissor area..
    {
      add_current_to_stack();

      box_.p1.x = std::max(box.p1.x, box_.p1.x);
      box_.p1.y = std::max(box.p1.y, box_.p1.y);
      box_.p2.x = std::min(box.p2.x, box_.p2.x);
      box_.p2.y = std::min(box.p2.y, box_.p2.y);

      // ensure box size isn't negative

      box_.p2.x = std::max(box_.p1.x, box_.p2.x);
      box_.p2.y = std::max(box_.p1.y, box_.p2.y);
    }
    else
    {
      enabled_ = true;

      box_ = box;

      glEnable(GL_SCISSOR_TEST);
    }

    glScissor(box_.p1.x, box_.p1.y, box_.p2.x - box_.p1.x, box_.p2.y - box_.p1.y);
  }


  void pop() noexcept
  {
    if (stack_size_)
    {
      box_ = stack_[--stack_size_];
      glScissor(box_.p1.x, box_.p1.y, box_.p2.x - box_.p1.x, box_.p2.y - box_.p1.y);
    }
    else 
      if (enabled_)
      {
        glDisable(GL_SCISSOR_TEST);
        enabled_ = false;
      }
  }


private:


  void add_current_to_stack() noexcept
  {
    if (stack_size_ == max_stack_size_)
    {
      log_error("Past max stack size");
      return;
    }

    stack_[stack_size_++] = box_;
  }


  bool enabled_{false};
  gpu::int_box box_;        // the active box

  const static int max_stack_size_ = 10;
  std::array<gpu::int_box, max_stack_size_> stack_;
  int stack_size_{0};

}; // class scissor
 
} // namespace plate
