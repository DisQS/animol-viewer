#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"


namespace plate {

class widget_alpha : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::Display | Prop::UnDisplay, parent);
  }

  
  void display() noexcept
	{
    ui_->alpha_.push_transform(transform_);
  }

  void undisplay() noexcept
	{
    ui_->alpha_.pop();
  }


  inline void cancel() noexcept
  {
    property_ = Prop::None;
  }

  inline void uncancel() noexcept
  {
    property_ = Prop::Display | Prop::UnDisplay;
  }


  void set_transform_alpha(float a) noexcept
  {   
    transform_ = a;
  }


  std::string_view name() const noexcept
  {
    return "alpha";
  }


  float transform_;

}; // widget_alpha

} // namespace plate
