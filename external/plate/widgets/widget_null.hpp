#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"


namespace plate {

class widget_null : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui) noexcept
  {
    ui_event_destination::init(_ui, gpu::int_box{{0,0},{0,0}}, Prop::None);
  }


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, gpu::int_box{{0,0},{0,0}}, Prop::None, parent);
  }


  std::string_view name() const noexcept
  {
    return "null";
  }


}; // widget_null

} // namespace plate
