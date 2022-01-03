#pragma once

#include <vector>
#include <set>
#include <deque>
#include <map>
#include <memory>
#include <cmath>
#include <functional>

#include "anim.hpp"
#include "projection.hpp"
#include "alpha.hpp"
#include "ui_event.hpp"
#include "../../gpu/gpu.hpp"
#include "../log.hpp"


/*
    ui manages the state of a user interface window

    widgets/objects of type T can be added, removed and drawn

*/

namespace plate {

class shader_basic;
//class shader_compute_template;
class shader_object;
//class shader_object_instanced;
//class shader_text;
class shader_text_msdf;
class shader_texture;
//class shader_example_geom;
//class shader_circle;
class shader_rounded_box;

class font;

class scissor;
class ui_anim;

extern void process_ui_anims(const std::vector<std::weak_ptr<ui_anim>>&) noexcept;

template<class T>
class ui : public std::enable_shared_from_this<ui<T>>
{

public:

  enum Prop
  {
    None     = 0,
    Input    = 1<<0, // the object can accept mouse/keyboard or touch inputs
    Swipe    = 1<<1, // the object supports swipe movements
    Display  = 1<<2, // the object should be auto displayed
    Priority = 1<<3  // This means LOW priority - ie add widget to the bottom of the stack
  };


  ui()
  {
    do_draw();

    fragment_ = get_fragment();
  }


  ui(std::string name) :
    name_(name)
  {
    do_draw();
    plate::ui_event::add_dest(this, name_);

    fragment_ = get_fragment();
  }


  ~ui()
  {
    if (name_ != "")
      plate::ui_event::rm_dest(this, name_);
  }


  inline void fragment_change(const std::string& fragment) noexcept
  {
    if (fragment == fragment_)
      log_debug(FMT_COMPILE("ignoring switch to fragment: {}"), fragment);
    else
    {
      log_debug(FMT_COMPILE("switch to fragment: {}"), fragment);

      if (fragment_cb_)
        fragment_cb_(fragment);

      fragment_ = fragment;
    }
  }


  inline void set_darkmode(bool v) noexcept
  {
    if (v)
    {
      if (color_mode_ == 1) // already set
        return;

      color_mode_ = 1;
    }
    else
    {
      if (color_mode_ == 0) // already set
        return;

      color_mode_ = 0;
    }

    glClearColor(bg_color_[color_mode_].r, bg_color_[color_mode_].g, bg_color_[color_mode_].b, bg_color_[color_mode_].a);
 
    // notify widgets of change

    if (top_widget_)
      top_widget_->tree_update_color_mode();

    do_draw();
  }


  inline bool is_darkmode() const noexcept
  {
    return color_mode_ == 1;
  }


  void set_pixel_ratio(double r) noexcept
  {
    prev_pixel_ratio_ = pixel_ratio_;
    pixel_ratio_ = r;
  }


  inline bool pixel_ratio_changed() const noexcept
  {
    return pixel_ratio_ != prev_pixel_ratio_;
  }


  void set_font_size(float s) noexcept
  {
    prev_font_size_ = font_size_;
    font_size_ = s;
  }


  inline float get_font_size() const noexcept
  {
    return font_size_;
  }


  inline bool font_size_changed() const noexcept
  {
    return font_size_ != prev_font_size_;
  }


  inline bool any_size_changed() const noexcept
  {
    return pixel_ratio_changed() || font_size_changed();
  }


  void set_name(std::string name) noexcept
  {
    name_ = name;
    plate::ui_event::add_dest(this, name_);
  }


  void unhook(T* t) noexcept
  {
    // remove from any transmit pipelines
        
    unsend_all(t);
    unsend_keyboard(t);
  
    if (mouse_metric_.current == t)
      mouse_metric_.current = nullptr;
  
    if (wheel_metric_.current == t)
      wheel_metric_.current = nullptr;
    
    for (int i = 0; i < MAX_TOUCHES; ++i)
      if (touch_metric_[i].current == t)
        touch_metric_[i].current = nullptr;

    //RM:if (multi_touch_metric_.current == t)
      //multi_touch_metric_.current = nullptr;
  };


  inline void unsend_all(T* t) noexcept
  {
    if (send_all_ == t)
      send_all_ = nullptr;
  };


  inline void unsend_all() noexcept
  {
    send_all_ = nullptr;
  };


  inline void send_all(T* t) noexcept
  {
    log_debug(FMT_COMPILE("sending all to: {}"), reinterpret_cast<std::size_t>(t));
    send_all_ = t;
  };


  inline void send_keyboard(const std::shared_ptr<T>& t) noexcept
  {
    if (send_keyboard_events_)
      send_keyboard_events_->deactivate();

    send_keyboard_events_ = t.get();
  };


  bool unsend_keyboard(T* t) noexcept
  {
    if (send_keyboard_events_ == t)
    {
      send_keyboard_events_ = nullptr;
      return true;
    }

    return false;
  };


  void close_keyboard() noexcept
  {
    if (send_keyboard_events_)
      send_keyboard_events_->deactivate();

    send_keyboard_events_ = nullptr;
  };


  inline void disable() noexcept
  {
    enabled_ = false;
  };


  inline void enable() noexcept
  {
     enabled_ = true;
  };


  void display_all()
  {
    projection_.reset();

    if (anim_.process(frame_diff_, frame_time_correct_))
      do_draw();

    if (!anims_.empty())
    {
      auto a = anims_;

      plate::process_ui_anims(a);
      
      if (!anims_.empty())
        do_draw();
    }

    for (auto& w : to_disconnect_)
      w->disconnect_from_parent();
      
    to_disconnect_.clear();

    in_draw_ = true;

    if (top_widget_)
      top_widget_->tree_display();

    in_draw_ = false;
  }


  void paste()
  {
    ui_event::paste(name_.data() + 1, name_.size() - 1);
  }


  inline void open_url(std::string_view path) noexcept
  {
    ui_event::open_url(path);
  }


  inline void set_fragment(std::string_view fragment) noexcept
  {
    if (fragment != fragment_)
    {
      fragment_ = fragment;
      ui_event::set_fragment(fragment);
    }
  }

  
  inline std::string get_fragment() noexcept
  {
    return ui_event::get_fragment();
  }


  std::string get_tz() noexcept
  {
    return ui_event::get_tz();
  }


  void do_reshape() noexcept
  {
    if (top_widget_)
      top_widget_->tree_reshape();

    do_draw();
  }


  void incoming_focus(bool focus) noexcept
  {
    is_focused_ = focus;
  }


  inline void set_focus() noexcept
  {
    if (!is_focused_)
    {
      auto n = name_.substr(1);
      ui_event::set_focus(n);
    }
  }


  void incoming_mouse_over(bool over) noexcept
  {
    //log_debug(FMT_COMPILE("Got mouse_over: {} for: {}"), over, name_);
  }


  // incoming receives updates from: a source with an id, an x and y position,
  // whether an event has changed, and any force detail

  void incoming_key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8,
                                                                                      ui_event::KeyMod mod) noexcept
  {
    if (event == ui_event::KeyEvent::KeyEventDown && code_utf8 == "Escape")
      top_widget_->tree_print();

    if (send_keyboard_events_)
    {
      do_draw();

      send_keyboard_events_->key_event(event, utf8, code_utf8, mod);
    }
  };


  void incoming_paste_event(std::string_view s) noexcept
  {
    if (send_keyboard_events_)
    {
      do_draw();

      send_keyboard_events_->key_event(ui_event::KeyEventDown, s, "", ui_event::KeyModNone);
    }
  };


  void incoming_virtual_keyboard_quit() noexcept
  {
    if (send_keyboard_events_)
    {
      send_keyboard_events_->deactivate();
      send_keyboard_events_ = nullptr;
    }
  }


  void incoming_mouse_position(int x, int y) noexcept
  {
    do_draw();
  
    // store the delta from its last position
  
    if (mouse_metric_.pos.x != -1)
    {
      mouse_metric_.delta.x = x - mouse_metric_.pos.x;
      mouse_metric_.delta.y = y - mouse_metric_.pos.y;
    }
  
    // store its current position
  
    mouse_metric_.pos.x = x;
    mouse_metric_.pos.y = y;
  
    bool enable_button_move_find_widget = false; // whether we can attach a widget to a mouse move with button down
  
    // store speed if mouse button pressed
  
    if (mouse_metric_.id)
    {
      const double now = ui_event::now();
  
      mouse_metric_.history_x.push_back({ now, mouse_metric_.delta.x });
      mouse_metric_.history_y.push_back({ now, mouse_metric_.delta.y });
    
      const auto sx = mouse_metric_.start_pos.x;
      const auto sy = mouse_metric_.start_pos.y;

      if ( !mouse_metric_.swipe && ((x-sx)*(x-sx) + (y-sy)*(y-sy) > MOVE_STICKY) )
      {
        mouse_metric_.swipe = true;
        mouse_metric_.start_pos2.x = x;
        mouse_metric_.start_pos2.y = y;
  
        if (mouse_metric_.current && !mouse_metric_.current->supports_swipe()) // does the current widget not support swipe?
        {
          mouse_metric_.current->ui_out();
          mouse_metric_.current = nullptr;
          enable_button_move_find_widget = true; // allow assigning another widget to this move event
        }
      }
    }
  
    // send to widgets
  
    // a send_all overrides all actions
  
    if (send_all_)
    {
      if (send_all_->ui_mouse_position_update()) // return false to relinquish control
        return;
      else
        send_all_ = nullptr;
    }
  
    if (mouse_metric_.id) // when a button is pressed - send events to the button owner if any
    {
      if (mouse_metric_.current)
      {
        if (!mouse_metric_.current->supports_swipe()) // nothing to do here as swipe is less than MOVE_STICKY
          return;

        mouse_metric_.current->ui_mouse_position_update();

        return;
      }
      else // nothing is current
      {
        if (enable_button_move_find_widget) // switch to the widget which handles swipe if any
        {
          mouse_metric_.current  = top_widget_->tree_find_input(x, y, mouse_metric_.swipe);

          if (mouse_metric_.current)
          {
            mouse_metric_.current->ui_mouse_button_update();
            mouse_metric_.current->ui_mouse_position_update();
          }
        }
        return;
      }
    }

    // no buttons pressed, find the widget under the mouse position and send it a position update

    auto w = top_widget_->tree_find_input(x, y, false);

    if (mouse_metric_.current && (w != mouse_metric_.current)) // send a ui_out to previous widget
      mouse_metric_.current->ui_out();

    if (w)
    {
      mouse_metric_.current = w;
      mouse_metric_.current->ui_mouse_position_update();
    }
  }


  void incoming_mouse_button(ui_event::MouseButtonEvent event, ui_event::MouseButton button, ui_event::KeyMod mods) noexcept
  {
    if (event == ui_event::MouseButtonEventClick || event == ui_event::MouseButtonEventDoubleClick)
      return;

    do_draw();
  
    // update the mouse status, if it's first time down then setup speed indicators, if they're all up apply speed
  
    double now = ui_event::now();
    mouse_metric_.double_click = false;
    mouse_metric_.click        = false;
  
    if (event == ui_event::MouseButtonEventDown)
    {
      set_focus();

      if (mouse_metric_.id == 0) // no other mouse buttons are currently down
      {
        mouse_metric_.id = button;
  
        mouse_metric_.swipe        = false;

        mouse_metric_.history_x.clear();
        mouse_metric_.history_y.clear();
        mouse_metric_.speed = {0, 0};
        mouse_metric_.start_pos.x = mouse_metric_.pos.x;
        mouse_metric_.start_pos.y = mouse_metric_.pos.y;
        mouse_metric_.delta = {0, 0};
        mouse_metric_.prev_down_start = mouse_metric_.down_start;
        mouse_metric_.down_start = now;

        auto w = top_widget_->tree_find_input(mouse_metric_.pos.x, mouse_metric_.pos.y, false);

        if (mouse_metric_.current && (w != mouse_metric_.current)) // send a ui_out to previous widget
          mouse_metric_.current->ui_out();

        mouse_metric_.current = w;
      }
      else // another button is already down, so simply add it
      {
        mouse_metric_.id |= button;
      }

      if (send_all_)
      {
        mouse_metric_.current = nullptr;
        send_all_->ui_mouse_button_update();
      }
      else
      {
        if (mouse_metric_.current)
          mouse_metric_.current->ui_mouse_button_update();
      }

      return;
    }
  
    if (event == ui_event::MouseButtonEventUp)
    {
      mouse_metric_.click        = !mouse_metric_.swipe && ((now - mouse_metric_.down_start)      <= 200);
      mouse_metric_.double_click = !mouse_metric_.swipe && ((now - mouse_metric_.prev_down_start) <= 400);

      mouse_metric_.id &= ~button;
  
      if (mouse_metric_.id == 0) // no buttons pressed any more, so calc speed
      {
        int distance = 0;
  
        while (!mouse_metric_.history_x.empty())
        {
          auto entry = mouse_metric_.history_x.back();
  
          if (now - entry.first > 100)// only look at very recent movement
            break;
  
          distance += entry.second;
  
          mouse_metric_.history_x.pop_back();
        }
  
        mouse_metric_.history_x.clear();
        mouse_metric_.speed.x = distance/0.11;
  
        distance = 0;

        while (!mouse_metric_.history_y.empty())
        {
          auto entry = mouse_metric_.history_y.back();

          if (now - entry.first > 100) // only look at very recent movement, 100ms
            break;
  
          distance += entry.second;
  
          mouse_metric_.history_y.pop_back();
        }
  
        mouse_metric_.history_y.clear();
        mouse_metric_.speed.y = distance/0.11;
  
        mouse_metric_.swipe = false;
      }

      if (send_all_)
      {
        send_all_->ui_mouse_button_update();
      }
      else
      {
        if (mouse_metric_.current)
        {
          mouse_metric_.current->ui_mouse_button_update();

          if (mouse_metric_.id == 0)
            mouse_metric_.current = nullptr;
        }
      }

      return;
    }
  }


  void incoming_touch(ui_event::TouchEvent event, int id, int x, int y) noexcept
  {
    do_draw();

    if (id < 0 || id >= MAX_TOUCHES)
      return;

    auto& m = touch_metric_[id];
    m.st = event;

    const double now = ui_event::now();

    switch (event)
    {
      case ui_event::TouchEventDown:
      {
        // set_focus(); // causes a jump as it moves the widget fully on the screen, disable for now

        m.id = id;

        m.pos.x = x;
        m.pos.y = y;

        m.click        = false;
        m.double_click = false;
        m.swipe        = false;
        m.multi        = false;

        m.history_x.clear();
        m.history_y.clear();
        m.speed = {0, 0};
        m.start_pos.x = x;
        m.start_pos.y = y;
        m.delta = {0, 0};
        m.prev_down_start = m.down_start;
        m.down_start = now;

        if (send_all_)
        {
          if (send_all_->ui_touch_update(id))
            return;

          send_all_ = nullptr;
        }

        // find the widget to send this to

        auto w = top_widget_->tree_find_input(x, y, false);

        if (w)
        {
          if (w->ui_touch_update(id))
          {
            m.current = w;

            // has this formed a multi touch?

            for (auto& n : touch_metric_)
            {
              if (n.st != 0 && n.id != m.id && n.current == m.current)
              {
                n.multi = true;
                m.multi = true;

                multi_touch_metric_.dist = 0;
              }
            }
          }
        }

        break;
      }
      case ui_event::TouchEventUp:
      {
        m.click        = !m.swipe && ((now - m.down_start)      <= 200);
        m.double_click = !m.swipe && ((now - m.prev_down_start) <= 400);

        // calc speed

        int x_distance = 0;

        if (m.swipe)
        {
          while (!m.history_x.empty())
          {
            auto entry = m.history_x.back();

            if (now - entry.first > 100) // only look at very recent movement
              break;

            x_distance += entry.second;

            m.history_x.pop_back();
          }
        }

        m.history_x.clear();
        m.speed.x = x_distance/0.11;

        int y_distance = 0;

        if (m.swipe)
        {
          while (!m.history_y.empty())
          {
            auto entry = m.history_y.back();

            if (now - entry.first > 100) // only look at very recent movement, 100ms
              break;

            y_distance += entry.second;

            m.history_y.pop_back();
          }
        }

        m.history_y.clear();
        m.speed.y = y_distance/0.11;

        if (x_distance*x_distance + y_distance*y_distance < 12*12) // probably caused by a fat finger
        {
          m.speed.x = 0;
          m.speed.y = 0;
        }

        if (send_all_)
        {
          if (send_all_->ui_touch_update(id))
            return;

          send_all_ = nullptr;
        }

        if (m.current)
        {
          m.current->ui_touch_update(id);
          m.current = nullptr;
        }

        m.st = 0;

        break;
      }

      case ui_event::TouchEventMove:
      {
        m.delta.x = x - m.pos.x;
        m.delta.y = y - m.pos.y;

        m.pos.x = x;
        m.pos.y = y;

        //multitouch

        if (m.multi)
        {
          for (auto& n : touch_metric_)
          {
            if (n.st != 0 && n.id != m.id && n.current == m.current)
            {
              int xdiff = m.pos.x - n.pos.x;
              int ydiff = m.pos.y - n.pos.y;

              float dist = std::sqrt(static_cast<double>(xdiff*xdiff + ydiff*ydiff));

              multi_touch_metric_.delta = dist / multi_touch_metric_.dist;
              multi_touch_metric_.dist  = dist;

              if (m.current && multi_touch_metric_.delta != std::numeric_limits<double>::infinity())
                m.current->ui_zoom_update();

              return;
            }
          }
        }

        //double now = ui_event::now();

        m.history_x.push_back({ now, m.delta.x });
        m.history_y.push_back({ now, m.delta.y });

        if (send_all_)
        {
          if (send_all_->ui_touch_update(id))
            return;

          send_all_ = nullptr;
        }

        bool enable_touch_move_find_widget = false; // if the user is swiping and the current widget doesn't support swipe, search

        const auto sx = m.start_pos.x;
        const auto sy = m.start_pos.y;

        if ( !m.swipe && ((x-sx)*(x-sx) + (y-sy)*(y-sy) > MOVE_STICKY) )
        {
          m.swipe = true;
          m.start_pos2.x = x;
          m.start_pos2.y = y;

          if (m.current && !m.current->supports_swipe()) // try and find another widget
          {
            m.current->ui_out();
            m.current = nullptr;
            enable_touch_move_find_widget = true;
          }
        }

        if (m.current)
        {
          if (m.current->ui_touch_update(id))
            return;

          m.current = nullptr;
          enable_touch_move_find_widget = true;
        }

        if (!enable_touch_move_find_widget)
          return;

        // find a widget to send this move to

        auto w = top_widget_->tree_find_input(x, y, true);
        if (w)
        {
          // send through a down first

          m.st = ui_event::TouchEventDown;
          w->ui_touch_update(id);

          m.st = ui_event::TouchEventMove;

          if (w->ui_touch_update(id))
          {
            m.current = w;
            return;
          }
        }

        break;
      }

      case ui_event::TouchEventCancel:
      {
        if (m.current)
          m.current->ui_touch_update(id);

        return;
      }

      default:
        log_debug("Bad touch event");
    }
  }


  bool incoming_scroll_wheel(double x_delta, double y_delta) noexcept
  {
    do_draw();

    auto& x = mouse_metric_.pos.x;
    auto& y = mouse_metric_.pos.y;

    auto w = top_widget_->tree_find_input(x, y, false);

    if (w)
    {
      w->ui_scroll(x_delta, y_delta);
    }

    return true;
  }


  // window interfaces

  inline void set_x_offset(float x_offset) noexcept { x_offset_ = x_offset; }
  inline void set_y_offset(float y_offset) noexcept { y_offset_ = y_offset; }

  inline float get_x_offset() const noexcept { return x_offset_; }
  inline float get_y_offset() const noexcept { return y_offset_; }


  void set_frame_time(const double t) noexcept
  {
    frame_time_correct_ = (frame_id_ - prev_frame_id_ == 1);
      
    prev_frame_time_ = frame_time_;
    frame_time_ = t;
    frame_diff_ = (frame_time_ - prev_frame_time_) / 1'000.0; // convert to seconds from ms
    prev_frame_id_ = frame_id_;

//    if (fabs(frame_diff_ - 0.01666) >= 0.015) log_debug(FMT_COMPILE("frame diff: {}"), frame_diff_);
  }


  void set_frame() noexcept
  {
    ++frame_id_;
  }


  inline void do_draw() noexcept
  {
    to_draw_ = true;
  }


  void redo_draw() noexcept
  {
    redo_draw_[0] = true;
    redo_draw_[1] = true;

    do_draw();
  }


  void add_to_command_queue(std::function<void ()> f) noexcept
  {
    do_draw();

    std::lock_guard<std::mutex> lock(mutex_);

    command_queue_.push_back(f);
  }

  void add_to_post_command_queue(std::function<void ()> f) noexcept
  {
    do_draw();

    std::lock_guard<std::mutex> lock(mutex_);

    post_command_queue_.push_back(f);
  }


  void run_command_queue() noexcept
  {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::function<void ()>> q;

    std::swap(q, command_queue_);

    for (auto& c : q) c();
  }

  void run_post_command_queue() noexcept
  {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::function<void ()>> q;

    std::swap(q, post_command_queue_);

    for (auto& c : q) c();
  }

/*
  void set_layer_zoom_out(int layer)
  {
    destinations_settings_[layer] = { 1, 0 };
    do_draw();
  };
  void set_layer_zoom_in(int layer)
  {
    destinations_settings_[layer] = { 2, 0 };
    do_draw();
  };
*/

  // state

  constexpr static int MAX_TOUCHES = 10;    // maximum number of simultaneous touches to allow
  constexpr static int MOVE_STICKY = 15*15; // how far a move has to be for it to register (squared),
                                            // as sometimes just clicking also has a small small part

  struct metric_data
  {
    int    id = 0;
    
    int    st = 0;                     // one of TouchEventDown, Up, Move or Cancel
    bool   swipe        = false;       // true if there was movement between down and up button
    bool   click        = false;       // true if this has become a 'click'
    bool   double_click = false;       // true if this has become a double 'click'
    bool   multi        = false;       // true if this is part of a multi-touch
  
    double prev_down_start{0};         // for working out whether a double click has happened.
    double      down_start{0};
  
    gpu::int_vec2 pos{-1,-1};
  
    gpu::int_vec2 start_pos {-1,-1};  // from down press
    gpu::int_vec2 start_pos2{-1,-1};  // from go into swipe mode
  
    std::deque<std::pair<double, int> > history_x; // queues of (time, pos) from when DOWN state is received
    std::deque<std::pair<double, int> > history_y;
  
    gpu::int_vec2 speed{0,0};  // set when an UP state is received
    gpu::int_vec2 delta{0,0};  // set per update
  
    // current widget taking data
  
    T* current{nullptr};
  
    metric_data() {};
  };

  struct multi_touch_metric_data
  {
    double dist{0};
    double delta{0};
  };


  // io
/*
  struct display_settings
  {
    int mode{0};
    float counter{0.0f};
  };

  std::map<int, std::vector<std::shared_ptr<T>>> destinations_; // from 'layer' to a set of entries on the layer
  std::map<int, display_settings> destinations_settings_; // from 'layer' to a set of entries on the layer
*/
  std::shared_ptr<T> top_widget_;

  std::vector<std::shared_ptr<T>> to_disconnect_;

  std::vector<std::weak_ptr<ui_anim>> anims_;
  anim anim_;
  
  T* send_all_             = nullptr;
  T* send_keyboard_events_ = nullptr;
    
  bool enabled_       = true;
//  bool in_touch_      = false;
  bool in_multitouch_ = false;

  gpu::float_point last_zoom_  = {1, 1};
  gpu::float_point last_shift_ = {0, 0};
    
  metric_data touch_metric_[MAX_TOUCHES];
  metric_data mouse_metric_;
  metric_data wheel_metric_;

  multi_touch_metric_data multi_touch_metric_; 

  // window

  std::string name_;
  float font_size_{1.0};         // 1 = no scaling, < 1 => smaller, > 1 => bigger
  float prev_font_size_{1.0};
  std::shared_ptr<font> font_;

  float version_;

  int viewport_width_ = 0, viewport_height_ = 0;
  int pixel_width_ = 0,    pixel_height_ = 0;

  gpu::int_box coords() const noexcept
  {
    return {{0,0}, {pixel_width_, pixel_height_}};
  }

  double pixel_ratio_screen_ = 1; // the screen pixel ratio
  double pixel_ratio_        = 1; // the pixel ratio to use: a client set zoom including screen pixel_ratio

  double prev_pixel_ratio_   = 1; // the previous pixel_ratio

  float x_offset_ = 0;
  float y_offset_ = 0;

  int color_mode_{0}; // 0 => light, 1 => dark

  std::array<gpu::color, 2> bg_color_;        // background colors
  std::array<gpu::color, 2> bg_color_inv_;    // background colors inverted
  std::array<gpu::color, 2> txt_color_;       // txt colors
  std::array<gpu::color, 2> select_color_;    // selected colors
  std::array<gpu::color, 2> high_color_;      // highlighted colors
  std::array<gpu::color, 2> err_txt_color_;   // error txt message color

  std::string fragment_;  // the current thing we're viewing
  std::function<void (const std::string&)> fragment_cb_;

  bool is_focused_{false};

  bool to_draw_{true};
  bool in_draw_{false};
  bool in_anim_{false};

  bool redo_draw_[2] = { true, true }; // for redrawing/re-recording stuff

  double prev_frame_time_{0}; // the time the previous frame started being drawn
  double frame_time_{0}; // the time this frame started being drawn
  double frame_diff_{0}; // the amount of time since the previous frame was drawn
  std::size_t frame_id_{0};
  std::size_t prev_frame_id_{0}; // the frame id when the time was previously set
  bool frame_time_correct_{false}; // if the frame time is correct, i.e. no sleeping

  std::function<bool (std::uint64_t frame)> pre_draw_;

  std::mutex mutex_;
  std::vector<std::function<void ()>> command_queue_;         // pre  drawing
  std::vector<std::function<void ()>> post_command_queue_;    // post drawing

  alpha alpha_;
  projection projection_;
  float zoom_{1.0};
  int widgets_active_{0};

  std::uint32_t stencil_state_{0};
  std::unique_ptr<scissor> scissor_;

  int ctx_ = 0;

  shader_basic*                  shader_basic_                  = nullptr;
//  shader_compute_template*       shader_compute_template_       = nullptr;
  shader_object*                 shader_object_                 = nullptr;
//  shader_object_instanced*       shader_object_instanced_       = nullptr;
//  shader_text*                   shader_text_                   = nullptr;
  shader_text_msdf*              shader_text_msdf_              = nullptr;
  shader_texture*                shader_texture_                = nullptr;
//  shader_example_geom*           shader_example_geom_           = nullptr;
//  shader_circle*                 shader_circle_                 = nullptr;
  shader_rounded_box*            shader_rounded_box_            = nullptr;

}; // class ui_event_instance

} // namespace plate
