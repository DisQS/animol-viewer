#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/scissor.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/scissor.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/scissor.hpp"
#endif

