#pragma once

#include <vector>
#include <deque>
#include <map>
#include <functional>
#include <cmath>

#include "ui_event_destination.hpp"

/*

  ui event destination autoscroll.

  Widgets inherit from this to add themselves as a ui event destination with auto scroll capability,
  they will then receive calls such as:

  0,0 is bottom left - ?? confirm - might be top left!

    ui_out, XXX

*/

namespace plate {


class ui_event_destination_autoscroll : public ui_event_destination
{

public:

  enum class scroll { None, X, Y, XY };

  enum class Axis { X, Y };  // The axis something operates on

/*
  ui_event_destination_autoscroll(std::shared_ptr<state>& s) :
    ui_event_destination(s)
  {
  };


  ui_event_destination_autoscroll(std::shared_ptr<state>& s, const gpu::int_box& coords, std::uint32_t prop) :
    ui_event_destination(s, coords, prop)
	{
	};


  ui_event_destination_autoscroll(std::shared_ptr<state>& s, const gpu::int_box& coords,
                                              std::uint32_t prop, ui_event_destination* parent) :
    ui_event_destination(s, coords, prop, parent)	
  {
	};


  ui_event_destination_autoscroll(std::shared_ptr<state>& s, std::uint32_t prop, ui_event_destination* parent) :
    ui_event_destination(s, prop, parent)
  {
  };


  ~ui_event_destination_autoscroll()
  {
  };
*/

  void set_scroll(scroll s)
  {
    scroll_x_ = s == scroll::X || s == scroll::XY;
    scroll_y_ = s == scroll::Y || s == scroll::XY;
  };


  void set_x_size(std::int32_t s) { x_.size = s; };
  void set_y_size(std::int32_t s) { y_.size = s; };

  void set_x_offset(float offset) { x_.offset = offset; };
  void set_y_offset(float offset) { y_.offset = offset; };


  virtual void scroll_update() noexcept { }; // for when the display needs updating due to scroll movement

  virtual void scroll_click(gpu::int_vec2 pos)  noexcept { }; // to send through clicks/presses


  void pre_display()
  {
    if (scroll_x_)
      pre_display_update(x_);

    if (scroll_y_)
      pre_display_update(y_);
  };


  void stop_scrolling(bool total = false) // called by a parent that wants scrolling to stop
  {
    if (!total &&fabs(x_.speed) < 200) // feels nicer to let slow scrolling continue
      return;

    x_.mode = anim_mode::STILL;
    x_.speed = 0;

    if (get_past_height(Axis::X, x_.offset) != 0) // return home
    {
      x_.mode = anim_mode::RETURN;
      x_.start_offset = x_.offset;
      x_.step = 0;
    }
  }


  // mouse input callbacks are made to these:

  bool ui_mouse_position_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    if (m.swipe)
    {
      if (scroll_x_)
        x_.offset -= m.delta.x;

      if (scroll_y_)
        y_.offset += m.delta.y;

      scroll_update();
    }

    return true;
  }


  void ui_mouse_button_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    if (m.id & ui_event::MouseButtonLeft)
    {
      mouse_down_ = true;

      if (scroll_x_)
      {
        if (fabs(x_.speed) > 200)
          did_stop_anim_ = true;
        else
          did_stop_anim_ = false;

        x_.mode  = anim_mode::STILL;
        x_.speed = 0;
      }
      if (scroll_y_)
      {
        if (fabs(y_.speed) > 200)
          did_stop_anim_ = true;
        else
          did_stop_anim_ = false;

        y_.mode  = anim_mode::STILL;
        y_.speed = 0;
      }

      return;
    }

    if (mouse_down_ && !(m.id & ui_event::MouseButtonLeft))
    {
      mouse_down_ = false;

      if (m.click && !did_stop_anim_)
        scroll_click(m.start_pos);

      if (scroll_x_)
      {
        if (get_past_height(Axis::X, x_.offset) != 0) // return home
        {
          x_.mode = anim_mode::RETURN;
          x_.start_offset = x_.offset;
          x_.step = 0;
          x_.speed = 0;
        }
        else  // apply flow movement if there is some
        {
          if (m.speed.x == 0)
          {
            x_.mode = anim_mode::STILL;
          }
          else
          {
            x_.mode  = anim_mode::FLOW;
            x_.speed = m.speed.x;

            if (fabs(x_.speed) > 5000)
              x_.speed *= 2;
          }
        }
      }
      if (scroll_y_)
      {
        if (get_past_height(Axis::Y, y_.offset) != 0) // return home
        {
          y_.mode = anim_mode::RETURN;
          y_.start_offset = y_.offset;
          y_.step = 0;
          y_.speed = 0;
        }
        else  // apply flow movement if there is some
        {
          if (m.speed.y == 0)
          {
            y_.mode = anim_mode::STILL;
          }
          else
          {
            y_.mode  = anim_mode::FLOW;
            y_.speed = -m.speed.y;

            if (fabs(y_.speed) > 5000)
              y_.speed *= 2;
          }
        }
      }

      return;
    }
  }


  virtual bool ui_scroll(double x_delta, double y_delta) noexcept
  {
    if (scroll_y_)
    {
      if (y_.mode == anim_mode::RETURN) return true;
      if (get_past_height(Axis::Y, y_.offset)) return true;

      if (y_delta < 0 && y_.offset == 0) return true;
      if (y_delta > 0 && y_.offset == get_max_offset_y()) return true;

      if (y_.mode == anim_mode::FLOW)
        y_.speed -= y_delta * 2;
      else
      {
        y_.mode  = anim_mode::PREFLOW;
        y_.speed = -y_delta * 4;
      }

      scroll_update();
    }

    return true;
  }


  bool ui_touch_update(int id) noexcept
  {
    auto& m = ui_->touch_metric_[id];

    if (m.st == ui_event::TouchEventDown)
    {
      mouse_down_ = true;

      if (scroll_x_)
      {
        if (fabs(x_.speed) > 200)
          did_stop_anim_ = true;
        else
          did_stop_anim_ = false;

        x_.mode  = anim_mode::STILL;
        x_.speed = 0;
      }
      if (scroll_y_)
      {
        if (fabs(y_.speed) > 200)
          did_stop_anim_ = true;
        else
          did_stop_anim_ = false;

        y_.mode  = anim_mode::STILL;
        y_.speed = 0;
      }

      return true;
    }

    if (m.st == ui_event::TouchEventUp)
    {
      mouse_down_ = false;

      if (m.click && !did_stop_anim_)
        scroll_click(m.start_pos);

      if (scroll_x_)
      {
        if (get_past_height(Axis::X, x_.offset) != 0) // return home
        {
          x_.mode = anim_mode::RETURN;
          x_.start_offset = x_.offset;
          x_.step = 0;
          x_.speed = 0;
        }
        else  // apply flow movement if there is some
        {
          if (m.speed.x == 0)
          {
            x_.mode = anim_mode::STILL;
          }
          else
          {
            x_.mode  = anim_mode::FLOW;
            x_.speed = m.speed.x;

            if (fabs(x_.speed) > 5000)
              x_.speed *= 2;
          }
        }
      }
      if (scroll_y_)
      {
        if (get_past_height(Axis::Y, y_.offset) != 0) // return home
        {
          y_.mode = anim_mode::RETURN;
          y_.start_offset = y_.offset;
          y_.step = 0;
          y_.speed = 0;
        }
        else  // apply flow movement if there is some
        {
          if (m.speed.y == 0)
          {
            y_.mode = anim_mode::STILL;
          }
          else
          {
            y_.mode  = anim_mode::FLOW;
            y_.speed = -m.speed.y;

            if (fabs(y_.speed) > 5000)
              y_.speed *= 2;
          }
        }
      }

      return true;
    }

    if (m.st == ui_event::TouchEventMove)
    {
      if (!mouse_down_) return true;

      if (m.swipe)
      {
        if (scroll_x_)
          x_.offset += -m.delta.x;
        if (scroll_y_)
          y_.offset += m.delta.y;
        scroll_update();
      }
    }

    return true;
  }

  // return 0 if the current view is displaying in-range,
  // otherwise >0 if past the end or <0 if past the beginning

  float get_past_height(Axis a, float offset)
  {
    if (a == Axis::X)
    {
      if (offset < 0) return offset;

      float max_pos = get_max_offset_x();

      if (offset > max_pos) return offset - max_pos;

      return 0;
    }
    else
    {
      if (offset < 0) return offset;

      float max_pos = get_max_offset_y();

      if (offset > max_pos) return offset - max_pos;

      return 0;
    }
  }


  int get_max_offset_x()
  {
    return std::max(0, x_.size - (coords_.p2.x - coords_.p1.x));
  }
  int get_max_offset_y()
  {
    return std::max(0, y_.size - (coords_.p2.y - coords_.p1.y));
  }


  // keyboard

//  virtual void key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8, ui_event::KeyMod) { }

//  virtual void deactivate() {}



protected:


  enum class anim_mode { STILL,       // window is not automatically moving
                         FLOW,        // window is flowing
                         RETURN,      // window is returning from an out of bounds position
                         ZOOM,        // window is is a zoomed mode
                         PREFLOW      // window is about to enter a flow, it is delayed so that the frame diff
                                      //        times are correct
                        };

  struct scroll_state
  {
    scroll_state(Axis a) : axis(a) {}

    Axis         axis;
    std::int32_t size  {0};
    float        offset{0};
    anim_mode    mode  {0};
    float        speed {0};

    // for returning from out of bounds:

    std::int32_t step        {0};
    std::int32_t start_offset{0};
  };


  void pre_display_update(scroll_state& ss)
  {
    if (ss.mode == anim_mode::STILL)
      return;

    if (ss.mode == anim_mode::PREFLOW)
    {
      ss.mode = anim_mode::FLOW;
      ui_->do_draw();
      return;
    }

    if (ss.mode == anim_mode::FLOW)
    {
      float speed = fabs(ss.speed);
      float accel;
  
      if (speed < 100)
          accel = 25.0;
      else
        if (speed < 200)
          accel = 100.0;
        else
          if (speed < 2000) //      from_accel + to_accel-from_accel*frac
            accel = 100.0 + (1900 - (1900 * ((cos(M_PI * (fabs(ss.speed) - 200)/1800.0) + 1.0)/2.0)));
          else
            accel = 2000.0;

      accel *= (ss.speed > 0 ? -1 : 1);
  
      auto past_view = get_past_height(ss.axis, ss.offset);
  
      if (past_view < 0 && ss.speed > 0)
        accel = -6000 - (15 * ss.speed);

      if (past_view > 0 && ss.speed < 0)
        accel =  6000 - (15 * ss.speed);
  
      //float distance = -((ss.speed * 1.0/60.0) + 0.5 * accel * (1.0/60.0 * 1.0/60.0));
      float distance = -((ss.speed * ui_->frame_diff_) + 0.5 * accel * (ui_->frame_diff_ * ui_->frame_diff_));
  
      ss.offset += distance;
  
      //float new_speed = ss.speed + accel * 1.0/60.0;
      float new_speed = ss.speed + accel * ui_->frame_diff_;
  
      if (new_speed == 0 || (ss.speed > 0 && new_speed < 0) || (ss.speed < 0 && new_speed > 0))
      {
        ss.mode = anim_mode::STILL;
  
        if (get_past_height(ss.axis, ss.offset) != 0)
        {
          ss.mode = anim_mode::RETURN;
          ss.start_offset = ss.offset;
          ss.step = 0;
        }
      } else
        ss.speed = new_speed;
    }

    if (ss.mode == anim_mode::RETURN)
    {
      ss.step++;

      float total_animate = get_past_height(ss.axis, ss.start_offset);

      if (total_animate == 0)
        ss.mode = anim_mode::STILL;
      else
      {
        int s;
        constexpr float half_way = 12.0;

        if (ss.step < half_way)
          s = sin((half_way - ss.step)/half_way * M_PI/2) * total_animate/2.0 + total_animate/2.0;
        else
          s = (1.0 - sin((ss.step - half_way)/half_way * M_PI/2)) * total_animate/2.0;

        ss.offset = ss.start_offset - total_animate + s;

        if (ss.step >= half_way * 2)
          ss.mode = anim_mode::STILL;
      }
    }

    scroll_update();

    if (ss.mode != anim_mode::STILL)
      ui_->do_draw();
  }



  bool scroll_x_{false};
  bool scroll_y_{false};

  gpu::int_box virtual_coords_;


  scroll_state x_{Axis::X};
  scroll_state y_{Axis::Y};

  bool mouse_down_{false};
  bool did_stop_anim_{false}; // used to stop clicks going through when clicking during an anim


}; // class ui_event_destination_autoscroll


} // namespace plate
