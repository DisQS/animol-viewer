#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_basic.hpp"


namespace plate {

class widget_box : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, const std::uint32_t prop,
            const std::shared_ptr<ui_event_destination>& parent, const std::array<gpu::color, 2>& c) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);

    color_ = c;

    upload_vertex();

    uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    upload_uniform();
  }

  
  void display() noexcept
	{
    ui_->shader_basic_->draw(ui_->projection_, ui_->alpha_.alpha_, uniform_buffer_, vertex_buffer_);
  }


  void set_geometry(const gpu::int_box& coords)
	{
    ui_event_destination::set_geometry(coords);
    upload_vertex();
  }


  void set_offset(const gpu::int_point& o)
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


  void update_color_alpha(float alpha)
  {
    color_[0].a = alpha;
    color_[1].a = alpha;

    upload_uniform();
  }


  void set_click_cb(std::function<void ()> cb)
  {
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
    return "box";
  }


private:


  void upload_vertex()
	{
    auto ptr = vertex_buffer_.allocate_staging(6, 4 * sizeof(float));
    float* vertex = new (ptr) float[24];

    int offset = 0;

    vertex[offset++] = (float)(coords_.p1.x);
    vertex[offset++] = (float)(coords_.p1.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(coords_.p1.x);
    vertex[offset++] = (float)(coords_.p2.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(coords_.p2.x);
    vertex[offset++] = (float)(coords_.p2.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    //

    vertex[offset++] = (float)(coords_.p1.x);
    vertex[offset++] = (float)(coords_.p1.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(coords_.p2.x);
    vertex[offset++] = (float)(coords_.p2.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(coords_.p2.x);
    vertex[offset++] = (float)(coords_.p1.y);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex_buffer_.upload();
    vertex_buffer_.free_staging();
  }


  void upload_uniform()
  {
    auto ptr = uniform_buffer_.allocate_staging(1, sizeof(shader_basic::ubo));

    shader_basic::ubo* u = new (ptr) shader_basic::ubo;
  
    u->offset = { {offset_x_}, {offset_y_}, {0}, {0} };
    std::memcpy(&u->color, &color_[ui_->color_mode_], 4 * 4);
    u->scale = { 1, 1 };
  
    uniform_buffer_.upload();
  }


  std::function<void ()> click_cb_;

  std::array<gpu::color, 2> color_;

  buffer vertex_buffer_;
  buffer uniform_buffer_;

  float offset_x_{0};
  float offset_y_{0};


}; // widget_box

} // namespace plate
