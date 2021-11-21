#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"


namespace plate {

class widget_projection : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::None, parent);
  }

  
  void display() noexcept
	{
    ui_->projection_.push_transform(transform);
  }

  void undisplay() noexcept
	{
    ui_->projection_.pop();
  }


  inline void cancel() noexcept
  {
    property_ = Prop::None;
  }

  inline void uncancel() noexcept
  {
    property_ = Prop::Display | Prop::UnDisplay;
  }


  void set_transform_shift(float x, float y = 0, float z = 0) noexcept
  {
    if (x == 0 && y == 0 && z == 0)
    {
      cancel();
      return;
    }

    uncancel();

    gpu::matrix44_shift(transform, x, y, z);
  }


  void set_transform_zoom(float s) noexcept
  {
    if (s == 1)
    {
      cancel();
      return;
    }

    uncancel();

    // shift center of screen to 0,0
    // appy zoom
    // shift back

    std::array<float, 16> shift1;
    std::array<float, 16> scale;
    std::array<float, 16> shift2;

    gpu::matrix44_shift(shift1, -ui_->projection_.width_ / 2.0, -ui_->projection_.height_ / 2.0, 0);
    gpu::matrix44_scale(scale, s, s, s);
    gpu::matrix44_shift(shift2, ui_->projection_.width_ / 2.0, ui_->projection_.height_ / 2.0, 0);

    std::array<float, 16> temp;

    gpu::matrix44_mult(temp, scale, shift1);
    gpu::matrix44_mult(transform, shift2, temp);
  }


  std::string_view name() const noexcept
  {
    return "projection";
  }


  projection::proj_store transform;

}; // widget_projection

} // namespace plate
