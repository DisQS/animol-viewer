#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_basic.hpp"


namespace plate {

class widget_triangle : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const int& x_pos1, const int& y_pos1,
                                       const int& x_pos2, const int& y_pos2,
																			 const int& x_pos3, const int& y_pos3, const gpu::color c) noexcept
  {
    ui_event_destination::init(_ui, gpu::int_box{{x_pos1, y_pos1},{x_pos3, y_pos2}},
                                                      Prop::Display | Prop::Input | Prop::Swipe);

    color_ = c;

    upload_vertex(x_pos1, y_pos1, x_pos2, y_pos2, x_pos3, y_pos3);

    uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    upload_uniform();
  }

  
  void display()
	{
    ui_->shader_basic_->draw(ui_->projection_, uniform_buffer_, vertex_buffer_);
  }


  void set_geometry(const int& x_pos1, const int& y_pos1, const int& x_pos2, const int& y_pos2,
																																const int& x_pos3, const int& y_pos3)
	{
    upload_vertex(x_pos1, y_pos1, x_pos2, y_pos2, x_pos3, y_pos3);
  }


  void ui_out()
	{
    log_debug("triangle got mouse out");
  }


  bool ui_mouse_position_update()
	{
    auto& m = ui_->mouse_metric_;

		if (m.swipe)
    {
      geometry_delta(m.delta.x, m.delta.y);
      upload_uniform();
    }

    return true;
  }


  void ui_mouse_button_update()
	{
    //printf("got ui_mouse_button_update\n");
  }


  bool ui_touch_update(int id)
  {
    auto& m = ui_->touch_metric_[id];

    if (m.st == ui_event::TouchEventDown)
    {
      in_touch_ = true;
      return true;
    }

    if (m.st == ui_event::TouchEventUp)
    {
      in_touch_ = false;
      return true;
    }

    if (m.st == ui_event::TouchEventMove)
    {
      if (!in_touch_) return true;

      if (m.swipe)
      {
        geometry_delta(m.delta.x, m.delta.y);
        upload_uniform();
      }
      return true;
    }

    return true;
  }


  std::string_view name() const noexcept
  {
    return "triangle";
  }


private:


  void upload_vertex(const int& x_pos1, const int& y_pos1, const int& x_pos2, const int& y_pos2,
																																	const int& x_pos3, const int& y_pos3)
	{
    int x_min = std::min(std::min(x_pos1, x_pos2), x_pos3);
    int y_min = std::min(std::min(y_pos1, y_pos2), y_pos3);

    int x_max = std::max(std::max(x_pos1, x_pos2), x_pos3);
    int y_max = std::max(std::max(y_pos1, y_pos2), y_pos3);

    int offset = 0;

    auto ptr = vertex_buffer_.allocate_staging(3, 4 * sizeof(float));
    float* vertex = new (ptr) float[12];

    vertex[offset++] = (float)(x_pos1 - x_min);
    vertex[offset++] = (float)(y_pos1 - y_min);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(x_pos2 - x_min);
    vertex[offset++] = (float)(y_pos2 - y_min);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex[offset++] = (float)(x_pos3 - x_min);
    vertex[offset++] = (float)(y_pos3 - y_min);
    vertex[offset++] = 0.0f;
    vertex[offset++] = 1.0f;

    vertex_buffer_.upload();
    vertex_buffer_.free_staging();

    //ui_event_destination::set_geometry({ {x_min, y_min}, {x_max, y_max} }, Prop::Input | Prop::Swipe | Prop::Display);
    ui_event_destination::set_geometry({ {x_min, y_min}, {x_max, y_max} });
  }


  void upload_uniform()
  {
    auto ptr = uniform_buffer_.allocate_staging(1, sizeof(shader_basic::ubo));

    shader_basic::ubo* u = new (ptr) shader_basic::ubo;
  
    u->offset = { { static_cast<float>(coords_.p1.x) }, { static_cast<float>(coords_.p1.y) }, {0}, {0} };
    std::memcpy(&u->color, &color_, 4 * 4);
    u->scale = { 1, 1 };
  
    uniform_buffer_.upload();
  }


  gpu::color color_;

  buffer vertex_buffer_;
  buffer uniform_buffer_;

  bool in_touch_{false};

}; // widget_triangle

} // namespace plate
