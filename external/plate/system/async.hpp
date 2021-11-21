#pragma once
  
#ifdef PLATE_WEBGL
  #include "webgl/async.hpp"
#endif
  
  
#ifdef PLATE_LINUX_GL
  #include "linux/async.hpp"
#endif
  
  
#ifdef PLATE_LINUX_VULKAN
  #include "linux/async.hpp"
#endif

