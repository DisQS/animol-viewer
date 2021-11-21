#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/text/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/text/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/text/shader.hpp"
#endif

