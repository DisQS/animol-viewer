#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/object/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/object/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/object/shader.hpp"
#endif

