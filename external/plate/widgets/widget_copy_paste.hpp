#pragma once

#include "../system/common/ui_event_destination.hpp"

#include "../system/common/font.hpp"
#include "gpu/common/gpu.hpp"
#include "gpu/scissor.hpp"

#include "widget_rounded_box.hpp"
#include "widget_text.hpp"


namespace plate {

class widget_copy_paste : public ui_event_destination
{

public:

  enum Options { COPY = 1, PASTE = 2 };

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent,
                                                    int xpos, int ypos, int options) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::Display, parent);
    options_ = options;

    auto width = get_max_width();

    gpu::int_box b{{ xpos - width/2, ypos }, { xpos + width/2, ypos + static_cast<int>(45 * ui_->pixel_ratio_) }};

    ui_event_destination::set_geometry(b);

    w_rbox_ = make_ui<widget_rounded_box>(ui_, coords_, Prop::Display, shared_from_this(),
                                                ui_->bg_color_, 15 * ui_->pixel_ratio_);

    b.p2.x = b.p1.x + (my_width())/2;

    w_copy_ = make_ui<widget_text>(ui_, b, Prop::Display | Prop::Input, shared_from_this(), "COPY",
                gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.5f);

    b.p1.x = b.p2.x;
    b.p2.x = coords_.p2.x;

    w_paste_ = make_ui<widget_text>(ui_, b, Prop::Display | Prop::Input, shared_from_this(), "PASTE",
                gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.5f);

    w_copy_->set_click_cb([this]
    {
      ui_->incoming_key_event(ui_event::KeyEventDown, "c", "", ui_event::KeyModControl);

      if (cb_)
        cb_("COPY");
    });

    w_paste_->set_click_cb([this]
    {
      ui_->paste();

      if (cb_)
        cb_("PASTE");
    });

    options_ = -1;
    set_options(options);

    set_geom();

    do_slide();
  }


  void set_callback(std::function< void (std::string_view s)> cb) noexcept
  {
    cb_ = cb;
  }


  void set_options(int options) noexcept
  {
    if (options_ == options)
      return;

    options_ = options;

    if (get_count() == 0)
    {
      w_paste_->hide();
      w_copy_->hide();
    }
    if (get_count() == 1)
    {
      w_copy_->hide();
    }

    do_slide();
  }


  void display() noexcept
  {
    // display part buttons during anim

    if (anim_handle_ == 0)
      return;

    auto b = coords_;
    b.p1.x = b.p2.x - cur_width_;

    w_rbox_->display();

    ui_->scissor_->push(b);

    w_copy_->display();
    w_paste_->display();

    ui_->scissor_->pop();
  }


  std::string_view name() const noexcept
  {
    return "copy_paste";
  }
  

private:


  int get_count() const noexcept
  {
    int count = 0;

    if (options_ & COPY)
      ++count;

    if (options_ & PASTE)
      ++count;

    return count;
  }


  inline int get_target_width() const noexcept
  {
    return get_count() * 80 * ui_->pixel_ratio_;
  }

  inline int get_max_width() const noexcept
  {
    return 2 * 80 * ui_->pixel_ratio_;
  }


  void set_geom()
  {
    int mw = get_max_width();

    auto b = coords_;
    b.p1.x = b.p2.x - cur_width_;

    w_rbox_->set_geometry(b);
  }


  void do_slide()
  {
    if (cur_width_ == get_target_width())
      return;

    std::vector<anim::targets> t{{{&cur_width_, cur_width_, get_target_width()}}};

    anim_handle_ = ui_->anim_.add(std::move(t), anim::SoftSoft, 0.25, [this]
    {
      set_geom();
    },
    [this]
    {
      anim_handle_ = 0;

      if (cur_width_ == 0)
      {
        w_rbox_->hide();
        w_paste_->hide();
        w_copy_->hide();
      }
      else
      {
        if (cur_width_ < get_max_width())
        {
          w_rbox_->show();
          w_paste_->show();
          w_copy_->hide();
        }
        else
        {
          if (cur_width_ == get_max_width())
          {
            w_rbox_->show();
            w_paste_->show();
            w_copy_->show();
          }
        }
      }
    });

    // during anim - display these manually

    w_rbox_->hide();
    w_paste_->hide();
    w_copy_->hide();

    reposition(); // move this widget to end of display list
  }


  int options_;

  int cur_width_{0};

  std::function<void (std::string_view)> cb_;

  std::shared_ptr<widget_rounded_box>  w_rbox_; // bg

  std::shared_ptr<widget_text> w_copy_;
  std::shared_ptr<widget_text> w_paste_;

  int anim_handle_{0};

}; // widget_keyboard_copy_paste


} // namespace plate

