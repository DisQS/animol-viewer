#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "widgets/widget_box.hpp"
#include "widgets/widget_texture.hpp"
#include "widgets/widget_text.hpp"
#include "widgets/widget_text_list.hpp"
#include "widgets/widget_text_box.hpp"

#include "widgets/anim_projection.hpp"
#include "widgets/anim_alpha.hpp"
#include "widgets/anim_merge.hpp"

#include "widget_control.hpp"


namespace animol {

using namespace plate;
using namespace std::literals;

template<class VIEWER>
class widget_menu_option : public plate::ui_event_destination
{

public:


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords,
              const std::shared_ptr<ui_event_destination>& parent, VIEWER* v) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display, parent);

    viewer_ = v;

    create_button();
  }


  void set_geometry(const gpu::int_box& coords) noexcept override
  {
    ui_event_destination::set_geometry(coords);

    widget_menu_->set_geometry(coords.zoom(0.6, coords.middle()));

    update_menu();
  }


  std::string_view name() const noexcept override
  {
    return "menu_option";
  }


private:

  
  void create_button() noexcept
  {
    static constexpr int sz = 120;
    static constexpr int w  = sz * 4;;

    const std::uint8_t r = ui_->txt_color_[0].r * 255;
    const std::uint8_t g = ui_->txt_color_[0].g * 255;
    const std::uint8_t b = ui_->txt_color_[0].b * 255;

    const auto bitmap = [&]
    {
      std::array<std::uint8_t, sz * sz * 4> bitmap{};

      for (int row = 0; row < sz; ++row)
      {
        for (int col = 0; col < sz; ++col)
        {
          int offset = (row * w) + (col * 4);
          bitmap[offset  ] = r;
          bitmap[offset+1] = g;
          bitmap[offset+2] = b;
          bitmap[offset+3] = 0;
        }
      }

      auto fill_f = [&] (const int s1, const int s2)
      {
        for (int row = s1; row < s2; ++row)
        {
          for (int col = 0; col < sz; ++col)
          {
            int offset = (row * w) + (col * 4);
            bitmap[offset+3] = 255;
          }
        }
      };

      fill_f(sz*0/5, sz*1/5);
      fill_f(sz*2/5, sz*3/5);
      fill_f(sz*4/5, sz*5/5);

      return bitmap;
    }();

    texture t(reinterpret_cast<const std::byte*>(&bitmap[0]), sz * sz * 4, sz, sz, 4);
    t.upload();

    widget_menu_ = ui_event_destination::make_ui<widget_texture>(ui_, coords_, Prop::Display, shared_from_this(),
                                                                        std::move(t), widget_texture::options::STRETCH);
  
    widget_menu_->set_click_cb([this] ()
    {
      create_menu_list();
    });
  }


  void create_menu_list() noexcept
  {
    if (widget_menu_list_ || widget_about_) // wait for these to die before allowing a new menu
      return;

    viewer_->hold_control();

    // shade screen

    std::array<gpu::color, 2> shade_color;
    shade_color[0] = { 1.0f, 1.0f, 1.0f, 0.4f };
    shade_color[1] = { 0.0f, 0.0f, 0.0f, 0.4f };

    auto wp = parent_.lock(); // make the control the parent so it is draw last..

    widget_menu_shade_ = ui_event_destination::make_ui<widget_box>(ui_, ui_->coords(),
                              Prop::Display | Prop::Input | Prop::Swipe, wp, shade_color);

    widget_menu_shade_->set_click_cb([this] ()
    {
      delete_menu_list();
    });

    auto fade_in = ui_event_destination::make_anim<anim_alpha>(ui_, widget_menu_shade_, ui_anim::Dir::Forward, 0.3);

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

    widget_menu_list_ = ui_event_destination::make_ui<widget_text_list<std::string_view>>(ui_, wp,
        coords, ui_->txt_color_, bg_col, bg_col, 0.8, 10 * ui_->pixel_ratio_, gpu::align::BOTTOM | gpu::align::RIGHT);

    std::vector<std::string_view> list;
    
    if (ui_event::have_file_system_access_support())
      list.push_back("open local"sv);

    list.push_back("About"sv);

    widget_menu_list_->set_text(std::move(list));

    widget_menu_list_->set_selection_cb([this] (std::uint32_t i, std::string_view s)
    {
      if (s == "open local")
      {
        viewer_->show_load();
        delete_menu_list();

        return;
      }

      if (s == "About")
      {
        create_about();

        auto merged = make_anim_merge(ui_, widget_menu_list_, widget_about_);

        merged.second->set_end_cb([this, wself(weak_from_this())] ()
        {
          if (auto w = wself.lock())
          {
            widget_menu_list_->disconnect_from_parent();
            widget_menu_list_.reset();
          }
        });

        return;
      }

    });

    auto merged = make_anim_merge(ui_, widget_menu_, widget_menu_list_);
  }


  void delete_menu_list() noexcept
  {
    if (!widget_menu_shade_)
      return;

    viewer_->release_control();

    if (!widget_about_) // keep the shade until about also goes
    {
      auto fade_out = ui_event_destination::make_anim<anim_alpha>(
          ui_, widget_menu_shade_, ui_anim::Dir::Reverse, 0.2);

      widget_menu_shade_->set_passive();

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
      auto merged = make_anim_merge(ui_, widget_menu_list_, widget_menu_);
      widget_menu_->show();

      merged.second->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_menu_list_->disconnect_from_parent();
          widget_menu_list_.reset();
        }
      });
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

      int start_y = coords_.p2.y + 10 * ui_->pixel_ratio_;
      int border = 10 * ui_->pixel_ratio_;
      int border2 = 2 * border;

      gpu::int_box coords = { { coords_.p2.x - width - border2, start_y + border },
                              { coords_.p2.x - border, start_y + height + border2 } };

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

    auto wp = parent_.lock();

    widget_about_ = ui_event_destination::make_ui<widget_text_box>(ui_, c, wp,
          txt, ui_->txt_color_, bg_col, 0.6, 10 * ui_->pixel_ratio_);

    widget_menu_shade_->set_click_cb([this] () // shade now controls about
    {
      delete_about();
    });
  }


  void delete_about() noexcept
  {
    if (!widget_menu_shade_)
      return;

    viewer_->release_control();

    // shade
     
    auto fade_out = ui_event_destination::make_anim<anim_alpha>(
        ui_, widget_menu_shade_, ui_anim::Dir::Reverse, 0.2);
  
    widget_menu_shade_->set_passive();

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
      auto merged = make_anim_merge(ui_, widget_about_, widget_menu_);
      widget_menu_->show();

      merged.second->set_end_cb([this, wself(weak_from_this())] ()
      {
        if (auto w = wself.lock())
        {
          widget_about_->disconnect_from_parent();
          widget_about_.reset();
        }
      });
    }
  }


  std::shared_ptr<widget_texture>                     widget_menu_;

  std::shared_ptr<widget_text_list<std::string_view>> widget_menu_list_;
  std::shared_ptr<widget_box>                         widget_menu_shade_;

  std::shared_ptr<widget_text_box>                    widget_about_;

  VIEWER* viewer_{nullptr};

}; // class widget_menu_option


} // namespace animol
