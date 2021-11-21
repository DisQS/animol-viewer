#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/projection.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/projection.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/projection.hpp"
#endif

