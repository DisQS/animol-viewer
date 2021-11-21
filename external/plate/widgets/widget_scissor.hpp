#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/scissor.hpp"


namespace plate {

class widget_scissor : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::None, parent);
  }

  
  void display() noexcept
	{
    ui_->scissor_->push(box_);
  }

  void undisplay() noexcept
	{
    ui_->scissor_->pop();
  }


  inline void cancel() noexcept
  {
    property_ = Prop::None;
  }

  inline void uncancel() noexcept
  {
    property_ = Prop::Display | Prop::UnDisplay;
  }


  void set(const gpu::int_box& b) noexcept
  {
      uncancel();

      box_ = b;
  }


  std::string_view name() const noexcept
  {
    return "scissor";
  }


private:


  gpu::int_box box_;

}; // widget_scissor

} // namespace plate
