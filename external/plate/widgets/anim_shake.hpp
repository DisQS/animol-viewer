#pragma once

#include "../system/common/ui_anim.hpp"

#include "widget_projection.hpp"
#include "widget_stencil.hpp"


namespace plate {

class anim_shake : public ui_anim
{

public:

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_anim::init(_ui, Prop::None, parent);

    style_      = ui_anim::Linear;
    total_time_ = 0.25;
    
    widget_proj_ = make_anim<widget_projection>(ui_, shared_from_this());
    
    targets_ = { { &anim_offset_x_, 0.0, 2 * M_PI * 2 } };

    init_targets();
  }
  

  void on_change() noexcept
  {
    widget_proj_->set_transform_shift(std::sin(anim_offset_x_) * ui_->pixel_ratio_ * 15);
  }


  std::string_view name() const noexcept
  {
    return "anim_shake";
  }


private:


  std::shared_ptr<plate::widget_projection> widget_proj_;

  float anim_offset_x_{0};

}; // anim_shake

} // namespace plate
