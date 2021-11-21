#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/shaders/rounded_box/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/shaders/rounded_box/shader.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/shaders/rounded_box/shader.hpp"
#endif

