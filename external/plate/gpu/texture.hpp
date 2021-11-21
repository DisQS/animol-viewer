#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/texture.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/texture.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/texture.hpp"
#endif

