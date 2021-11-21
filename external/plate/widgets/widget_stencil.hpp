#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/stencil.hpp"

#include "widget_null.hpp"


namespace plate {

class widget_stencil : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::Display | Prop::UnDisplay, parent);

    mask_ = make_ui<widget_null>(ui_, shared_from_this());
    mask_->set_part_display();
  }

  
  void display() noexcept
	{
    ui_->stencil_->push();

    for (auto& w : mask_->children_)
      w->tree_display();
    
    ui_->stencil_->render();
  }

  void undisplay() noexcept
	{
    ui_->stencil_->clear();

    for (auto& w : mask_->children_)
      w->tree_display();

    ui_->stencil_->pop();
  }


  inline void cancel() noexcept
  {
    property_ = Prop::None;
    mask_->property_ = Prop::None;
  }


  inline void uncancel() noexcept
  {
    property_ = Prop::Display | Prop::UnDisplay;
    mask_->property_ = Prop::PartDisplay;
  }


  std::shared_ptr<widget_null> get_mask_parent() noexcept
  {
    return mask_;
  }


  std::string_view name() const noexcept
  {
    return "stencil";
  }


private:


  std::shared_ptr<widget_null> mask_;

}; // widget_stencil

} // namespace plate
