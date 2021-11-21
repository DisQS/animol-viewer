#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/circle/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/circle/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/circle/shader.hpp"
#endif

