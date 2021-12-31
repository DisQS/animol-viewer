#pragma once

#include <vector>

#include "../system/common/ui_event_destination_autoscroll.hpp"
#include "../system/common/font.hpp"

#include "widget_box.hpp"
#include "widget_rounded_box.hpp"



/* a lightweight list of text

  supports pre-defined list, or a dynamic list via async requests.

*/


namespace plate {

template<class STRING_TYPE>
class widget_text_list : public ui_event_destination_autoscroll
{

public:

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent,
                  const gpu::int_box coords, const std::array<gpu::color, 2>& c,
                  const std::array<gpu::color, 2>& sel_c,
                  const std::array<gpu::color, 2>& bg_c, float font_scale,
                  int border = 0, int al = gpu::align::TOP) noexcept
  {
    ui_event_destination_autoscroll::init(_ui, coords, Prop::Input | Prop::Swipe | Prop::Display, parent);

    text_color_   = c;
    select_color_ = sel_c;
    bg_color_     = bg_c;
    font_scale_   = font_scale;
    border_       = border;
    align_        = al;

    set_scroll(scroll::Y);
  }


  void set_text(std::vector<STRING_TYPE>&& d) noexcept
  {
    data_ = std::move(d);

    upload_vertex();
    adjust_coords();
    upload_uniform();

    create_bg();
  }


  void set_text(const std::vector<STRING_TYPE>& d) noexcept
  {
    data_ = d;

    upload_vertex();
    adjust_coords();
    upload_uniform();

    create_bg();
  }


  void set_selection_cb(std::function<void (std::uint32_t, std::string_view)> cb) noexcept
  {
    selection_cb_ = cb;
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    ui_event_destination_autoscroll::set_geometry(coords);

    adjust_coords();

    create_bg();

    upload_uniform();
  }


  void set_offset(const gpu::int_point& o)
  {
    offset_ = o;

    upload_uniform();

    if (wbox_selection_)
      wbox_selection_->set_offset({o.x, o.y + static_cast<int>(y_.offset)});
  }


  void display() noexcept
  {
    pre_display();

    ui_->stencil_->push();

    wbox_bg_->display();

    ui_->stencil_->render();

    wbox_bg_->display();

    if (wbox_selection_)
      wbox_selection_->display();

    ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, uniform_buffer_, vertex_buffer_,
                ui_->font_->texture_, text_color_[ui_->color_mode_], ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());

    ui_->stencil_->pop();
  }


  void scroll_update() noexcept
  {
     upload_uniform();
  }


  void scroll_click(gpu::int_vec2 pos) noexcept
  {
    //log_debug(FMT_COMPILE("Got click pos: {} {}"), pos.x, pos.y);

    // convert mouse click pos to index...

    auto y = ((coords_.p2.y - border_ - pos.y) + y_.offset) / line_height_;

    if (y < 0 || y >= data_.size()) return;

    selected(y);
  }


  void ui_out() noexcept
  {
    //log_debug("texture got mouse out");
  }


  std::string_view name() const noexcept
  {
    return "text_list";
  }


private:


  void create_bg() noexcept
  {
    if (wbox_bg_)
      wbox_bg_->set_geometry(coords_);
    else
      wbox_bg_ = ui_event_destination::make_ui<widget_rounded_box>(ui_, coords_, Prop::None, shared_from_this(), bg_color_, border_);
  }


  void selected(std::uint32_t entry) noexcept
  {
    if (!selection_cb_ || wbox_selection_) // only allow one selection
      return;

    int hb = border_ / 2;

    wbox_selection_ = ui_event_destination::make_ui<widget_box>(ui_,
                          gpu::int_box{{coords_.p1.x + hb, static_cast<int>(coords_.p2.y - border_ - (line_height_ * (entry + 1)))},
                                       {coords_.p2.x - hb, static_cast<int>(coords_.p2.y - border_ - (line_height_ * entry))}},
                                        Prop::None, shared_from_this(), select_color_);

    wbox_selection_->set_offset({0, static_cast<int>(y_.offset)});

    selection_cb_(entry, data_[entry]);

    ui_->redo_draw();
  }

    
  void upload_vertex() noexcept
  {
    std::vector<float> vertex;
    std::uint32_t current_index = 0;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_ * font_scale_;

    line_height_    = ui_->font_->get_size() * (font_scale * 2);

    set_y_size(data_.size() * line_height_);

    max_width_ = 0;

    int c = 1;
    for (auto& t : data_)
    {
      auto vpos = ui_->font_->generate_vertex(t, vertex, font_scale);

      int width = ceil(vpos.p2.x);

      if (width > max_width_)
        max_width_ = width;

      // align the coords so the center of all the text is at (0,0) - allows for rotation

      int x_shift = -width/2;
      int y_center = (line_height_ - vpos.p2.y)/2;

      int y_shift = y_.size - (c * line_height_) - (y_.size/2) + y_center;

      for (int i = current_index; i < vertex.size(); i += 6)
      {
        vertex[i]   += x_shift;
        vertex[i+1] += y_shift;
      }

      vertex_buffer_offset_.push_back(current_index);

      current_index = vertex.size();

      ++c;
    }

    vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, GL_TRIANGLES);
  }


  void adjust_coords() noexcept
  {
    if (border_ == 0)
      return;

    auto w = coords_.width();
    auto h = coords_.height();

    // fixme add all combinations

    if (align_ == gpu::align::TOP)
    {
      coords_.p1.x += (w - max_width_ + 2 * border_) / 2;
      coords_.p1.y = coords_.p2.y - y_.size - 2 * border_;
      coords_.p2.x = coords_.p1.x + max_width_ + 2 * border_;

      return;
    }

    if (align_ == (gpu::align::BOTTOM | gpu::align::RIGHT))
    {
      coords_.p1.x = coords_.p2.x - (max_width_ + 2 * border_);
      coords_.p2.y = coords_.p1.y + y_.size + 2 * border_;

      return;
    }
  }


  void upload_uniform() noexcept
  {
    auto u = uniform_buffer_.allocate_staging(1);
  
    u->offset = { { static_cast<float>(coords_.p1.x + coords_.width() / 2) + offset_.x },
                  { static_cast<float>(coords_.p2.y - border_ - y_.size/2) + y_.offset  + offset_.y},
                  { 0.0f }, { 0.0f } };

    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };

    u->scale  = { 1.0f, 1.0f };
  
    uniform_buffer_.upload();

    if (wbox_selection_)
      wbox_selection_->set_offset({0, static_cast<int>(y_.offset)});
  }


  std::array<gpu::color, 2> text_color_;
  std::array<gpu::color, 2> bg_color_;
  std::array<gpu::color, 2> select_color_;

  float font_scale_;
  int border_;
  int align_;

  std::vector<STRING_TYPE> data_;
  int max_width_{0};

  gpu::int_point offset_{0,0};

  buffer<shader_text_msdf::basic> vertex_buffer_;
  buffer<shader_text_msdf::ubo>   uniform_buffer_;

  std::vector<std::uint32_t> vertex_buffer_offset_; // index of where each entry starts in the vertex_buffer
                                                    // so that we can sub-draw

  int line_height_;

  std::shared_ptr<widget_rounded_box> wbox_bg_;
  std::shared_ptr<widget_box> wbox_selection_;

  std::function<void (std::uint32_t, std::string_view)> selection_cb_;

}; // widget_text_list


} // namespace plate

