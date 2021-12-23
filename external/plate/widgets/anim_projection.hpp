#pragma once

#include "../system/common/ui_anim.hpp"

#include "widget_projection.hpp"
#include "widget_stencil.hpp"


namespace plate {

class anim_projection : public ui_anim
{

public:

  enum class Options { None, FixedStencil };

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent,
                Options options, float x, float y, float z, std::uint32_t dir = Dir::Forward) noexcept
  {
    ui_anim::init(_ui, Prop::None, parent);

    style_      = ui_anim::SoftSoft;
    total_time_ = 0.6;
        
    if (options == Options::FixedStencil)
    {
      std::shared_ptr<plate::widget_stencil> ws = make_anim<widget_stencil>(ui_, shared_from_this());

      ws->get_mask_parent()->add_child(parent->get_background());

      widget_proj_    = make_anim<widget_projection>(ui_, ws);
    }
    else
      widget_proj_    = make_anim<widget_projection>(ui_, shared_from_this());


    if (dir == Dir::Forward)
      targets_ = {
                  { &anim_offset_x_, x, 0.0 },
                  { &anim_offset_y_, y, 0.0 },
                  { &anim_offset_z_, z, 0.0 }
                 };
    else // reverse
      targets_ = {
                  { &anim_offset_x_, 0.0, x },
                  { &anim_offset_y_, 0.0, y },
                  { &anim_offset_z_, 0.0, z }
                 };

    init_targets();
  }


  void on_change() noexcept
  {
    widget_proj_->set_transform_shift(anim_offset_x_, anim_offset_y_, anim_offset_z_);
  }


  std::string_view name() const noexcept
  {
    return "anim_projection";
  }


private:


  std::shared_ptr<plate::widget_projection> widget_proj_;

  float anim_offset_x_, anim_offset_y_, anim_offset_z_;

}; // anim_projection

} // namespace plate
