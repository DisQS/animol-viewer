#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "system/webgl/worker.hpp"

#include "widgets/widget_box.hpp"
#include "widgets/widget_basic_vertex.hpp"
#include "widgets/widget_text.hpp"
#include "widgets/widget_text_list.hpp"
#include "widgets/widget_text_box.hpp"

#include "widgets/anim_projection.hpp"
#include "widgets/anim_alpha.hpp"
#include "widgets/anim_merge.hpp"


namespace pdbmovie {

using namespace plate;
using namespace std::literals;

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

    if (ui_event::now() - last_user_interaction_ > 5'000)
    {
      if (!mouse_down_ && !have_anim() && !widget_menu_list_ && !widget_about_)
      {
        auto fade_out = ui_event_destination::make_anim<anim_alpha>(
                                              ui_, shared_from_this(), ui_anim::Dir::Reverse);

        fade_out->set_end_cb([this] ()
        {
          disconnect_from_parent();
        });

        fading_out_ = true;
      }
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
      widget_menu_->disconnect_from_parent();

      widget_play_.reset();
      widget_pause_.reset();
      widget_menu_.reset();

      create_buttons();
    }

    update_positions();

    update_menu();
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


  void update_status() noexcept // called by viewer if it has updated playing status
  {
    update_buttons();
  }


  std::string_view name() const noexcept
  {
    return "control";
  }


private:

  
  void create_sliders() noexcept
  {
    float spacing = coords_.height() / 10.0;

    int x_off = spacing * 2;

    gpu::int_box coords = {{ coords_.p1.x + x_off, static_cast<int>(coords_.p1.y + spacing * 8.0f) },
                           { coords_.p2.x - x_off, static_cast<int>(coords_.p1.y + spacing * 9.0f) }};

    widget_slider_ = ui_event_destination::make_ui<widget_box>(ui_, coords, Prop::Display,
                                                          shared_from_this(), ui_->select_color_);

    widget_slider_played_ = ui_event_destination::make_ui<widget_box>(ui_, coords, Prop::Display,
                                                            shared_from_this(), ui_->txt_color_);

    update_slider_played();
  }


  void update_positions() noexcept
  {
    float spacing = coords_.height() / 10.0;

    int x_off = spacing * 2;

    // sliders

    gpu::int_box coords = {{ coords_.p1.x + x_off, static_cast<int>(coords_.p1.y + spacing * 8.0f) },
                           { coords_.p2.x - x_off, static_cast<int>(coords_.p1.y + spacing * 9.0f) }};

    widget_slider_->set_geometry(coords);

    update_slider_played();

    // buttons

    int sz = spacing * 6;

    coords = { { coords_.p2.x - (2*x_off) - sz, coords_.p1.y + static_cast<int>(spacing) },
               { coords_.p2.x, coords_.p1.y + static_cast<int>(spacing) + sz } };

    widget_menu_->set_geometry(coords);
  }


  void update_slider_played() noexcept
  {
    auto [ current, count ] = viewer_->get_frame();

    float spacing = coords_.height() / 10.0;

    int x_off = spacing * 2;

    int start_x = coords_.p1.x + x_off;

    gpu::int_box coords = {{ start_x, static_cast<int>(coords_.p1.y + spacing   * 8.0f) },
                           { start_x + static_cast<int>((coords_.width() - (x_off*2)) * ((current + 1) / count)), static_cast<int>(coords_.p1.y + spacing   * 9.0f) }};

    widget_slider_played_->set_geometry(coords);
  }


  void update_frame(int x_pos) noexcept
  {
    float spacing = coords_.height() / 10.0;

    int x_off = spacing * 2;

    int start_x = coords_.p1.x + x_off;

    float width_x = coords_.width() - (x_off * 2);

    int frame = (std::max(0, x_pos - start_x) / width_x) * viewer_->get_frame().second;

    viewer_->set_frame(frame);
  }


  void create_buttons() noexcept
  {
    float top_spacing = coords_.height() / 10.0;

    int sz = top_spacing * 6;
      
    int x_off = top_spacing * 2;

    // the actual width of each button is sz + 2 * x_off (x_off either side of sz to help touch)

    gpu::int_box button_coords = { { coords_.p1.x, coords_.p1.y + static_cast<int>(top_spacing) },
                                   { coords_.p1.x + (2*x_off) + sz, coords_.p1.y + static_cast<int>(top_spacing) + sz } };

    float spacing = top_spacing * 6.0 / 5.0;

    // play

    std::array<shader_basic::basic_vertex, 3> v_play =
    {{
      {x_off + spacing * 1.0f, spacing * 1.0f, 0.0f, 1.0f},
      {x_off + spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {x_off + spacing * 4.0f, spacing * 2.5f, 0.0f, 1.0f}
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
      {x_off + spacing * 1.0f, spacing * 1.0f, 0.0f, 1.0f},
      {x_off + spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, spacing * 1.0f, 0.0f, 1.0f},

      {x_off + spacing * 2.0f, spacing * 1.0f, 0.0f, 1.0f},
      {x_off + spacing * 1.0f, spacing * 4.0f, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, spacing * 4.0f, 0.0f, 1.0f},

      {x_off + spacing * 3.0f, spacing * 1.0f, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, spacing * 4.0f, 0.0f, 1.0f},
      {x_off + spacing * 4.0f, spacing * 1.0f, 0.0f, 1.0f},

      {x_off + spacing * 4.0f, spacing * 1.0f, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, spacing * 4.0f, 0.0f, 1.0f},
      {x_off + spacing * 4.0f, spacing * 4.0f, 0.0f, 1.0f}
    }};

    widget_pause_ = ui_event_destination::make_ui<widget_basic_vertex>(ui_, button_coords,
                                                    shared_from_this(), ui_->txt_color_, v_pause);

    widget_pause_->set_click_cb([this] ()
    {
      viewer_->set_playing(false);
      update_buttons();
      user_interacted();
    });

    // menu

    button_coords = { { coords_.p2.x - (2*x_off) - sz, coords_.p1.y + static_cast<int>(top_spacing) },
                      { coords_.p2.x, coords_.p1.y + static_cast<int>(top_spacing) + sz } };

    float height   = (spacing * 3.0f) / 4.0f;
    float height_2 = height / 2.0f;

    std::array<shader_basic::basic_vertex, 18> v_menu =
    {{
      {x_off + spacing * 2.0f, (spacing * 1.0f), 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 1.0f) + height, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 1.0f), 0.0f, 1.0f},

      {x_off + spacing * 3.0f, (spacing * 1.0f), 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 1.0f) + height, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 1.0f) + height, 0.0f, 1.0f},


      {x_off + spacing * 2.0f, (spacing * 2.5f) - height_2, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 2.5f) + height_2, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 2.5f) - height_2, 0.0f, 1.0f},

      {x_off + spacing * 3.0f, (spacing * 2.5f) - height_2, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 2.5f) + height_2, 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 2.5f) + height_2, 0.0f, 1.0f},


      {x_off + spacing * 2.0f, (spacing * 4.0f) - height, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 4.0f), 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 4.0f) - height, 0.0f, 1.0f},

      {x_off + spacing * 3.0f, (spacing * 4.0f) - height, 0.0f, 1.0f},
      {x_off + spacing * 2.0f, (spacing * 4.0f), 0.0f, 1.0f},
      {x_off + spacing * 3.0f, (spacing * 4.0f), 0.0f, 1.0f},
    }};

    widget_menu_ = ui_event_destination::make_ui<widget_basic_vertex>(ui_, button_coords,
                                                    shared_from_this(), ui_->txt_color_, v_menu);
  
    widget_menu_->set_click_cb([this] ()
    {
      user_interacted();
      create_menu();
    });

    update_buttons();
  }


  void update_buttons() noexcept
  {
    if (viewer_->get_frame().second <= 1) // only one from, so nothing to play/pause
    {
      widget_play_->hide();
      widget_pause_->hide();
      
      return;
    }

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


  void create_menu() noexcept
  {
    if (widget_menu_list_)
      return;

    // shade screen

    std::array<gpu::color, 2> shade_color;
    shade_color[0] = { 1.0f, 1.0f, 1.0f, 0.4f };
    shade_color[1] = { 0.0f, 0.0f, 0.0f, 0.4f };

    widget_menu_shade_ = ui_event_destination::make_ui<widget_box>(ui_, ui_->coords(),
                              Prop::Display | Prop::Input | Prop::Swipe,
                              shared_from_this(), shade_color);

    widget_menu_shade_->set_click_cb([this] ()
    {
      delete_menu();
    });

    auto fade_in = ui_event_destination::make_anim<anim_alpha>(
       ui_, widget_menu_shade_, ui_anim::Dir::Forward, 0.4);

    // menu

    int width  = std::min(coords_.width(), static_cast<int>(200 * ui_->pixel_ratio_));
    int height = static_cast<int>(150 * ui_->pixel_ratio_);

    int start_y = coords_.p2.y + 10 * ui_->pixel_ratio_;
    int border = 10 * ui_->pixel_ratio_;
    int border2 = 2 * border;

    gpu::int_box coords = { { coords_.p2.x - width - border2, start_y + border },
                            { coords_.p2.x - border, start_y + height + border2 } };

    std::array<gpu::color, 2> bg_col;
    bg_col[0] = { 0.92f, 0.92f, 0.92f, 0.94f };
    bg_col[1] = { 0.08f, 0.08f, 0.08f, 0.94f };

    widget_menu_list_ = ui_event_destination::make_ui<widget_text_list<std::string_view>>(ui_, shared_from_this(),
        coords, ui_->txt_color_, bg_col, bg_col, 0.8, 10 * ui_->pixel_ratio_, gpu::align::BOTTOM | gpu::align::RIGHT);

    std::vector<std::string_view> list;
    
    if (ui_event::have_file_system_access_support())
      list.push_back("open local"sv);

    list.push_back("about"sv);

    widget_menu_list_->set_text(std::move(list));

    widget_menu_list_->set_selection_cb([this] (std::uint32_t i, std::string_view s)
    {
      if (s == "open local")
      {
        viewer_->show_load();
        delete_menu();

        return;
      }

      if (s == "about")
      {
        create_about();

        auto merged = make_anim_merge(ui_, widget_menu_list_, widget_about_, ui_anim::Dir::Forward);

        merged.second->set_end_cb([this, wself(weak_from_this())] ()
        {
          if (auto w = wself.lock())
          {
            widget_menu_list_->disconnect_from_parent();
            widget_menu_list_.reset();

            user_interacted();
          }
        });

        return;
      }

    });

    auto merged = make_anim_merge(ui_, widget_menu_, widget_menu_list_, ui_anim::Dir::Forward);
  }


  void delete_menu() noexcept
  {
    user_interacted();

    if (!widget_menu_shade_)
      return;

    if (!widget_about_) // keep the shade until about also goes
    {
      auto fade_out = ui_event_destination::make_anim<anim_alpha>(
          ui_, widget_menu_shade_, ui_anim::Dir::Reverse, 0.2);

      fade_out->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_menu_shade_->disconnect_from_parent();
          widget_menu_shade_.reset();
        }
      });
    }

    if (widget_menu_list_)
    {
      auto merged = make_anim_merge(ui_, widget_menu_list_, widget_menu_, ui_anim::Dir::Forward);
      widget_menu_->show();

      merged.second->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_menu_list_->disconnect_from_parent();
          widget_menu_list_.reset();

          user_interacted();
        }
      });
      /*
      auto slide_out = ui_event_destination::make_anim<anim_projection>( ui_, widget_menu_list_,
        anim_projection::Options::None, widget_menu_list_->my_width(), 0, 0, ui_anim::Dir::Reverse, 0.2);

      slide_out->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_menu_list_->disconnect_from_parent();
          widget_menu_list_.reset();

          widget_menu_->show();
          user_interacted();
        }
      });
      */
    }
  }


  void update_menu()
  {
    // shade

    if (widget_menu_shade_)
      widget_menu_shade_->set_geometry(ui_->coords());

    // menu

    if (widget_menu_list_)
    {
      int width  = std::min(coords_.width(), static_cast<int>(200 * ui_->pixel_ratio_));
      int height = static_cast<int>(150 * ui_->pixel_ratio_);

      gpu::int_box coords = { { coords_.p2.x - width, coords_.p1.y },
                              { coords_.p2.x, coords_.p1.y + height } };

      widget_menu_list_->set_geometry(coords);
    }

    // about

    if (widget_about_)
    {
      int border = 10 * ui_->pixel_ratio_;
      int border2 = border * 2;

      auto c = ui_->coords();
            
      int h = std::min(static_cast<int>(400 * ui_->pixel_ratio_), (c.height() - border2) / 2);
      int w = std::min(static_cast<int>(600 * ui_->pixel_ratio_), static_cast<int>((c.width() - border2) * 0.8));
  
      int x_centre = c.width()/2;
      int y_centre = c.height()/2;
  
      c.p1 = { x_centre - w/2, y_centre - h/2 };
      c.p2 = { x_centre + w/2, y_centre + h/2 };

      widget_about_->set_geometry(c);
    }
  }


  void create_about() noexcept
  {
    int border = 10 * ui_->pixel_ratio_;
    int border2 = border * 2;

    auto c = ui_->coords();

    int h = std::min(static_cast<int>(400 * ui_->pixel_ratio_), (c.height() - border2) / 2);
    int w = std::min(static_cast<int>(600 * ui_->pixel_ratio_), static_cast<int>((c.width() - border2) * 0.8));

    int x_centre = c.width()/2;
    int y_centre = c.height()/2;

    c.p1 = { x_centre - w/2, y_centre - h/2 };
    c.p2 = { x_centre + w/2, y_centre + h/2 };

    std::array<gpu::color, 2> bg_col;
    bg_col[0] = { 0.92f, 0.92f, 0.92f, 0.94f };
    bg_col[1] = { 0.08f, 0.08f, 0.08f, 0.94f };

    std::string txt = "About.\n\nThe AniMol web app is an open-source project, developed at the Disordered Quantum Systems (DisQS) Research group at the University of Warwick.\n\nThe code is released in its entirety under the 'New BSD License' and can be found at github.com/DisQS/animol-viewer.\n\nPlease feel free to contact us on twitter @AniMolWebApp, or submit an issue to the animol-viewer GitHub.";

    widget_about_ = ui_event_destination::make_ui<widget_text_box>(ui_, c, shared_from_this(),
          txt, ui_->txt_color_, bg_col, 0.6, 10 * ui_->pixel_ratio_);

//    auto fade_in = ui_event_destination::make_anim<anim_alpha>(
//       ui_, widget_about_, ui_anim::Dir::Forward, 0.2);

    widget_menu_shade_->set_click_cb([this] () // shade now controls about
    {
      delete_about();
    });
  }


  void delete_about() noexcept
  {
    user_interacted();
  
    if (!widget_menu_shade_)
      return;

    // shade
     
    auto fade_out = ui_event_destination::make_anim<anim_alpha>(
        ui_, widget_menu_shade_, ui_anim::Dir::Reverse, 0.2);
  
    fade_out->set_end_cb([this, wself(weak_from_this())] ()
    {
      if (auto w = wself.lock())
      {
        widget_menu_shade_->disconnect_from_parent();
        widget_menu_shade_.reset();
      }
    });

    // about

    if (widget_about_)
    {
      auto merged = make_anim_merge(ui_, widget_about_, widget_menu_, ui_anim::Dir::Forward);
      widget_menu_->show();

      merged.second->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_about_->disconnect_from_parent();
          widget_about_.reset();

          user_interacted();
        }
      });
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

    // make sure text displays within slider boundary

    float spacing = coords_.height() / 10.0;
    int x_off = spacing * 2;

    auto [ x_shift, y_shift ] = widget_frame_num_->get_shifts();

    if (offset - x_shift < coords_.p1.x + x_off)
      offset = coords_.p1.x + x_off + x_shift;

    if (offset + x_shift > coords_.p2.x - x_off)
      offset = coords_.p2.x - x_off - x_shift;

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
  std::shared_ptr<widget_basic_vertex> widget_menu_;

  std::shared_ptr<widget_box>    widget_slider_;
  std::shared_ptr<widget_box>    widget_slider_played_;

  std::shared_ptr<widget_text>   widget_frame_num_;

  std::shared_ptr<widget_text_list<std::string_view>> widget_menu_list_;
  std::shared_ptr<widget_box>                         widget_menu_shade_;

  std::shared_ptr<widget_text_box> widget_about_;

  VIEWER* viewer_{nullptr};

  bool mouse_down_{false};
  int  touch_id_;

  double last_user_interaction_{0};
  bool fading_out_{false};


}; // class widget_main


} // namespace pdbmovie
