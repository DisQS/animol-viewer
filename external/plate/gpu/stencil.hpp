#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/stencil.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/stencil.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/stencil.hpp"
#endif

