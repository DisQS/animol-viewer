#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/texture/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/texture/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/texture/shader.hpp"
#endif

