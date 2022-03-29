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
class widget_menu_layer : public plate::ui_event_destination
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
    return "menu_layers";
  }


protected:

  
  virtual void create_button() noexcept
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

      for (int row = 0; row < sz; ++row)
      {
        for (int col = 0; col < sz; ++col)
        {
          auto circle = [=] (const int x, const int y, const int r)
          {
            return (col-x)*(col-x) + (row-y)*(row-y) < r*r;
          };

          auto line = [=] (const int x1, const int y1, const int x2, const int y2, const int r)
          {
            const float l2 = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);  // i.e. |w-v|^2 -  avoid a sqrt
            if (l2 == 0.0f) return (col-x1)*(col-x1) + (row-y1)*(row-y1) < r*r; // line of length 0
            const float t = std::clamp(((col-x1)*(x2-x1) + (row-y1)*(y2-y1)) / l2, 0.0f, 1.0f);
            const float xproj = x1 + t * (x2-x1);
            const float yproj = y1 + t * (y2-y1);
            return (col-xproj)*(col-xproj) + (row-yproj)*(row-yproj) < r*r;
          };

          if (   circle(sz*1/6, sz*5/6, sz/6)
              || circle(sz*1/3, sz*1/6, sz/6)
              || circle(sz*5/6, sz*1/3, sz/6)
              || circle(sz*2/3, sz*5/6, sz/6)
              || line(sz*1/6, sz*5/6, sz*1/3, sz*1/6, sz/20)
              || line(sz*5/6, sz*1/3, sz*1/3, sz*1/6, sz/20)
              || line(sz*5/6, sz*1/3, sz*2/3, sz*5/6, sz/20)
             )
          {
            int offset = (row * w) + (col * 4);
            bitmap[offset+3] = 255;
          }
        }
      }

      return bitmap;
    }();

    texture t(reinterpret_cast<const std::byte*>(&bitmap[0]), sz * sz * 4, sz, sz, 4);
    t.upload();

    widget_menu_ = ui_event_destination::make_ui<widget_texture>(ui_, coords_, Prop::Display, shared_from_this(),
                                                                        std::move(t), widget_texture::options::STRETCH);
  
    widget_menu_->set_click_cb([this] ()
    {
      create_menu_list();
      //viewer_->key_event(ui_event::KeyEvent::KeyEventDown, "p", "KeyP", ui_event::KeyModNone);
    });
  }


  virtual void create_menu_list() noexcept
  {
    if (widget_menu_list_)
      return;

    viewer_->hold_control();

    // shade screen

    std::array<gpu::color, 2> shade_color;
    shade_color[0] = { 1.0f, 1.0f, 1.0f, 0.0f };
    shade_color[1] = { 0.0f, 0.0f, 0.0f, 0.0f };

    auto wp = parent_.lock();

    widget_menu_shade_ = ui_event_destination::make_ui<widget_box>(ui_, ui_->coords(),
                              Prop::Display | Prop::Input, wp, shade_color);

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
                            { coords_.p2.x + border, start_y + height + border2 } };

    std::array<gpu::color, 2> bg_col;
    bg_col[0] = { 0.92f, 0.92f, 0.92f, 0.94f };
    bg_col[1] = { 0.08f, 0.08f, 0.08f, 0.94f };

    widget_menu_list_ = ui_event_destination::make_ui<widget_text_list<std::string_view>>(ui_, wp,
        coords, ui_->txt_color_, bg_col, bg_col, 0.8, 10 * ui_->pixel_ratio_, gpu::align::BOTTOM | gpu::align::RIGHT);

    std::vector<std::string_view> list;
    
    list.push_back("Spacefill"sv);
    list.push_back("Cartoon"sv);

    widget_menu_list_->set_text(std::move(list));

    widget_menu_list_->set_selection_cb([this] (std::uint32_t i, std::string_view s)
    {
      if (s == "Spacefill")
      {
        viewer_->switch_to_layer("layer_spacefill");
        widget_menu_list_->clear_selection();
        return;
      }

      if (s == "Cartoon")
      {
        viewer_->switch_to_layer("layer_cartoon");
        widget_menu_list_->clear_selection();
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

    auto fade_out = ui_event_destination::make_anim<anim_alpha>(ui_, widget_menu_shade_, ui_anim::Dir::Reverse, 0.2);

    widget_menu_shade_->set_passive();

    fade_out->set_end_cb([this, wself(weak_from_this())] ()
    {
      if (auto w = wself.lock())
      {
        widget_menu_shade_->disconnect_from_parent();
        widget_menu_shade_.reset();
      }
    });

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
  }


  std::shared_ptr<widget_texture>                     widget_menu_;

  std::shared_ptr<widget_text_list<std::string_view>> widget_menu_list_;
  std::shared_ptr<widget_box>                         widget_menu_shade_;

  VIEWER* viewer_{nullptr};

}; // class widget_menu_layer


} // namespace animol
