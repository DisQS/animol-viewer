#pragma once

#include "../system/common/ui_anim.hpp"

#include "widget_alpha.hpp"


namespace plate {

class anim_alpha : public ui_anim
{

public:

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent,
                                                    std::uint32_t dir = Dir::Forward) noexcept
  {
    ui_anim::init(_ui, Prop::None, parent);

    style_      = ui_anim::SoftSoft;
    total_time_ = 0.4;

    widget_alpha_ = make_anim<widget_alpha>(ui_, shared_from_this());

    if (dir == Dir::Forward)
      targets_ = { { &a, 0.0, 1.0 } };
    else
      targets_ = { { &a, 1.0, 0.0 } };

    init_targets();
  }
  

  void on_change() noexcept
  {
    widget_alpha_->set_transform_alpha(a);
  }


  std::string_view name() const noexcept
  {
    return "anim_alpha";
  }


private:


  std::shared_ptr<plate::widget_alpha> widget_alpha_;

  float a;

}; // anim_alpha

} // namespace plate
