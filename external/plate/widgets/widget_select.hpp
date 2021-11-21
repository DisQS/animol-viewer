#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"


#include "widget_text.hpp"
#include "widget_circle.hpp"


namespace plate {

class widget_select : public ui_event_destination
{


public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box coords, std::uint32_t prop,
            const std::shared_ptr<ui_event_destination>& parent, const std::string text, float scale) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);

    text_  = text;
    scale_ = scale;

    selected_ = true;

    auto b = coords;
    b.p2.x = b.p1.x + b.height();

    wcircle_   = make_ui<widget_circle>(ui_, b,  Prop::Display, shared_from_this(), ui_->txt_color_);

    float f = b.height() * 0.9;

    auto b1 = b;
    b1.p1.x += f;
    b1.p1.y += f;
    b1.p2.x -= f;
    b1.p2.y -= f;

    wcircle_unselected_ = make_ui<widget_circle>(ui_, b1, Prop::Display, shared_from_this(), ui_->bg_color_);

    f = b.height() * 0.8;

    auto b2 = b;
    b2.p1.x += f;
    b2.p1.y += f;
    b2.p2.x -= f;
    b2.p2.y -= f;

    wcircle_selected_ = make_ui<widget_circle>(ui_, b2, Prop::Display, shared_from_this(), ui_->txt_color_);

    wtext_ = make_ui<widget_text>(ui_, coords, prop, shared_from_this(), text_, gpu::align::CENTER,
                                                    ui_->txt_color_, gpu::rotation{0,0,0}, scale_);
  }


  inline void set_click_cb(std::function<void (bool)> cb) noexcept
  {
    click_cb_ = std::move(cb);
  }


  bool selected() const noexcept
  {
    return selected_;
  }


  void selected(bool status) noexcept
  {
    selected_ = status;

    if (selected_)
      wcircle_selected_->show();
    else
      wcircle_selected_->hide();
  }
  

  void display() noexcept
	{
  }


  void ui_mouse_button_update()
	{
    auto& m = ui_->mouse_metric_;

    if (m.click)
    {
      if (selected_)
      {
        selected_ = false;
        wcircle_selected_->hide();
      }
      else
      {
        selected_ = true;
        wcircle_selected_->show();
      }
    }
  }


  bool ui_touch_update(int id)
  {
    if (id != 0)
      return true;

    auto& m = ui_->touch_metric_[id];

    if (m.click)
    {
      if (selected_)
      {
        selected_ = false;
        wcircle_selected_->hide();
      }
      else
      {
        selected_ = true;
        wcircle_selected_->show();
      }
    }

    return true;
  }


  std::string_view name() const noexcept
  {
    return "select";
  }

private:


  std::string text_;
  float scale_;

  bool selected_;
  int  anim_;

  std::shared_ptr<widget_text> wtext_;

  std::shared_ptr<widget_circle> wcircle_;
  std::shared_ptr<widget_circle> wcircle_unselected_;
  std::shared_ptr<widget_circle> wcircle_selected_;

  std::function<void (bool selected)> click_cb_;

}; // widget_textentry

} // namespace plate
