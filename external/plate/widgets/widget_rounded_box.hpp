#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_rounded_box.hpp"


namespace plate {

class widget_rounded_box : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, const std::uint32_t prop,
                  const std::shared_ptr<ui_event_destination>& parent,
                  const std::array<gpu::color, 2>& c, float r) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);

    color_  =c;
    radius_ = r;

    upload_vertex();

    upload_uniform();
  }

  
  void display() noexcept override
	{
    ui_->shader_rounded_box_->draw(ui_->projection_, ui_->alpha_.alpha_, uniform_buffer_, vertex_buffer_);
  }


  void set_geometry(const gpu::int_box& coords) noexcept override
	{
    ui_event_destination::set_geometry(coords);

    upload_vertex();
    upload_uniform();
  }


  void update_color_mode() noexcept override
  {
    upload_uniform();
  }


  void set_offset(const gpu::int_point& o)
  {
    offset_x_ = o.x;
    offset_y_ = o.y;

    upload_uniform();
  }


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


  void ui_mouse_button_update() noexcept override
  {
    auto& m = ui_->mouse_metric_;
  
    if (m.click && click_cb_)
      click_cb_();
  }


  bool ui_touch_update(int id) noexcept override
  {
    auto& m = ui_->touch_metric_[id];

    if (m.click && click_cb_)
      click_cb_();

    return true;
  }


  std::string_view name() const noexcept override
  {
    return "rounded_box";
  }


private:


  void upload_vertex()
	{
    auto entry = vertex_buffer_.allocate_staging(6, GL_TRIANGLES);

    *entry++ = { (float)(coords_.p1.x), (float)(coords_.p1.y), 0.0f, 1.0f, 0.0f, 0.0f };
    *entry++ = { (float)(coords_.p1.x), (float)(coords_.p2.y), 0.0f, 1.0f, 0.0f, 1.0f };
    *entry++ = { (float)(coords_.p2.x), (float)(coords_.p2.y), 0.0f, 1.0f, 1.0f, 1.0f };
    *entry++ = { (float)(coords_.p1.x), (float)(coords_.p1.y), 0.0f, 1.0f, 0.0f, 0.0f };
    *entry++ = { (float)(coords_.p2.x), (float)(coords_.p2.y), 0.0f, 1.0f, 1.0f, 1.0f };
    *entry++ = { (float)(coords_.p2.x), (float)(coords_.p1.y), 0.0f, 1.0f, 1.0f, 0.0f };

    vertex_buffer_.upload();
    vertex_buffer_.free_staging();
  }


  void upload_uniform()
  {
    auto u = uniform_buffer_.allocate_staging(1);
  
    u->offset = { {offset_x_}, {offset_y_}, {0}, {0} };
    std::memcpy(&u->color, &color_[ui_->color_mode_], 4 * 4);
    u->scale = { 1, 1 };
    u->dim = { static_cast<float>(my_width()), static_cast<float>(my_height()) };
    u->radius = radius_;
  
    uniform_buffer_.upload();
  }


  std::function<void ()> click_cb_;

  std::array<gpu::color, 2> color_;
  float radius_;

  buffer<shader_rounded_box::ubo>   uniform_buffer_;
  buffer<shader_rounded_box::basic> vertex_buffer_;

  float offset_x_{0};
  float offset_y_{0};


}; // widget_rounded_box

} // namespace plate
