#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/compute.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "gl/compute.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "vulkan/compute.hpp"
#endif

