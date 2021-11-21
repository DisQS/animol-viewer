#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/buffer.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/buffer.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/buffer.hpp"
#endif

