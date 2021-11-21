#pragma once

#include <vector>

#include "../system/common/ui_event_destination_autoscroll.hpp"
#include "../system/common/font.hpp"

#include "widget_box.hpp"



/* a lightweight list of text

  supports pre-defined list, or a dynamic list via async requests.

*/


namespace plate {

template<class STRING_TYPE>
class widget_text_list : public ui_event_destination_autoscroll
{

public:

  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent,
                  const gpu::int_box coords, const std::array<gpu::color, 2>& c, float scale) noexcept
  {
    ui_event_destination_autoscroll::init(_ui, coords, Prop::Input | Prop::Swipe | Prop::Display, parent);

    text_color_ = c;
    scale_      = scale;

    uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    set_scroll(scroll::Y);
  }


  void set_text(std::vector<STRING_TYPE>&& d) noexcept
  {
    data_ = std::move(d);

    upload_vertex();
    upload_uniform();
  }


  void set_text(const std::vector<STRING_TYPE>& d) noexcept
  {
    data_ = d;

    upload_vertex();
    upload_uniform();
  }


  void set_selection_cb(std::function<void (std::uint32_t, std::string_view)> cb) noexcept
  {
    selection_cb_ = cb;
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

    ui_->scissor_->push(coords_);

    if (wbox_selection_)
      wbox_selection_->display();

    ui_->shader_text_msdf_->draw(ui_->projection_, ui_->alpha_, uniform_buffer_, vertex_buffer_,
                ui_->font_->texture_, text_color_[ui_->color_mode_], ui_->font_->get_atlas_width(), ui_->font_->get_atlas_height());

    ui_->scissor_->pop();
  }


  void scroll_update() noexcept
  {
     upload_uniform();
  }


  void scroll_click(gpu::int_vec2 pos) noexcept
  {
    //log_debug(FMT_COMPILE("Got click pos: {} {}"), pos.x, pos.y);

    // convert mouse click pos to index...

    auto y = ((coords_.p2.y - pos.y) + y_.offset) / line_height_;

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


  void selected(std::uint32_t entry)
  {
    //log_debug(FMT_COMPILE("selected entry: {}"), entry);

    std::array<gpu::color,2> color{{{0.5, 0.5, 0.5, 1.0}, {0.5, 0.5, 0.5, 1.0}}};

    wbox_selection_ = ui_event_destination::make_ui<widget_box>(ui_,
                          gpu::int_box{{coords_.p1.x, static_cast<int>(coords_.p2.y - (line_height_ * (entry + 1)))},
                                       {coords_.p2.x, static_cast<int>(coords_.p2.y - (line_height_ * entry))}},
                                        Prop::None, shared_from_this(), color);

    wbox_selection_->set_offset({0, static_cast<int>(y_.offset)});

    if (selection_cb_)
      selection_cb_(entry, data_[entry]);

    ui_->redo_draw();
  }

    
  void upload_vertex() noexcept
  {
    std::vector<float> vertex;
    std::uint32_t current_index = 0;

    float font_scale = ui_->pixel_ratio_ * ui_->font_size_;

    line_height_    = ui_->font_->get_size() * (font_scale * 2);

    set_y_size(data_.size() * line_height_);

    int c = 1;
    for (auto& t : data_)
    {
      auto vpos = ui_->font_->generate_vertex(t, vertex, font_scale);

      int max_width_ = ceil(vpos.p2.x);

      // align the coords so the center of all the text is at (0,0) - allows for rotation

      int x_shift = -max_width_/2;
      int y_center = (line_height_ - vpos.p2.y)/2;

      int y_shift = y_.size - (c * line_height_) - (y_.size/2) + y_center;
      //int y_shift = -y_.size/2 + (c * line_height_);

      for (int i = current_index; i < vertex.size(); i += 6)
      {
        vertex[i]   += x_shift;
        vertex[i+1] += y_shift;
      }

      vertex_buffer_offset_.push_back(current_index);

      current_index = vertex.size();

      ++c;
    }

    vertex_buffer_.upload(reinterpret_cast<std::byte*>(vertex.data()), vertex.size() / 6, 6 * sizeof(float));
  }


  void upload_uniform() noexcept
  {
    auto ptr = uniform_buffer_.allocate_staging(1, sizeof(shader_text::ubo));
  
    shader_text::ubo* u = new (ptr) shader_text::ubo;
  
    u->offset = { { static_cast<float>((coords_.p1.x + coords_.p2.x) / 2) + offset_.x },
                  { static_cast<float>(coords_.p2.y - y_.size/2) + y_.offset  + offset_.y},
                  { 0.0f }, { 0.0f } };

    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };

    u->scale  = { scale_, scale_ };
  
    uniform_buffer_.upload();

    if (wbox_selection_)
      wbox_selection_->set_offset({0, static_cast<int>(y_.offset)});
  }


  std::array<gpu::color, 2> text_color_; // light/dark mode

  float scale_;

  std::vector<STRING_TYPE> data_;

  gpu::int_point offset_{0,0};

  buffer vertex_buffer_;
  buffer uniform_buffer_;

  std::vector<std::uint32_t> vertex_buffer_offset_; // index of where each entry starts in the vertex_buffer
                                                    // so that we can sub-draw

  int line_height_;

  std::shared_ptr<widget_box> wbox_selection_;
  std::function<void (std::uint32_t, std::string_view)> selection_cb_;

}; // widget_text_list


} // namespace plate

