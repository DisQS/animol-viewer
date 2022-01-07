#pragma once

#include <cstddef>
#include <cstdlib>

#include <GLES3/gl3.h>

#include "../gpu.hpp"

#include "../../system/common/ui_event_destination.hpp"


/*
    1. push() to start a new stencil layer. The contents is restricted to stencil being applied
              in the previous layer

    2. render() to start drawing using the stencil created in step 1

    3. clear() to redraw what was drawn in step 1 to clear the previous stencil

    4. pop() to revert to the previous stencil

*/

namespace plate {

class stencil {
  
public:

  stencil(std::uint32_t* state, ui_event_destination* w) noexcept :
    state_(state),
    w_(w)
  {
    push_and_render(state_);

    w_->display();

    render(state_);
  }


  ~stencil() noexcept
  {
    clear(state_);

    w_->display();

    pop(state_);
  }


  static void push(std::uint32_t* state) noexcept // render into stencil
  {
    *state += 1;

    if (*state == 1)
      glEnable(GL_STENCIL_TEST);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, *state - 1, 255);
    glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
  }


  static void push_and_render(std::uint32_t* state) noexcept // render into stencil and buffer at the same time
  {
    *state += 1;

    if (*state == 1)
      glEnable(GL_STENCIL_TEST);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, *state - 1, 255);
    glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
  }


  static void render(std::uint32_t* state) noexcept // draw using stencil
  { 
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, *state, 255);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  }


  static void clear(std::uint32_t* state) noexcept // clear from stencil
  {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, *state, 255);
    glStencilOp(GL_DECR, GL_DECR, GL_DECR);
  }


  static void pop(std::uint32_t* state) noexcept // all done
  { 
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    *state -= 1;

    if (*state == 0)
      glDisable(GL_STENCIL_TEST);
    else
    {
      glStencilFunc(GL_EQUAL, *state, 255);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
  }


private:


  std::uint32_t* state_;       // depth pos
  ui_event_destination* w_;

}; // class stencil
 
} // namespace plate
