#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "system/webgl/worker.hpp"

#include "widgets/widget_box.hpp"
#include "widgets/widget_basic_vertex.hpp"
#include "widgets/widget_text.hpp"

#include "widgets/anim_projection.hpp"
#include "widgets/anim_alpha.hpp"


namespace pdbmovie {

using namespace plate;

template<class VIEWER>
class widget_control : public plate::ui_event_destination
{

public:


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords,
              const std::shared_ptr<ui_event_destination>& parent, VIEWER* v) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display | Prop::Input | Prop::Swipe, parent);

    viewer_ = v;

    create_sliders();
    create_buttons();

    auto fade_in = ui_event_destination::make_anim<anim_alpha>(
                                            ui_, shared_from_this(), ui_anim::Dir::Forward);

    user_interacted();
  }


  void display() noexcept
  {
    update_slider_played();

    // after an amount of time of inactivity, fade this out

    if (!mouse_down_ && !have_anim() && (ui_event::now() - last_user_interaction_ > 3'000))
    {
      auto fade_out = ui_event_destination::make_anim<anim_alpha>(
                                              ui_, shared_from_this(), ui_anim::Dir::Reverse);

      fade_out->set_end_cb([this] ()
      {
        disconnect_from_parent();
      });

      fading_out_ = true;
    }

    ui_->do_draw();
  }


  void user_interacted() noexcept
  {
    last_user_interaction_ = ui_event::now();

    if (fading_out_)
    {
      fading_out_ = false;
      clear_anim();
    }
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    bool height_changed = false;

    if (coords.height() != my_height()) // if height has changed - regenerate vertices for buttons
      height_changed = true;

    ui_event_destination::set_geometry(coords);

    if (height_changed)
    {
      widget_play_->disconnect_from_parent();
      widget_pause_->disconnect_from_parent();

      widget_play_.reset();
      widget_pause_.reset();

      create_buttons();
    }

    update_sliders();
  }

  bool ui_mouse_position_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    user_interacted();

    if (mouse_down_)
    {
      update_frame(m.pos.x);

      create_frame_num(m.pos.x);
    }

    return true;
  }


  void ui_mouse_button_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    user_interacted();

    if (m.id & ui_event::MouseButtonLeft)
    {
      mouse_down_ = true;
      viewer_->set_playing(false);

      update_frame(m.pos.x);

      create_frame_num(m.pos.x);

      return;
    }

    if (!m.id) // button up
    {
      mouse_down_ = false;

      if (widget_play_->is_hidden()) // reset viewer to play
        viewer_->set_playing(true);

      stop_frame_num();

      return;
    }
  }

  bool ui_touch_update(int id) noexcept
  {
    if (mouse_down_ && touch_id_ != id) // can only touch with one finger
      return true;

    auto& m = ui_->touch_metric_[id];

    user_interacted();

    if (m.st == ui_event::TouchEventDown)
    {
      mouse_down_ = true;
      touch_id_   = id;
      viewer_->set_playing(false);

      update_frame(m.pos.x);
      create_frame_num(m.pos.x);

      return true;
    }

    if (m.st == ui_event::TouchEventUp)
    {
      mouse_down_ = false;

      if (widget_play_->is_hidden()) // reset viewer to play
        viewer_->set_playing(true);

      stop_frame_num();

      return true;
    }

    // must be a move

    update_frame(m.pos.x);
    create_frame_num(m.pos.x);

    return true;
  }


  std::string_view name() const noexcept
  {
    return "control";
  }


private:

  
  void create_sliders() noexcept
  {
    float spacing = coords_.height() / 9.0;

    int x_off = spacing * 2;

    gpu::int_box coords = {{ coords_.p1.x + x_off, static_cast<int>(coords_.p1.y + spacing * 7.0f) },
                           { coords_.p2.x - x_off, static_cast<int>(coords_.p1.y + spacing * 8.0f) }};

    widget_slider_ = ui_event_destination::make_ui<widget_box>(ui_, coords, Prop::Display,
                                                          shared_from_this(), ui_->select_color_);

    widget_slider_played_ = ui_event_destination::make_ui<widget_box>(ui_, coords, Prop::Display,
                                                            shared_from_this(), ui_->txt_color_);

    update_slider_played();
  }


  void update_sliders() noexcept
  {
    float spacing = coords_.height() / 9.0;

    int x_off = spacing * 2;

    gpu::int_box coords = {{ coords_.p1.x + x_off, static_cast<int>(coords_.p1.y + spacing * 7.0f) },
                           { coords_.p2.x - x_off, static_cast<int>(coords_.p1.y + spacing * 8.0f) }};

    widget_slider_->set_geometry(coords);

    update_slider_played();
  }


  void update_slider_played() noexcept
  {
    auto [ current, count ] = viewer_->get_frame();

    float spacing = coords_.height() / 9.0;

    int x_off = spacing * 2;

    int start_x = coords_.p1.x + x_off;

    gpu::int_box coords = {{ start_x, static_cast<int>(coords_.p1.y + spacing   * 7.0f) },
                           { start_x + static_cast<int>((coords_.width() - (x_off*2)) * ((current + 1) / count)), static_cast<int>(coords_.p1.y + spacing   * 8.0f) }};

    widget_slider_played_->set_geometry(coords);
  }


  void update_frame(int x_pos) noexcept
  {
    float spacing = coords_.height() / 9.0;

    int x_off = spacing * 2;

    int start_x = coords_.p1.x + x_off;

    float width_x = coords_.width() - (x_off * 2);

    int frame = (std::max(0, x_pos - start_x) / width_x) * viewer_->get_frame().second;

    viewer_->set_frame(frame);
  }


  void create_buttons() noexcept
  {
    float top_spacing = coords_.height() / 9.0;

    int sz = top_spacing * 6;
      
    int x_off = top_spacing * 2;

    gpu::int_box button_coords = { { coords_.p1.x + x_off, coords_.p1.y },
                                   { coords_.p1.x + x_off + sz, coords_.p1.y + sz } };

    float spacing = button_coords.width() / 5.0;

    // play

    std::array<shader_basic::basic_vertex, 3> v_play =
    {{
      {spacing * 1.0f, spacing * 1.0f, 0.0f, 1.0f},
      {spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {spacing * 4.0f, spacing * 2.5f, 0.0f, 1.0f}
    }};

    widget_play_ = ui_event_destination::make_ui<widget_basic_vertex>(ui_, button_coords,
                                                      shared_from_this(), ui_->txt_color_, v_play);

    widget_play_->set_click_cb([this] ()
    {
      viewer_->set_playing(true);
      update_buttons();
      user_interacted();
    });

    // pause

    std::array<shader_basic::basic_vertex, 12> v_pause =
    {{
      {spacing * 1.0f, spacing * 1.0f, 0.0f, 1.0f},
      {spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {spacing * 2.0f, spacing * 1.0f, 0.0f, 1.0f},

      {spacing * 2.0f, spacing * 1.0f, 0.0f, 1.0f},
      {spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {spacing * 2.0f, spacing * 4.0f, 0.0f, 1.0f},

      {spacing * 3.0f, spacing * 1.0f, 0.0f, 1.0f},
      {spacing * 3.0f, spacing * 4.0f, 0.0f, 1.0f},
      {spacing * 4.0f, spacing * 1.0f, 0.0f, 1.0f},

      {spacing * 4.0f, spacing * 1.0f, 0.0f, 1.0f},
      {spacing * 3.0f, spacing * 4.0f, 0.0f, 1.0f},
      {spacing * 4.0f, spacing * 4.0f, 0.0f, 1.0f}
    }};

    widget_pause_ = ui_event_destination::make_ui<widget_basic_vertex>(ui_, button_coords,
                                                    shared_from_this(), ui_->txt_color_, v_pause);

    widget_pause_->set_click_cb([this] ()
    {
      viewer_->set_playing(false);
      update_buttons();
      user_interacted();
    });

    update_buttons();
  }


  void update_buttons() noexcept
  {
    if (viewer_->is_playing())
    {
      widget_pause_->show();
      widget_play_->hide();
    }
    else
    {
      widget_play_->show();
      widget_pause_->hide();
    }
  }


  void create_frame_num(int offset) noexcept
  {
    auto [ frame, count ] = viewer_->get_frame();

    std::string txt = fmt::to_string(static_cast<int>(frame) + 1); // 1 instead of 0 based

    if (widget_frame_num_)
      widget_frame_num_->set_text(txt);
    else
      widget_frame_num_ = ui_event_destination::make_ui<widget_text>(ui_, gpu::int_box{{0,0},{0,0}}, Prop::Display, shared_from_this(), txt, gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);

    // make sure text displays

    auto [ x_shift, y_shift ] = widget_frame_num_->get_shifts();

    if (offset - x_shift < coords_.p1.x)
      offset = coords_.p1.x + x_shift;

    if (offset + x_shift > coords_.p2.x)
      offset = coords_.p2.x - x_shift;

    widget_frame_num_->set_offset(offset, coords_.p2.y + (20 * ui_->pixel_ratio_));
  }


  void stop_frame_num() noexcept
  {
    if (widget_frame_num_)
    {
      widget_frame_num_->disconnect_from_parent();
      widget_frame_num_.reset();
    }
  }
  

  std::shared_ptr<widget_basic_vertex> widget_play_;
  std::shared_ptr<widget_basic_vertex> widget_pause_;

  std::shared_ptr<widget_box>    widget_slider_;
  std::shared_ptr<widget_box>    widget_slider_played_;

  std::shared_ptr<widget_text>   widget_frame_num_;

  VIEWER* viewer_{nullptr};

  bool mouse_down_{false};
  int  touch_id_;

  double last_user_interaction_{0};
  bool fading_out_{false};


}; // class widget_main


} // namespace pdbmovie
