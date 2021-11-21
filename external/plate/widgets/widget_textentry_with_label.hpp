#pragma once

#include <memory>

#include "widget_textentry.hpp"


namespace plate {

class widget_textentry_with_label : public widget_textentry
{

public:

  void init(const std::shared_ptr<state>& _ui, const gpu::int_box coords, std::uint32_t prop,
                              const std::shared_ptr<ui_event_destination>& parent,
                              const std::string& text, const std::string& label,
                              gpu::align al, const std::array<gpu::color, 2>& c,
                              const gpu::rotation r, float scale) noexcept
  {
    widget_textentry::init(_ui, coords, prop, parent, text, al, c, r, scale);

    // shift the label across slightly

    x_offset_ = 20 * ui_->pixel_ratio_;

    auto co = coords;
    co.p1.x += x_offset_;

    wlabel_ = make_ui<widget_text>(ui_, co, Prop::Display, shared_from_this(), label, al, c, r, scale);

    if (text.empty())
      mini_ = 0;
    else
    {
      mini_ = 100;
      wlabel_->set_scale_and_offset( 1.0 - 0.3, - x_offset_, font_height_ * 1.2);
    }
  }


  ~widget_textentry_with_label()
  {
    if (anim_id_)
      ui_->anim_.erase(anim_id_);
  }


  void active(bool status) noexcept
  {
    widget_textentry::active(status);

    int target = 0;

    if (status || !text_.empty())
      target = 100;

    if (target_ == target)
      return;
      
    target_ = target;

    std::vector<anim::targets> t{{{ &mini_, mini_, target_ }}};

    if (anim_id_)
      ui_->anim_.erase(anim_id_);

    anim_id_ = ui_->anim_.add(std::move(t), anim::SoftSoft, 0.25, [this, wself{weak_from_this()}]
    {
      if (auto p = wself.lock())
      {
        auto r = mini_ / 100.0;

        wlabel_->set_scale_and_offset( 1.0 - (r * 0.3), -r * x_offset_, r * font_height_ * 1.2);
      }
    }, [this, wself{weak_from_this()}]
    {
      if (auto p = wself.lock())
        anim_id_ = 0;
    });
  }


  std::string_view name() const noexcept
  {
    return "textentry_with_label";
  }
  
private:


  int mini_;   // 0 => full size, 100 => minimised size
  int target_; // target size

  int anim_id_{0};

  float x_offset_;

  std::shared_ptr<widget_text> wlabel_;

}; // widget_textentry_with_label

} // namespace plate
