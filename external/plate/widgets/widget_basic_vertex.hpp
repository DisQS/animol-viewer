#pragma once

#include <memory>
#include <span>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_basic.hpp"


namespace plate {

class widget_basic_vertex : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords,
            const std::shared_ptr<ui_event_destination>& parent, const std::array<gpu::color, 2>& c,
            std::span<shader_basic::basic_vertex> vertex_data) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display, parent);
 
    color_ = c;

    vertex_data_.assign(vertex_data.begin(), vertex_data.end());

    upload_vertex();
    upload_uniform();
  }

  
  void display() noexcept
	{
    ui_->shader_basic_->draw(ui_->projection_, ui_->alpha_.alpha_, uniform_buffer_, vertex_buffer_);
  }


  void set_geometry(const gpu::int_box& coords) noexcept
	{
    ui_event_destination::set_geometry(coords);
    upload_uniform();
  }


  void set_vertex_data(std::span<shader_basic::basic_vertex> vertex_data) noexcept
  {
    vertex_data_.assign(vertex_data.begin(), vertex_data.end());

    upload_vertex();
  }


  void set_offset(const gpu::int_point& o) noexcept
  {
    offset_x_ = o.x;
    offset_y_ = o.y;

    upload_uniform();
  }


  inline void get_offset(float& x, float& y) const noexcept
  {
    x = offset_x_;
    y = offset_y_;
  }

  inline auto get_offset_x() const noexcept { return offset_x_; }
  inline auto get_offset_y() const noexcept { return offset_y_; }


  void update_color_alpha(float alpha) noexcept
  {
    color_[0].a = alpha;
    color_[1].a = alpha;

    upload_uniform();
  }


  void set_click_cb(std::function<void ()> cb) noexcept
  {
    set_input();
    click_cb_ = std::move(cb);
  }


  void ui_mouse_button_update() noexcept
  {
    auto& m = ui_->mouse_metric_;
  
    if (m.click && click_cb_)
      click_cb_();
  }


  bool ui_mouse_position_update() noexcept
  {
    return true;
  }


  bool ui_touch_update(int id) noexcept
  {
    auto& m = ui_->touch_metric_[id];

    if (m.click && click_cb_)
      click_cb_();

    return true;
  }


  std::string_view name() const noexcept
  {
    return "basic_vertex";
  }


private:


  void upload_vertex() noexcept
	{
    vertex_buffer_.upload(vertex_data_, GL_TRIANGLES);
  }


  void upload_uniform() noexcept
  {
    auto u = uniform_buffer_.allocate_staging(1);

    u->offset = { { coords_.p1.x + offset_x_ }, { coords_.p1.y + offset_y_ }, {0}, {0} };
    std::memcpy(&u->color, &color_[ui_->color_mode_], 4 * 4);
    u->scale = { 1, 1 };
  
    uniform_buffer_.upload();
  }


  std::function<void ()> click_cb_;

  std::array<gpu::color, 2> color_;

  std::vector<shader_basic::basic_vertex> vertex_data_;

  buffer<shader_basic::ubo>          uniform_buffer_;
  buffer<shader_basic::basic_vertex> vertex_buffer_;

  float offset_x_{0};
  float offset_y_{0};

}; // widget_basic_vertex

} // namespace plate
