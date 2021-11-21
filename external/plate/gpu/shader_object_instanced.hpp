#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/object_instanced/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/object_instanced/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/object_instanced/shader.hpp"
#endif

