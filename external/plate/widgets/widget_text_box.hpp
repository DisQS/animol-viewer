#pragma once

#include "../system/common/ui_event_destination.hpp"

#include "../system/common/font.hpp"


namespace plate {

class widget_text_box : public ui_event_destination_autoscroll
{

public:

  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords,
                     const std::shared_ptr<ui_event_destination>& parent,
                     const std::string& text, const std::array<gpu::color, 2>& c, 
                     const std::array<gpu::color, 2>& bg_c, float font_scale, int border) noexcept
  {
    ui_event_destination_autoscroll::init(_ui, coords, Prop::Input | Prop::Swipe | Prop::Display, parent);

    text_     = text;
    color_    = c;
    bg_color_ = bg_c;
    fscale_   = font_scale;
    border_   = border;

    create_bg();

    upload_vertex();
    upload_uniform();

    set_scroll(scroll::Y);
  }


  void set_text(const std::string& t) noexcept
  {
    if (text_ == t)
      return;

    text_ = t;
    upload_vertex();
    upload_uniform();
  }


  void set_scale(float s) noexcept
  {
    scale_ = s;
    upload_uniform();
  }


  void display() noexcept
  {
    ui_->stencil_->push();

    wbox_bg_->display();

    ui_->stencil_->render();

    wbox_bg_->display();

    if (!text_.empty())
    {
      pre_display();
  
      ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, uniform_buffer_, vertex_buffer_, ui_->font_->texture_,
                      color_[ui_->color_mode_], ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());
    }

    ui_->stencil_->pop();
  }


  void scroll_update() noexcept
  {
     upload_uniform();
  }


  std::string_view name() const noexcept
  {
    return "text_box";
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    auto old_width = coords_.width();

    ui_event_destination_autoscroll::set_geometry(coords);

    create_bg();

    if (old_width != coords_.width() || ui_->any_size_changed()) // change in width or zoom so re-wrap text
      upload_vertex();

    upload_uniform();

    stop_scrolling(true); // forces us to return to a stable view if resize takes us away
  }


private:


  void create_bg() noexcept
  {
    gpu::int_box c = { { coords_.p1.x - border_, coords_.p1.y - border_ },
                       { coords_.p2.x + border_, coords_.p2.y + border_ } };

    if (wbox_bg_)
      wbox_bg_->set_geometry(c);
    else
      wbox_bg_ = ui_event_destination::make_ui<widget_rounded_box>(ui_, c, Prop::None, shared_from_this(), bg_color_, border_);
  }

    
  void upload_vertex() noexcept
  {
    if (text_.empty())
      return;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * fscale_;

    std::vector<float> vertex;

    auto vpos = ui_->font_->generate_vertex(text_, vertex, font_scale, coords_.width());

    y_low_ = vpos.p1.y;
    y_high_ = vpos.p2.y;

    vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, GL_TRIANGLES);

    set_y_size(y_high_ - y_low_);
  }


  void upload_uniform() noexcept
  {
    auto u = uniform_buffer_.allocate_staging(1);

    float y_off = coords_.height() - y_high_ + y_.offset;

    u->offset = { { static_cast<float>(coords_.p1.x) },
                  { static_cast<float>(coords_.p1.y + y_off) },
                  {0.0f}, {0.0f} };
    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };
    u->scale  = { scale_, scale_ };
  
    uniform_buffer_.upload();
  }


  buffer<shader_text_msdf::basic> vertex_buffer_;
  buffer<shader_text_msdf::ubo>   uniform_buffer_;

  std::string text_;

  std::array<gpu::color, 2> color_;
  std::array<gpu::color, 2> bg_color_;

  float fscale_;
  int border_;
  float scale_{1.0};

  int y_low_, y_high_;

  int font_height_;

  std::shared_ptr<widget_rounded_box> wbox_bg_;

}; // widget_text_box


} // namespace plate

