#pragma once

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <functional>

#include "ui_event.hpp"
#include "ui.hpp"
#include "../../gpu/gpu.hpp"

/*

  ui event destination.

  Widgets inherit from this to add themselves as a ui event destination, they will then receive calls such as:

    ui_out, ui_move, ui_state, ui_zoom and key_state

*/

namespace plate {

class ui_event_destination;

typedef ui<ui_event_destination> state;

namespace keyboard
{
  extern void open(std::shared_ptr<state>& s) noexcept;
  extern void close() noexcept;
  extern void cancel_delete() noexcept;
}




class ui_event_destination : public std::enable_shared_from_this<ui_event_destination>
{

public:

  enum Prop
  {
    None        = 0,
    Input       = 1<<0, // the object can accept mouse/keyboard or touch inputs
    Swipe       = 1<<1, // the object supports swipe movements
    Display     = 1<<2, // the object should be auto displayed
    Reshape     = 1<<3, // the object receives a reshape if the canvas has been resized/reshaped
    Priority    = 1<<4, // This means LOW priority - ie add widget to the bottom of the stack
    Hidden      = 1<<5, // Overide show or input so that this AND everything underneath is hidden
    PartDisplay = 1<<6, // Auto display this (if set), but nothing underneath it, input as per normal
    UnDisplay   = 1<<7, // the object should have undisplay called once its children have been display
    Passive     = 1<<8  // Override inptus so that this AND everything underneath doesn't accept input
  };


  enum class Msg
  {
    Logout    // user has logged out
  };

  // when creating a ui_event_destination use make_ui so that a shared_ptr can be auto added to the ui_

  template<typename T, typename... _Args>
  static std::shared_ptr<T> make_ui(_Args&&... __args)
  {
    auto w = std::make_shared<T>();
    
    w->init(std::forward<_Args>(__args)...);

    // add it to ui

    if (!w->ui_->top_widget_)
    {
      w->ui_->top_widget_ = w;
      return w;
    }

    if (auto p = w->parent_.lock())
      p->add_child(w);
    else
      w->ui_->top_widget_->add_child(w);

    return w;
  }


  template<typename T, typename... _Args>
  static std::shared_ptr<T> make_anim(_Args&&... __args)
  {
    auto w = std::make_shared<T>();

    w->init(std::forward<_Args>(__args)...);

    if (auto p = w->parent_.lock())
      p->add_anim(w);

    return w;
  }

protected:

  ui_event_destination() noexcept
  {
  }

public:

  inline void init(const std::shared_ptr<state>& _ui) noexcept
  {
    ui_ = _ui;

    property_ = Prop::Input | Prop::Swipe | Prop::Display;

    parent_ = ui_->top_widget_;

    ++(ui_->widgets_active_);
  }


  inline void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, std::uint32_t prop) noexcept
  {
    ui_ = _ui;
    coords_ = coords;
    property_ = prop;

    parent_ = ui_->top_widget_;

    ++(ui_->widgets_active_);
	}


  inline void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords,
                            std::uint32_t prop, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_       = _ui;
    coords_   = coords;
    property_ = prop;
    parent_   = parent;

    ++(ui_->widgets_active_);
	}


  inline void init(const std::shared_ptr<state>& _ui, std::uint32_t prop,
                          const std::shared_ptr<ui_event_destination>& parent)
  {
    ui_       = _ui;
    property_ = prop;
    parent_   = parent;

    coords_   = { {-1, -1}, {-1, -1} };

    ++(ui_->widgets_active_);
  }


  ~ui_event_destination()
  {
    if (ui_)
    {
      --(ui_->widgets_active_);

      if (supports_input())
        ui_->unhook(this);
    }
  }


  void hide()
	{
    property_ |= Prop::Hidden;
	}


  void show()
  {
    property_ &= ~Prop::Hidden;
  }


  void reposition() // move to front or back of display list according to Prop::Priority
  {
    if (auto p = parent_.lock())
    {
      auto self = shared_from_this();

      p->rm_child(self);
      p->add_child(self);
    }
  }


  void reposition_after(std::string_view name) // move after this widget in the parents view
  {
    if (auto p = parent_.lock())
    {
      auto self = shared_from_this();

      for (auto it = p->children_.begin(); it != children_.end(); ++it)
      {
        if ((*it)->name() == name)
        {
          p->rm_child(self);
          p->children_.insert(++it, self);
          parent_ = p;
          return;
        }
      }
    }
  }


  inline int my_width()  const noexcept
  {
    return coords_.p2.x - coords_.p1.x;
  }

  inline int my_height() const noexcept
  {
    return coords_.p2.y - coords_.p1.y;
  }

  inline void set_geometry(const gpu::int_box& coords) noexcept
  {
    coords_ = coords;
  }


  // an interface to allow passing of coords

  virtual void handle_coords(const gpu::float_box& coords) noexcept
  {

  }


  // an interface to allow getting background widgets for a stencil

  virtual std::shared_ptr<ui_event_destination> get_background() noexcept
  {
    return nullptr;
  }

/*
  void set_geometry(const gpu::int_box& coords, std::uint32_t prop)
	{
		coords_   = coords;
    property_ = prop;
	}
*/

  inline gpu::int_box get_geometry() const noexcept
  {
    return coords_;
  }


  void geometry_delta(int x_shift, int y_shift) noexcept
	{
		coords_.p1.x += x_shift;
    coords_.p2.x += x_shift;
    coords_.p1.y += y_shift;
    coords_.p2.y += y_shift;
	}


  void add_anim(std::shared_ptr<ui_event_destination> w) noexcept
  {
    if (!w)
      return;
    
    anim_ = w;
    w->parent_ = shared_from_this();

    ui_->do_draw();
  }


  void clear_anim() noexcept
  {
    anim_.reset();
  }


  void add_child(std::shared_ptr<ui_event_destination> w) noexcept
  {
    if (!w)
      return;

    // check not already in

    for (auto& c : children_)
      if (w == c)
        return;

    // add 

    if (w->property_ & Prop::Priority) // at front
      children_.push_front(w);
    else // at back
      children_.push_back(w);
    
    w->parent_ = shared_from_this();

    ui_->do_draw();
  }


  void rm_child(std::shared_ptr<ui_event_destination> w) noexcept
  {
    for (auto it = children_.begin(); it != children_.end(); ++it)
    {
      if (*it == w)
      {
        children_.erase(it);

        w->parent_.reset();

        ui_->do_draw();

        return;
      }
    }
    
    log_debug(FMT_COMPILE("Unbale to remove from children as not found sz: {} parent: {} child: {}"),
      children_.size(),
      name(),
      w->name());
  }


  void rm_child(std::string_view name) noexcept
  {
    for (auto it = children_.begin(); it != children_.end();)
    {
      if ((*it)->name() == name)
      {
        (*it)->parent_.reset();

        it = children_.erase(it);
      }
      else
        ++it;
    }
  }


  void disconnect_from_parent() noexcept
  {
    if (!ui_)
      return;

    if (auto p = parent_.lock())
    {
      if (ui_->in_draw_)
      {
        //log_debug(FMT_COMPILE("{} setting hidden as is in_draw"), name());
        set_hidden();
        ui_->to_disconnect_.push_back(shared_from_this());
        ui_->do_draw();
      }
      else
      {
        //log_debug(FMT_COMPILE("{} removing parent proper"), name());
        p->rm_child(shared_from_this());
      }
    }
    else
      log_debug("disconnect from parent without a parent");
  }


  inline bool supports_input()    const noexcept { return property_ & Prop::Input;   }
  inline bool supports_swipe()    const noexcept { return property_ & Prop::Swipe;   }
  inline bool supports_display()  const noexcept { return property_ & Prop::Display; }
  inline bool supports_undisplay()const noexcept { return property_ & Prop::UnDisplay; }
  inline bool is_hidden()         const noexcept { return property_ & Prop::Hidden;  }
  inline bool is_part_display()   const noexcept { return property_ & Prop::PartDisplay; }
  inline bool is_passive()        const noexcept { return property_ & Prop::Passive;  }

  inline void set_hidden()              noexcept { property_ |=  Prop::Hidden;  }
  inline void unset_hidden()            noexcept { property_ &= ~Prop::Hidden;  }

  inline void set_part_display()        noexcept { property_ |=  Prop::PartDisplay;  }
  inline void unset_part_display()      noexcept { property_ &= ~Prop::PartDisplay;  }

  inline void set_passive()             noexcept { property_ |=  Prop::Passive;  }
  inline void unset_passive()           noexcept { property_ &= ~Prop::Passive;  }


  virtual bool message(Msg m) noexcept
  {
    return false;
  }


  void tree_reshape() noexcept
  {
    if (property_ & Reshape)
      reshape();

    auto copy = children_; // in case they are modified during tree descent

    for (auto& c : copy)
      c->tree_reshape();
  }


  void tree_message(Msg m) noexcept
  {
    if (message(m)) // when handled for the branch return straight away
      return;

    if (anim_)
      anim_->tree_message(m);

    auto copy = children_; // in case they are modified during tree descent

    for (auto& c : copy)
      c->tree_message(m);
  }


  inline void send_keyboard(bool via_touch = false) noexcept
  {
    if (ui_)
    {
      ui_->send_keyboard(shared_from_this());

      if (via_touch)
        keyboard::open(ui_);
    }
  }

  inline void unsend_keyboard() noexcept
  {
    if (ui_ && ui_->unsend_keyboard(this))
      keyboard::close();
  }


  inline void close_keyboard() noexcept
  {
    keyboard::close();
    ui_->close_keyboard();
  }


  // display callbacks are made on these:


  int tree_print(int depth = 0) const noexcept
  {
    int count = 1;

    if (depth == 0)
      log_debug(FMT_COMPILE("widget_active: {}"), ui_->widgets_active_);

    std::string s(depth, ' ');

    std::string options = "[ ";
    if (supports_input())     options += "input ";
    if (supports_swipe())     options += "swipe ";
    if (supports_display())   options += "display ";
    if (supports_undisplay()) options += "undisplay ";
    if (is_hidden())          options += "hidden ";
    if (is_part_display())    options += "is_part_display ";
    if (is_passive())         options += "is_passive ";
    options += "]";

    log_debug(FMT_COMPILE("{} {}  {} ({})"), s, options, name(), children_.size());

    if (anim_)
    {
      log_debug(FMT_COMPILE("{} anim->"), s);
      count += anim_->tree_print(depth + 1);
      log_debug(FMT_COMPILE("{} children->"), s);
    }

    for (auto& w : children_)
      count += w->tree_print(depth + 1);

    if (depth == 0)
      log_debug(FMT_COMPILE("tree_entries: {}"), count);

    return count;
  }


  virtual void display() noexcept
  {
  }

  virtual void undisplay() noexcept
  {
  }


  void tree_display() noexcept
  {
    if (is_hidden())
      return;

    if (anim_)
    {
      anim_->tree_anim_display();

      if (is_hidden()) // the anim can set the widget to hidden
        return;
    }

    if (supports_display())
      display();

    if (!is_part_display())
      for (auto& w : children_)
        w->tree_display();

    if (supports_undisplay())
      undisplay();

    if (anim_)
      anim_->tree_anim_undisplay();
  }


  void tree_anim_display() noexcept
  {
    if (is_hidden())
      return;

    if (supports_display())
      display();

    if (!is_part_display() && anim_)
      anim_->tree_anim_display();
  }


  void tree_anim_undisplay() noexcept
  {
    if (is_hidden())
      return;

    if (!is_part_display() && anim_)
      anim_->tree_anim_undisplay();

    if (supports_undisplay())
      undisplay();
  }


  virtual void update_color_mode() noexcept
  {
  }


  void tree_update_color_mode() noexcept
  {
    update_color_mode();

    for (auto& w : children_)
      w->tree_update_color_mode();
  }


  ui_event_destination* find_input(const int& x, const int& y, const bool& swipe) noexcept
  {
    if (is_hidden() || is_passive() || !supports_input())
      return nullptr;

    if (swipe && !supports_swipe())
      return nullptr;

    if (coords_.p1.x <= x && x <= coords_.p2.x && coords_.p1.y <= y && y <= coords_.p2.y)
      return this;
    
    return nullptr;
  }

  ui_event_destination* tree_find_input(const int& x, const int& y, const bool& swipe) noexcept
  {    
    if (is_hidden() || is_passive())
      return nullptr;

    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
    {
      auto w = (*it)->tree_find_input(x, y, swipe);

      if (w)
        return w;
    }

    return find_input(x, y, swipe);
  }

  virtual std::string_view name() const noexcept = 0;
/*  {
    return "no_name";
  }*/


  // mouse input callbacks are made to these:

  virtual bool ui_mouse_position_update() noexcept
  {
    return false;
  }


  virtual void ui_out() noexcept
  {
  }


  virtual void ui_mouse_button_update() noexcept
  {
  }

  virtual bool ui_scroll(double x_delta, double y_delta) noexcept
  {
    return false;
  }

	// touch

	virtual bool ui_touch_update(int i) noexcept
  {
    return true;
  }

	virtual bool ui_zoom_update() noexcept
  {
    return true;
  }

//  virtual bool ui_move(metric_data& m) { return false; }
//  virtual void ui_state(metric_data& m) { }
//  virtual void ui_zoom(gpu::float_point zoom, gpu::float_point shift) { }

  // canvas

  virtual void reshape() noexcept
  {
  }


  // keyboard

  virtual void key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8, ui_event::KeyMod) { }

  virtual void deactivate() {}


  // state

  std::uint32_t property_;     // Input, Swipe and Display

  gpu::int_box coords_;

  std::shared_ptr<state> ui_;  // the ui this widget belongs to

  std::shared_ptr<ui_event_destination>            anim_;
  std::list<std::shared_ptr<ui_event_destination>> children_;
  std::weak_ptr<ui_event_destination>              parent_;
};


} // namespace plate
