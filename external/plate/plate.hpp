#pragma once

#include "gpu/gpu.hpp"

#ifdef PLATE_LINUX_GL
  #define GLFW_INCLUDE_GL

  #include "system/glfw/ui_event_impl.hpp"
#endif


#ifdef PLATE_WEBGL
  #include "system/webgl/ui_event_impl.hpp"
#endif


#ifdef PLATE_LINUX_VULKAN
  #define GLFW_INCLUDE_VULKAN

  #include "system/glfw/ui_event_impl.hpp"
#endif


#include "system/common/ui_event.hpp"
#include "system/common/ui_event_destination.hpp"
#include "system/common/ui_anim.hpp"

#include "gpu/gpu.hpp"
#include "gpu/buffer.hpp"
#include "gpu/api.hpp"

#include "widgets/widget_null.hpp"


namespace plate
{
  inline std::shared_ptr<plate::state> create(std::string canvas_name, std::string font_file, std::function< void (bool)> cb) noexcept
  {
    auto p = arch_create(canvas_name, font_file, cb);

    auto w = ui_event_destination::make_ui<widget_null>(p);

    return p;
  };

} // namespace plate
