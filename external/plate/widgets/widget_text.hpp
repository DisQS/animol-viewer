#pragma once

#include "../system/common/ui_event_destination.hpp"

#include "../system/common/font.hpp"


namespace plate {

class widget_text : public ui_event_destination
{

public:

  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, std::uint32_t prop,
                     const std::shared_ptr<ui_event_destination>& parent,
                     const std::string& text,
                     std::uint32_t al, const std::array<gpu::color, 2>& c,
                     const gpu::rotation& r, float fscale) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);

    text_     = text;
    align_    = al;
    color_    = c;
    rotation_ = r;
    fscale_   = fscale;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * fscale_;

    upload_vertex();

    upload_uniform();
  }


  void set_text(const std::string& t) noexcept
  {
    if (text_ == t)
      return;

    text_ = t;
    upload_vertex();
    upload_uniform();
  }


  void set_click_cb(std::function< void ()> cb) noexcept
  {
    click_cb_ = cb;
  }

  void click() noexcept
  {
    if (click_cb_)
      click_cb_();
  }


  void set_scale(float s) noexcept
  {
    scale_ = s;
    upload_uniform();
  }

  void set_offset(int x, int y) noexcept
  {
    offset_x_ = x;
    offset_y_ = y;
    upload_uniform();
  }


  void set_scale_and_offset(float s, int x, int y) noexcept
  {
    scale_ = s;
    offset_x_ = x;
    offset_y_ = y;
    upload_uniform();
  }

  
  void display() noexcept
  {
    if (text_.empty())
      return;

    ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, uniform_buffer_, vertex_buffer_, ui_->font_->texture_,
                      color_[ui_->color_mode_], ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());
  }


  void ui_out() noexcept
  {
  }


  void ui_mouse_button_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    if (m.click && click_cb_)
      click_cb_();
  }


  bool ui_touch_update(int id) noexcept
  {
    auto& m = ui_->touch_metric_[id];

    if (m.st == ui_event::TouchEventUp)
    {
      if (m.click && click_cb_)
        click_cb_();

      return true;
    }

    return true;
  }


  std::string_view name() const noexcept
  {
    return "text";
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    if (coords_ == coords)
      return;

    ui_event_destination::set_geometry(coords);

    upload_vertex();
    upload_uniform();
  }


private:

    
  void upload_vertex() noexcept
  {
    if (text_.empty())
      return;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * fscale_;

    font_height_ = ui_->font_->get_max_height() * font_scale;

    std::vector<float> vertex;

    auto vpos = ui_->font_->generate_vertex(text_, vertex, font_scale);

    max_width_ = ceil(vpos.p2.x);
    y_low_ = vpos.p1.y;
    y_high_ = vpos.p2.y;

    // align the coords so the center of the text is at (0,0) - allows for rotation

    x_shift_ = max_width_/2;

    if (align_ & gpu::align::OUTSIDE)
      y_shift_ = (+ui_->font_->get_min_y()) + (font_height_ / 2); // zero base and half shift
    else
      y_shift_ = (y_high_ + y_low_)/2; // NB: y_low is usually negative

    for (int i = 0; i < vertex.size(); i += 6)
    {
      vertex[i]   -= x_shift_;
      vertex[i+1] -= y_shift_;
    }

    align_to_box();

    vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, GL_TRIANGLES);
  }


  void upload_uniform() noexcept
  {
    auto u = uniform_buffer_.allocate_staging(1);

    float xs = (1.0 - scale_) * max_width_/2.0;
  
    u->offset = { { static_cast<float>(coords_.p1.x + offset_x_ + x_align_offset_) - xs },
                  { static_cast<float>(coords_.p1.y + offset_y_ + y_align_offset_) }, {0.0f}, {0.0f} };
    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };
    u->scale  = { scale_, scale_ };
  
    uniform_buffer_.upload();
  }


  void align_to_box() noexcept
  {
    auto w = my_width();
    auto h = my_height();

    if (align_ & gpu::align::LEFT)
    {
      x_align_offset_ = x_shift_;
      y_align_offset_ = h / 2.0;
    }
    if (align_ & gpu::align::RIGHT)
    {
      x_align_offset_ = w - x_shift_;
      y_align_offset_ = h / 2.0;
    }

    if (align_ & gpu::align::CENTER)
    {
      x_align_offset_ = w / 2.0;
      y_align_offset_ = h / 2.0;
    }

    if (align_ & gpu::align::TOP)
    {
      x_align_offset_ = w / 2.0;

      if (align_ & gpu::align::OUTSIDE)
        y_align_offset_ = h - (font_height_ / 2);
      else
        y_align_offset_ = h - y_shift_;
    }

    if (align_ & gpu::align::BOTTOM)
    {
      x_align_offset_ = w / 2.0;

      if (align_ & gpu::align::OUTSIDE)
        y_align_offset_ = font_height_ / 2;
      else
        y_align_offset_ = y_shift_;
    }
  }


  buffer<shader_text_msdf::basic> vertex_buffer_;
  buffer<shader_text_msdf::ubo> uniform_buffer_;

  std::uint32_t align_;

  std::string text_;

  std::function<void ()> click_cb_;

  bool in_touch_{false};

  std::array<gpu::color, 2> color_;
  gpu::rotation rotation_;
  float fscale_;
  float scale_{1.0};

  int x_align_offset_{0};
  int y_align_offset_{0};

  int offset_x_{0};
  int offset_y_{0};

  int x_shift_, y_shift_;

  int y_low_, y_high_, max_width_; // y is relative to font baseline

  int font_height_;

}; // widget_text


} // namespace plate

