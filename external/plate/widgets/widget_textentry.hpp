#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../system/common/ui_event_destination_autoscroll.hpp"
#include "../system/common/font.hpp"
#include "../gpu/shader_basic.hpp"
#include "../gpu/stencil.hpp"


#include "widget_box.hpp"
#include "widget_circle.hpp"
#include "widget_keyboard.hpp"
#include "widget_copy_paste.hpp"

/*
    A textentry widget - define box/shape/colors

    per char store:

      utf8 character
      tex_coords
      2 triangles
      advance_x and y for next char pos
*/

namespace plate {

class widget_textentry : public ui_event_destination_autoscroll
{
  enum Cursor { None = 0, Update = 1, Vibrate = 2, NoScroll = 4 };

  class selection
  {
    public:

      ~selection()
      {
        if (wbox_)
          wbox_->disconnect_from_parent();
      }


      void set_color(const std::array<gpu::color, 2>& c) // fixme
      {
        color_ = c;
      }

      bool enabled() const noexcept { return enabled_; };

      void display()
      {
        if (enabled_)
          wbox_->display();
      }


      void set_height(int height)
      {
        height_ = height;
      }

      
      void set_offset(std::shared_ptr<state>& s, const int& x_pos, const int& y_pos)
      {
        if (!wbox_)
          wbox_ = ui_event_destination::make_ui<widget_box>(s, gpu::int_box{{-1, -1},{-1, -1}},
                                                                      Prop::None, nullptr, color_);

        wbox_->set_offset({x_pos, y_pos});
      }


      void enable(std::shared_ptr<state>& s, int char_start, int pixel_start, int char_end, int pixel_end)
      {
        char_start_  = char_start;
        pixel_start_ = pixel_start;
        char_end_    = char_end;
        pixel_end_   = pixel_end;

        if ((fixed_char_start_ != -1) && (char_start_ > fixed_char_start_))
        {
          char_start_ = fixed_char_start_;
          pixel_start_ = fixed_pixel_start_;
        }

        if ((fixed_char_end_ != -1) && (char_end_ < fixed_char_end_))
        {
          char_end_ = fixed_char_end_;
          pixel_end_ = fixed_pixel_end_;
        }

        auto t_pixel_start = pixel_start_;
        auto t_pixel_end   = pixel_end_;

        if (pixel_end_ - pixel_start_ < 2) // make it like a cursor
        {
          t_pixel_start -= 1;
          t_pixel_end   += 1;
        }

        if (!wbox_)
          wbox_ = ui_event_destination::make_ui<widget_box>(s,
                  gpu::int_box{{t_pixel_start, 0},{t_pixel_end, height_}}, Prop::None, nullptr, color_);
        else
          wbox_->set_geometry({t_pixel_start, 0, t_pixel_end, height_});

        enabled_ = true;
      }


      void disable()
      {
        enabled_ = false;
        clear_fix();
      }


      bool get_selection(int& char_start, int& char_end)
      {
        if (!enabled_)
          return false;

        if (char_start_ < char_end_)
        {
          char_start = char_start_;
          char_end   = char_end_;
        }
        else
        {
          char_start = char_end_;
          char_end   = char_start_;
        }

        return true;
      }


      bool get_pixels(int& pixel_start, int& pixel_end)
      {
        if (!enabled_)
          return false;

        pixel_start = pixel_start_;
        pixel_end   = pixel_end_;

        float offx, offy;

        if (wbox_)
        {
          wbox_->get_offset(offx, offy);
          pixel_start += offx;
          pixel_end   += offx;
        }

        return true;
      }


      void fix() noexcept
      {
        fixed_char_start_  = char_start_;
        fixed_char_end_    = char_end_;

        fixed_pixel_start_ = pixel_start_;
        fixed_pixel_end_   = pixel_end_;
      }

      void fix_at(int pos, int pixel)
      {
        fixed_char_start_  = pos;
        fixed_pixel_start_ = pixel;
        fixed_char_end_    = pos;
        fixed_pixel_end_   = pixel;
      }

      void clear_fix() noexcept
      {
        fixed_char_start_ = fixed_char_end_ = -1;
      }


      void set_left(int char_start, int pixel_start)
      {
        char_start_  = char_start;
        pixel_start_ = pixel_start;

        wbox_->set_geometry({pixel_start_, 0, pixel_end_, height_});
      }


      void set_right(int char_end, int pixel_end)
      {
        char_end_  = char_end;
        pixel_end_ = pixel_end;

        wbox_->set_geometry({pixel_start_, 0, pixel_end_, height_});
      }


      int char_start_, char_end_;
      int pixel_start_, pixel_end_;


    private:


      std::array<gpu::color, 2> color_;

      bool enabled_{false};
      int height_;

      int fixed_char_start_{-1};
      int fixed_char_end_{-1};
      int fixed_pixel_start_{-1};
      int fixed_pixel_end_{-1};

      std::shared_ptr<widget_box> wbox_; // highlight box
  };


public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box coords, std::uint32_t prop,
                              const std::shared_ptr<ui_event_destination>& parent,
                              const std::string text,
                              gpu::align al, const std::array<gpu::color, 2>& c,
                              const gpu::rotation r, float scale) noexcept
  {
    ui_event_destination_autoscroll::init(_ui, coords, prop, parent);

    text_     = text;
    align_    = al;
    color_    = c;
    rotation_ = r;
    scale_    = scale;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * scale_;

    font_height_ = ui_->font_->get_max_height() * font_scale;

    selection_.set_color(ui_->bg_color_); //ui_->select_color_);
    selection_.set_height(font_height_);

    upload_vertex();

    uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    upload_uniform();

    int sep = (my_height() - font_height_) / 2; // distance from bottom of widget coords to text

    auto line_c = coords;

    line_c.p1.y += sep - (8 * ui_->pixel_ratio_);

    line_c.p2.y = line_c.p1.y + (2 * ui_->pixel_ratio_);

    std::array<gpu::color, 2> seethrough{{{0,0,0,0},{0,0,0,0}}};

    wbox_stencil_ = ui_event_destination::make_ui<widget_box>(ui_, coords_, Prop::None, shared_from_this(),
                                                                                              seethrough);

    wbox_ = ui_event_destination::make_ui<widget_box>(ui_, line_c, Prop::None, shared_from_this(), c);

    int hwidth = ui_->pixel_ratio_;

    wbox_c_ = ui_event_destination::make_ui<widget_box>(ui_, gpu::int_box{
                        {coords_.p1.x - hwidth, coords_.p1.y + sep},
                        {coords_.p1.x + hwidth, coords_.p2.y - sep}}, Prop::None, shared_from_this(), color_);

    wcircle_   = ui_event_destination::make_ui<widget_circle>(ui_, gpu::int_box{{0,0},{0,0}}, Prop::None,
                                                                                shared_from_this(), color_);
    wcircle_1_ = ui_event_destination::make_ui<widget_circle>(ui_, gpu::int_box{{0,0},{0,0}}, Prop::None,
                                                                                shared_from_this(), color_);
    wcircle_2_ = ui_event_destination::make_ui<widget_circle>(ui_, gpu::int_box{{0,0},{0,0}}, Prop::None,
                                                                                shared_from_this(), color_);

    wtext_copy_paste_ = ui_event_destination::make_ui<widget_copy_paste>(ui_, shared_from_this(),
                            coords_.p1.x + my_width()/2, coords_.p1.y + sep + (font_height_ * 1.2) , 0);

    cursor_ = meta_.size();
    upload_cursor_pos();

    set_scroll(scroll::X);
  }


  inline const std::string& get_string() const noexcept
  {
    return text_;
  }


  inline void set_done_cb(std::function<void (bool)> cb) noexcept
  {
    done_cb_ = std::move(cb);
  }


  inline void set_active_cb(std::function<void ()> cb) noexcept
  {
    active_cb_ = std::move(cb);
  }


  inline void set_password(bool is_pw) noexcept
  {
    password_ = is_pw;
    upload_vertex();
  }


  virtual void active(bool status) noexcept // called whenever active status changes
  {
    if (status && active_cb_)
      active_cb_();
  }
  
  void display() noexcept
	{
    pre_display();

    if (auto_scroll_)
    {
      if (auto_scroll_ < 0)
      {
        ++auto_scroll_;

        if (auto_scroll_ == 0 && cursor_ > 0)
        {
          auto_scroll_ = -3;
          --cursor_;
          upload_cursor_pos();

          if (selection_.enabled())
          {
            if (touch_type_ == 1)
              selection_.set_left(cursor_, meta_[cursor_].x);
            if (touch_type_ == 2)
              selection_.set_right(cursor_, meta_[cursor_].x);
            if (touch_type_ == 4)
              selection_.enable(ui_, cursor_, meta_[cursor_].x, cursor_, meta_[cursor_].x);

            upload_selection_pos();
          }
        }
      }
      else // > 0
      {
        --auto_scroll_;

        if (auto_scroll_ == 0 && cursor_ < meta_.size())
        {
          auto_scroll_ = 3;
          ++cursor_;
          upload_cursor_pos();

          if (selection_.enabled())
          {
            if (touch_type_ == 1)
              selection_.set_left(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);
            if (touch_type_ == 2)
              selection_.set_right(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);
            if (touch_type_ == 4)
              selection_.enable(ui_, cursor_, meta_[cursor_ - 1].x + meta_[cursor_-1].advance,
                                     cursor_, meta_[cursor_ - 1].x + meta_[cursor_-1].advance);

            upload_selection_pos();
          }
        }
      }

      if (auto_scroll_)
        ui_->do_draw();
    };

    if (hold_timer_ != 0)
    {
      if (hold_timer_ < 0)
        hold_timer_ = 0.3;
      else
      {
        hold_timer_ -= ui_->frame_diff_;

        if (hold_timer_ <= 0)
        {
          hold_timer_actioned_ = true;
          hold_timer_ = 0;
          is_touch_ = true;
          cursor_ = get_pos_from_screen(ui_->touch_metric_[0].pos.x);
          select_word();
          if (selection_.enabled())
            wtext_copy_paste_->set_options(widget_copy_paste::COPY | widget_copy_paste::PASTE);
          else
          {
            wtext_copy_paste_->set_options(widget_copy_paste::PASTE);
            upload_cursor_pos(Update|Vibrate|NoScroll);
          }
          if (!active_)
          {
            send_keyboard(true);
            set_active(true);
          }
        }
      }
      ui_->do_draw();
    };

    ui_->stencil_->push();

    wbox_stencil_->display();

    ui_->stencil_->render();

    wbox_->display();

    if (active_)
      selection_.display();

    if (!meta_.empty())
      ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_,
                    uniform_buffer_, vertex_buffer_, ui_->font_->texture_,
                    color_[ui_->color_mode_], ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());
    
    ui_->stencil_->clear();
    wbox_stencil_->display();

    ui_->stencil_->pop();

    if (active_)
    {
      if (selection_.enabled())
      {
        if (is_touch_)
        {
          int s, e;
  
          if (selection_.get_pixels(s, e))
          {
            if (s >= coords_.p1.x && s <= coords_.p2.x)
              wcircle_1_->display();
            if (e >= coords_.p1.x && e <= coords_.p2.x)
              wcircle_2_->display();
          }
        }
      }
      else
      {
        if (wbox_c_->get_offset_x() >= 0 && wbox_c_->get_offset_x() <= my_width())
        {
          wbox_c_->display();

          if (is_touch_)
            wcircle_->display();
        }
      }
    }
  };


  void deactivate()
  {
    selection_.disable();
    is_touch_ = false;
    hold_timer_ = 0;

    wtext_copy_paste_->set_options(0);
    set_active(false);
  };


  void activate(bool with_touch)
  {
    cursor_ = meta_.size();
    select_line();
    send_keyboard(with_touch);
    set_active(true);
  };


  void ui_out()
	{
    //log_debug("textentry got mouse out");
  }


  bool ui_mouse_position_update()
	{
    auto& m = ui_->mouse_metric_;

    if (m.swipe && m.id == ui_event::MouseButtonLeft)
    {
      if (!auto_scroll_)
      {
        // find and set 'end' cursor position

        auto end_cursor = get_pos_from_screen(m.pos.x);

        if (end_cursor != cursor_)
        {
          if (!selection_.enabled())
            selection_.fix_at(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);

          cursor_ = end_cursor;

          if (cursor_ == 0)
            selection_.enable(ui_, 0, meta_[0].x, 0, meta_[0].x);
          else
            selection_.enable(ui_, cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance,
                                   cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);
        }
      }

      if (m.pos.x <= coords_.p1.x + x_space_ + (5 * ui_->pixel_ratio_) && x_.offset > 0)
      {
        touch_type_ = 4;
        if (auto_scroll_ >= 0)
          auto_scroll_ = -3;
      }
      else
        if (m.pos.x >= coords_.p2.x - x_space_ - (5 * ui_->pixel_ratio_))
        {
          touch_type_ = 4;
          if (auto_scroll_ <= 0)
            auto_scroll_ = 3;
        }
        else
        {
          if (auto_scroll_)
          {
            touch_type_ = 0;
            if (!selection_.enabled())
              upload_cursor_pos(Update);
            else
            {
              upload_cursor_pos();
              upload_selection_pos();
            }
  
            auto_scroll_ = 0;
          }
        }
          
    }

    return true;
  };


  void ui_mouse_button_update()
	{
    auto& m = ui_->mouse_metric_;

    if (auto_scroll_)
      auto_scroll_ = 0;

    if (m.id == ui_event::MouseButtonLeft) // down on left
    {
      // find and set cursor position

      int offset = m.pos.x - (coords_.p1.x + x_space_) + x_.offset;
      if (offset < 0)
        offset = 0;

      cursor_ = meta_.size();

      int i = 0;
      for (auto& ma : meta_)
      {
        if (ma.x + ma.advance/2 > offset)
        {
          cursor_ = i;
          break;
        }
        ++i;
      }

      log_debug(FMT_COMPILE("in swipe: {}"), m.swipe);
      double now = ui_event::now();
      if (!m.swipe && ((now - m.prev_down_start) <= 300))
      {
        log_debug(FMT_COMPILE("double down enabled: {}"), selection_.enabled());
        if (selection_.enabled())
          select_line();
        else
          select_word();
      }
      else
        upload_cursor_pos(Update);
      
      if (!active_)
      {
        send_keyboard();
        set_active(true);
      }

      return;
    }
  };


  bool ui_touch_update(int id)
  {
    if (id != 0)
      return true;

    auto& m = ui_->touch_metric_[id];

    if (m.st == ui_event::TouchEventUp)
    {
      if (auto_scroll_) // we were in auto scroll mode so stop it
      {
        if (selection_.enabled())
        {
          upload_cursor_pos();
          upload_selection_pos(Vibrate);
        }
        else
          upload_cursor_pos(Update | Vibrate);

        auto_scroll_ = 0;
      }
      else
      {
        if ((touch_type_ == 0 || touch_type_ == 3 || m.double_click) && !m.swipe && !did_stop_anim_ && !hold_timer_actioned_)
        {
          if (!meta_.empty() || active_)
            is_touch_ = true;

          // update cursor position

          auto old_cursor = cursor_;
          cursor_ = get_pos_from_screen(m.pos.x);

          if (m.double_click)
          {
            if (selection_.enabled())
              select_line();
            else
              select_word();

            wtext_copy_paste_->set_options((selection_.enabled() ? widget_copy_paste::COPY : 0) |
                                                                   widget_copy_paste::PASTE);
          }
          else
          {
            upload_cursor_pos(((old_cursor == cursor_) || !m.swipe) ? Update|NoScroll : Update|Vibrate|NoScroll);
            if (is_touch_)
              wtext_copy_paste_->set_options(widget_copy_paste::PASTE);
          }
    
          if (!active_)
          {
            send_keyboard(true);
            set_active(true);
          }
        }
      }

      if (touch_type_ != 0)
        m.speed.x = 0;
      ui_event_destination_autoscroll::ui_touch_update(m.id);

      hold_timer_ = 0;

      return true;
    }

    if (m.st == ui_event::TouchEventDown)
    {
      // could either be a general touch, or touching the selection handles

      touch_type_ = 0;
      hold_timer_actioned_ = false;

      int s, e;
      if (selection_.get_pixels(s, e))
      {
        auto d1 = abs(m.pos.x - s);
        auto d2 = abs(m.pos.x - e);

        if (d1 < d2 && d1 < 25 * ui_->pixel_ratio_)
          touch_type_ = 1;
        else
          if (d2 < d1 && d2 < 25 * ui_->pixel_ratio_)
            touch_type_ = 2;
      }
      else // cursor?
      {
        if (is_touch_ && abs(m.pos.x - get_pixel_offset_from_cursor(cursor_)) <= 25 * ui_->pixel_ratio_)
          touch_type_ = 3;
      }

      if (touch_type_ == 0 || touch_type_ == 3)
        hold_timer_ = -1;

      ui_event_destination_autoscroll::ui_touch_update(m.id); // stop autoscroll
    }

    if (m.st == ui_event::TouchEventDown || m.st == ui_event::TouchEventMove)
    {
      if (m.swipe)
        hold_timer_ = 0;

      if (touch_type_ == 0) // pass this to autoscroll
      {
        ui_event_destination_autoscroll::ui_touch_update(m.id);
        return true;
      }

      if (touch_type_ == 3 && !hold_timer_actioned_)
      {
        // update cursor position

        if ((m.pos.x >= coords_.p1.x + x_space_) && (m.pos.x <= coords_.p2.x - x_space_))
        {
          auto old_cursor = cursor_;
          cursor_ = get_pos_from_screen(m.pos.x);

          upload_cursor_pos(((old_cursor == cursor_) || !m.swipe) ? Update|NoScroll : Update|Vibrate|NoScroll);
        }
      }

      if (touch_type_ == 1 && !auto_scroll_)
      {
        cursor_ = get_pos_from_screen(m.pos.x);
        if (cursor_ != selection_.char_start_)
        {
          if (cursor_ == 0)
            selection_.set_left(cursor_, 0);
          else
            selection_.set_left(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);

          upload_selection_pos(Vibrate);
        }
      }

      if (touch_type_ == 2 && !auto_scroll_)
      {
        cursor_ = get_pos_from_screen(m.pos.x);
        if (cursor_ != selection_.char_end_)
        {
          if (cursor_ == 0)
            selection_.set_right(cursor_, 0);
          else
            selection_.set_right(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);

          upload_selection_pos(Vibrate);
        }
      }

      // go into auto scroll?

      if (m.pos.x <= coords_.p1.x + x_space_ + (5 * ui_->pixel_ratio_) && x_.offset > 0)
      {
        if (auto_scroll_ >= 0)
          auto_scroll_ = -3;
      }
      else
        if (m.pos.x >= coords_.p2.x - x_space_ - (5 * ui_->pixel_ratio_))
        {
            if (auto_scroll_ <= 0)
            auto_scroll_ = 3;
        }
        else
          if (auto_scroll_)
          {
            if (!selection_.enabled())
              upload_cursor_pos(Update);
            else
            {
              upload_cursor_pos();
              upload_selection_pos();
            }

            auto_scroll_ = 0;
          }
    }

    return true;
  };


  void scroll_update() noexcept
  {
    upload_uniform();
    if (selection_.enabled())
      upload_selection_pos();
    else
      upload_cursor_pos(NoScroll|Update);
  };


  void key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8, ui_event::KeyMod mod)
  {
    is_touch_ = false;
    hold_timer_ = 0;
    wtext_copy_paste_->set_options(0);

    stop_scrolling();

    if (event == ui_event::KeyEventDown && (code_utf8 == "ShiftLeft" || code_utf8 == "ShiftRight"))
      shift_pressed_ = true;

    if (event == ui_event::KeyEventUp   && (code_utf8 == "ShiftLeft" || code_utf8 == "ShiftRight"))
      shift_pressed_ = false;

    if (mod == ui_event::KeyModControl)
    {
      if (event == ui_event::KeyEventDown)
      {
        if (utf8 == "c") // ctrl-C
        {
          int char_start, char_end; 
          selection_.get_selection(char_start, char_end);

          if (char_end <= char_start || char_end < 1 || char_start < 0 ||
              char_end > meta_.size() || char_start >= meta_.size())
            return;

          auto start = meta_[char_start].index;
          std::string s = text_.substr(start, (meta_[char_end-1].index + meta_[char_end-1].len) - start);

          if (!s.empty())
            ui_event::js_copy(s.data(), s.size());

          selection_.disable();

          return;
        }
        if (utf8 == "v") // ctrl-V
        {
          ui_->paste();

          return;
        }
      }

      return;
    }

    if (event == ui_event::KeyEventDown || event == ui_event::KeyEventPress)
    {
      log_debug(FMT_COMPILE("utf8: {} code_utf8: {}"), utf8, code_utf8);
      if (utf8 == "" || (utf8 == "Tab" && code_utf8 == "Tab"))
      {
        if (code_utf8 == "Backspace")
        {
          if (selection_.enabled())
          {
            int char_start, char_end; 
            selection_.get_selection(char_start, char_end);
            text_.erase(meta_[char_start].index, (meta_[char_end-1].index + meta_[char_end-1].len) - meta_[char_start].index);
            cursor_ = meta_[char_start].index;
          }
          else
          {
            if (meta_.size() == 0 || cursor_ == 0)
            {
              keyboard::cancel_delete();
              return;
            }

            auto& m = meta_[cursor_ - 1];

            text_.erase(m.index, m.len);
            --cursor_;
            x_.offset = std::max(0.0f, x_.offset - m.advance);
          }

          upload_vertex();
          upload_uniform();

          upload_cursor_pos(Update);
          return;
        }

        if (code_utf8 == "Delete")
        {
          if (selection_.enabled())
          {
            int char_start, char_end; 
            selection_.get_selection(char_start, char_end);
            text_.erase(meta_[char_start].index, (meta_[char_end-1].index + meta_[char_end-1].len) - meta_[char_start].index);
            cursor_ = meta_[char_start].index;
          }
          else
          {
            if (cursor_ == meta_.size()) return;

            auto& m = meta_[cursor_];

            text_.erase(m.index, m.len);
          }

          upload_vertex();
          upload_uniform();

          upload_cursor_pos(Update);
          return;
        }

        if (code_utf8 == "Return" || code_utf8 == "Enter" || code_utf8 == "NumpadEnter" ||
            code_utf8 == "Tab" || code_utf8 == "Go")
        {
          deactivate();
          unsend_keyboard();

          if (done_cb_)
            done_cb_(code_utf8 == "Go");

          return;
        }

        if (code_utf8 == "ArrowLeft" && cursor_ > 0)
        {
          --cursor_;
          if (shift_pressed_)
          {
            if (!selection_.enabled())
              selection_.fix_at(cursor_+1, meta_[cursor_].x + meta_[cursor_].advance);

            selection_.enable(ui_, cursor_, meta_[cursor_].x, cursor_, meta_[cursor_].x);
          }

          upload_cursor_pos(shift_pressed_ ? None : Update);

          return;
        }

        if (code_utf8 == "ArrowRight")
        {
          if (cursor_ >= meta_.size())
          {
            if (!shift_pressed_)
              upload_cursor_pos(Update); // deselects in case something was selected and we hit right arrow

            return;
          }

          ++cursor_;
          if (shift_pressed_)
          {
            if (!selection_.enabled())
              selection_.fix_at(cursor_-1, meta_[cursor_-1].x);

            selection_.enable(ui_, cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance,
                                   cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);
          }

          upload_cursor_pos(shift_pressed_ ? None : Update);

          return;
        }

        if (code_utf8 == "Home")
        {
          if (shift_pressed_ && !meta_.empty())
          {
            if (!selection_.enabled())
              selection_.fix_at(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);

            selection_.enable(ui_, 0, meta_[0].x, 0, meta_[0].x);
          }

          cursor_ = 0;
          upload_cursor_pos(shift_pressed_ ? None : Update);

          return;
        }

        if (code_utf8 == "End")
        {
          if (shift_pressed_ && !meta_.empty())
          {
            if (!selection_.enabled())
              selection_.fix_at(cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);

            cursor_ = meta_.size();;
            selection_.enable(ui_, cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance,
                                   cursor_, meta_[cursor_-1].x + meta_[cursor_-1].advance);
          }

          cursor_ = meta_.size();
          upload_cursor_pos(shift_pressed_ ? None : Update);

          return;
        }

      }
      else
      {
        int old_size = meta_.size();
        int deleted  = 0;

        if (selection_.enabled())
        {
          int char_start, char_end;
          selection_.get_selection(char_start, char_end);

          deleted = char_end - char_start;

          text_.erase(meta_[char_start].index,
                        (meta_[char_end-1].index + meta_[char_end-1].len) - meta_[char_start].index);

          text_.insert(meta_[char_start].index, utf8.data(), utf8.size());

          cursor_ = meta_[char_start].index;
        }
        else
        {
          if (cursor_ == 0)
            text_.insert(0, utf8.data(), utf8.size());
          else
          {
            auto& m = meta_[cursor_ - 1];
            text_.insert(m.index + m.len, utf8.data(), utf8.size());
          }
        }

        upload_vertex();
        upload_uniform();

        auto inc = std::max(0, static_cast<int>(meta_.size()) - (old_size - deleted));
        cursor_ += inc;

        upload_cursor_pos(Update);
        return;
      }
    }
  };


  std::string_view name() const noexcept
  {
    return "textentry";
  }

private:


  void select_line()
  {
    if (meta_.empty())
      return;

    auto end = meta_.size();

    selection_.enable(ui_, 0, meta_[0].x, end, meta_[end - 1].x + meta_[end - 1].advance);

    upload_selection_pos(Vibrate);
  };


  void select_word()
  {
    if (meta_.empty())
      return;

    // look left
  
    int sel_left;
       
    for (sel_left = cursor_; sel_left > 0; --sel_left)
    {
      auto& m = meta_[sel_left];
        
      if (m.len == 1 && text_[m.index] == ' ')
      {
        ++sel_left;
        break;
      }
    }
  
    // look right
  
    int sel_right;
       
    for (sel_right = cursor_; sel_right < meta_.size(); ++sel_right)
    {
      auto& m = meta_[sel_right];
  
      if (m.len == 1 && text_[m.index] == ' ')
        break;
    }
  
    cursor_ = sel_right; // position cursor at the end of the selection
    upload_cursor_pos(NoScroll|Update);

    if (sel_left != sel_right)
    {
      selection_.enable(ui_, sel_left, meta_[sel_left].x,
                            sel_right, meta_[sel_right - 1].x + meta_[sel_right - 1].advance);
      selection_.fix();
    }

    upload_selection_pos(Vibrate);
  };


  void upload_selection_pos(int mode = None)
  {
    int s, e;

    if (!selection_.get_pixels(s, e))
      return;

    if (mode & Vibrate)
      ui_event::js_vibrate(1);

    int sep = (my_height() - font_height_) / 2;

    int r = 14 * ui_->pixel_ratio_;

    gpu::int_box b{{s - r, coords_.p1.y + sep - r - r}, {s + r, coords_.p1.y + sep}};
    
    wcircle_1_->set_geometry(b);

    b = {{e - r, coords_.p1.y + sep - r - r}, {e + r, coords_.p1.y + sep}};
    
    wcircle_2_->set_geometry(b);
  };


  void upload_cursor_pos(int mode = None) // with_cursor =? update the cursor pos and optional vibrate
  {
    int offset = 0;

    if (meta_.size() == 0 || cursor_ <= 0)
    {
      cursor_ = 0;
      offset = 0;
    }
    else
    {
      if (cursor_ > meta_.size())
        cursor_ = meta_.size();

      auto& m = meta_[cursor_ - 1];

      offset = m.x + m.advance;
    }

    // scroll if cursor is off widget

    bool do_upload = false;

    if (!(mode & NoScroll))
    {
      auto viewable_x = my_width() - (2 * x_space_);

      if (offset < x_.offset)
      {
        x_.offset = offset;
        do_upload = true;
      }

      if (offset > x_.offset + viewable_x)
      {
        x_.offset = offset - viewable_x;
        do_upload = true;
      }

      if (x_.offset > 0 && x_.offset + viewable_x > meta_.back().x + meta_.back().advance)
      {
        x_.offset = std::max(0.0f, meta_.back().x + meta_.back().advance - viewable_x);
        do_upload = true;
      }
    }

    if (do_upload)
      upload_uniform();

    if (mode & Update)
    {
      selection_.disable();

      int total_off = offset - static_cast<int>(x_.offset) + x_space_;

      wbox_c_->set_offset({total_off, 0});
 
      int s = (my_height() - font_height_) / 2;

      int r = 14 * ui_->pixel_ratio_;

      gpu::int_box b{{coords_.p1.x + total_off - r, coords_.p1.y + s - r - r},
            {coords_.p1.x + total_off + r, coords_.p1.y + s}};

      wcircle_->set_geometry(b);

      if (mode & Vibrate)
        ui_event::js_vibrate(1);
    }
  };


  // returns the meta index position given a screen pixel position

  int get_pos_from_screen(int screen_pos)
  {
    return get_pos_from_distance(screen_pos - (coords_.p1.x + x_space_) + x_.offset);
  };

  // returns the meta index position given a pixel offset from the start

  int get_pos_from_distance(int pixel_offset)
  {
    int i = 0;

    for (auto& ma : meta_)  // fixme use binary search
    {
      if (ma.x + ma.advance/2 > pixel_offset)
        return i;

      ++i;
    }

    return i;
  };


  // returns the screen pixel offset of a cursor position

  int get_pixel_offset_from_cursor(int cursor)
  {
    if (cursor == 0)
      return coords_.p1.x + x_space_;

    int offset = meta_[cursor - 1].x + meta_[cursor - 1].advance;

    return coords_.p1.x + x_space_ + offset - x_.offset;
  };


  void upload_vertex()
	{
    meta_.clear();
    std::vector<float> vertex;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * scale_;

    gpu::float_box vpos;

    if (password_)
    {
      std::string stars(font::utf8_strlen(text_.c_str()), '*');
      vpos = ui_->font_->generate_vertex(stars, vertex, font_scale, &meta_);
    }
    else
      vpos = ui_->font_->generate_vertex(text_, vertex, font_scale, &meta_);

    max_width_ = ceil(vpos.p2.x);
    y_low_ = vpos.p1.y;
    y_high_ = vpos.p2.y;

    // align the coords so the center of the text is at (0,0) - allows for rotation

    int x_shift = max_width_ / 2;
    int y_shift = font_height_ / 2;

    for (int i = 0; i < vertex.size(); i += 6) {
      vertex[i]   -= x_shift;
      vertex[i+1] -= y_shift;
    }

    // align it to the box

    auto w = coords_.p2.x - coords_.p1.x - (2 * x_space_);
    auto h = (coords_.p2.y - coords_.p1.y) / 2.0 - (ui_->font_->get_min_y() * font_scale);

    if (align_ == gpu::align::LEFT) {
      x_align_offset_ = x_shift + x_space_;
      y_align_offset_ = h;
    }
    if (align_ == gpu::align::RIGHT) {
      x_align_offset_ = w - x_shift;
      y_align_offset_ = h;
    }
    if (align_ == gpu::align::CENTER) {
      x_align_offset_ = w / 2.0;
      y_align_offset_ = h;
    }

    vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, 6 * sizeof(float))  ;

    if (meta_.empty())
      set_x_size(0);
    else
      set_x_size(meta_.back().x + meta_.back().advance + (2 * x_space_));
  };


  void upload_uniform()
  {
    auto ptr = uniform_buffer_.allocate_staging(1, sizeof(shader_text::ubo));
  
    shader_text::ubo* u = new (ptr) shader_text::ubo;
 
    u->offset = { { static_cast<float>(coords_.p1.x + x_align_offset_ - x_.offset) },
                   { static_cast<float>(coords_.p1.y   + y_align_offset_) }, { 0.0f }, { 0.0f   } };

    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };
    u->scale  = { 1.0f, 1.0f };
  
    uniform_buffer_.upload();

    int sep = ((coords_.p2.y - coords_.p1.y) - font_height_)/2.0;

    selection_.set_offset(ui_, coords_.p1.x + x_space_ - x_.offset, coords_.p1.y + sep);
  };


  inline void set_active(bool status) noexcept
  {
    active_ = status;

    active(active_);
  };


protected:

  buffer vertex_buffer_;
  buffer uniform_buffer_;

  gpu::align align_;

  std::string               text_; // utf8
  std::vector<font::offset> meta_; // actual glyphs with: x_pos, index into text_ and byte len

  std::array<gpu::color, 2> color_;
  gpu::rotation rotation_;
  float scale_;

  int font_height_;

  int x_align_offset_{0};
  int y_align_offset_{0};

  int y_low_{0}, y_high_{0}, max_width_{0}; // y is relative to font baseline

  int cursor_{0}, x_space_{10};

  bool active_{false};
  bool password_{false}; // is set display * per character rather than actual text

  bool shift_pressed_{false};
  bool is_touch_{false};
  int  touch_type_{0}; // 0 => normal/cursor, 1 => left selection, 2 => right selection
  int auto_scroll_{0}; // 0 for none, -ve for left, +ve for right

  float hold_timer_{0}; // to display touch popus on hold ater a timeout
  float hold_timer_actioned_{false}; // did something after hold_timer expired

  std::shared_ptr<widget_box> wbox_stencil_; // for stencil of text entry area

  std::shared_ptr<widget_box> wbox_;    // line

  std::shared_ptr<widget_box> wbox_c_;  // cursor

  std::shared_ptr<widget_circle> wcircle_;   // a grab handle for touch interfaces to move cursor
  std::shared_ptr<widget_circle> wcircle_1_; // a grab handle for touch interfaces to move selection
  std::shared_ptr<widget_circle> wcircle_2_; // a grab handle for touch interfaces to move selection

  std::shared_ptr<widget_copy_paste> wtext_copy_paste_;

  selection selection_;

  std::function<void (bool touch)> done_cb_; // called on Return/Tab/Go
  std::function<void ()> active_cb_;         // called on activation

}; // widget_textentry

} // namespace plate
