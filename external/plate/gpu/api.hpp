#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/api.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/api.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/api.hpp"
#endif

