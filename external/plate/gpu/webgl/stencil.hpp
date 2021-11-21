#pragma once

#include <cstddef>
#include <cstdlib>

#include <GLES3/gl3.h>

#include "../gpu.hpp"

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

  void push() noexcept // render into stencil
  {
    if (!depth_++)
      glEnable(GL_STENCIL_TEST);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, depth_ - 1, 255);
    glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
  }


  void render() noexcept // draw using stencil
  { 
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, depth_, 255);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  }


  // fixme - auto clear in pop() by drawing a framebuffer sized box instead?

  void clear() noexcept // clear from stencil
  {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, depth_, 255);
    glStencilOp(GL_DECR, GL_DECR, GL_DECR);
  }


  void pop() noexcept // all done
  { 
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if (!--depth_)
      glDisable(GL_STENCIL_TEST);
    else
    {
      glStencilFunc(GL_EQUAL, depth_, 255);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
  }

private:


  std::uint32_t depth_{0};

}; // class stencil
 
} // namespace plate
