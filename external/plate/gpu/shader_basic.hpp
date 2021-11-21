#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/basic/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/basic/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/basic/shader.hpp"
#endif

