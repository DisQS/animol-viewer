#pragma once

#include "../system/common/ui_anim.hpp"

#include "widget_projection_alpha.hpp"


namespace plate {

class anim_merge : public ui_anim
{

public:

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& source,
              const std::shared_ptr<ui_event_destination>& parent, bool is_parent, std::uint32_t dir = Dir::Forward) noexcept
  {
    ui_anim::init(_ui, Prop::None, is_parent ? parent : source);

    style_      = ui_anim::SoftSoft;
    total_time_ = 0.2;
    is_parent_  = is_parent;
    
    widget_proj_alpha_ = make_anim<widget_projection_alpha>(ui_, shared_from_this());

    if (dir == Dir::Forward)
    {
      anim_offset_x_ = (source->coords_.p1.x + source->coords_.width()/2) - (parent->coords_.p1.x + parent->coords_.width()/2);
      anim_offset_y_ = (source->coords_.p1.y + source->coords_.height()/2) - (parent->coords_.p1.y + parent->coords_.height()/2);

      if (is_parent_)
        targets_ = {
                   { &anim_width_,  static_cast<float>(source->coords_.width()),
                                    static_cast<float>(parent->my_width()) },
                   { &anim_height_, static_cast<float>(source->coords_.height()),
                                    static_cast<float>(parent->my_height()) },
                   { &anim_offset_x_, anim_offset_x_, 0.0 },
                   { &anim_offset_y_, anim_offset_y_, 0.0 }
                   };
      else
        targets_ = {
                   { &anim_width_,  static_cast<float>(source->coords_.width()),
                                    static_cast<float>(parent->my_width()) },
                   { &anim_height_, static_cast<float>(source->coords_.height()),
                                    static_cast<float>(parent->my_height()) },
                   { &anim_offset_x_, 0, -anim_offset_x_ },
                   { &anim_offset_y_, 0, -anim_offset_y_ }
                   };
    }
    else // reverse
    {
      anim_offset_x_ = 0;
      anim_offset_y_ = 0;

      targets_ = {
                  { &anim_width_,  static_cast<float>(parent->my_width()),
                                   static_cast<float>(source->coords_.width()) },
                  { &anim_height_, static_cast<float>(parent->my_height()),
                                   static_cast<float>(source->coords_.height()) },
                  { &anim_offset_x_, 0.0, static_cast<float>(
                                                     (source->coords_.p1.x   + source->coords_.width()/2.0) -
                                                     (parent->coords_.p1.x + parent->coords_.width()/2.0)) },
                  { &anim_offset_y_, 0.0, static_cast<float>(
                                                      source->coords_.p2.y - parent->coords_.p2.y) }
                 };
    }

    init_targets();
  }


  void on_change() noexcept
  {
    if (is_parent_)
    {
      if (auto p = parent_.lock())
      {
        widget_proj_alpha_->transform_start();

        widget_proj_alpha_->set_transform_zoom(anim_width_ / p->my_width(), anim_height_ / p->my_height(),
            p->coords_.p1.x + p->coords_.width()/2.0, p->coords_.p1.y + p->coords_.height()/2.0);

        widget_proj_alpha_->set_transform_shift(anim_offset_x_, anim_offset_y_);
      }

      widget_proj_alpha_->set_transform_alpha(run_time_ / total_time_);
    }
    else // we are the child
    {
      if (auto p = parent_.lock())
      {
        widget_proj_alpha_->transform_start();
  
        widget_proj_alpha_->set_transform_zoom(anim_width_ / p->my_width(), anim_height_ / p->my_height(),
            p->coords_.p1.x + p->coords_.width()/2.0, p->coords_.p1.y + p->coords_.height()/2.0);
  
        widget_proj_alpha_->set_transform_shift(anim_offset_x_, anim_offset_y_);

        widget_proj_alpha_->set_transform_alpha(1.0 - run_time_ / total_time_);
      }
    }
  }


  std::string_view name() const noexcept
  {
    return "anim_merge";
  }


private:


  bool is_parent_;

  std::shared_ptr<plate::widget_projection_alpha> widget_proj_alpha_;

  float anim_offset_x_, anim_offset_y_;
  float anim_width_, anim_height_;

}; // anim_merge


// helper

std::pair<std::shared_ptr<anim_merge>, std::shared_ptr<anim_merge>> make_anim_merge(
    const std::shared_ptr<state>& ui, const std::shared_ptr<ui_event_destination>& source,
    const std::shared_ptr<ui_event_destination>& dest, std::uint32_t dir = ui_anim::Dir::Forward) noexcept
{
  auto dest_merge = ui_event_destination::make_anim<anim_merge>(ui, source, dest, true, dir);
  auto src_merge  = ui_event_destination::make_anim<anim_merge>(ui, source, dest, false, dir);

  std::weak_ptr<anim_merge> m = src_merge;

  src_merge->set_end_cb( [m] ()
  {
    if (auto a = m.lock())
      if (auto p = a->parent_.lock())
        p->hide();
  });

  return { dest_merge, src_merge };
}

} // namespace plate
