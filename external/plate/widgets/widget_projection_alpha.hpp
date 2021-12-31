#pragma once

#include <memory>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"


namespace plate {

class widget_projection_alpha : public ui_event_destination
{

public:


  void init(const std::shared_ptr<state>& _ui, const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, {{0,0},{0,0}}, Prop::Display | Prop::UnDisplay, parent);
  }

  
  void display() noexcept
	{
    ui_->projection_.push_transform(transform_);
    ui_->alpha_.push_transform(alpha_);
  }

  void undisplay() noexcept
	{
    ui_->projection_.pop();
    ui_->alpha_.pop();
  }


  inline void cancel() noexcept
  {
    property_ = Prop::None;
  }

  inline void uncancel() noexcept
  {
    property_ = Prop::Display | Prop::UnDisplay;
  }


  void transform_start() noexcept
  {
    gpu::matrix44_scale(transform_, 1, 1, 1); // identity matrix
  }


  void set_transform_shift(float x, float y = 0, float z = 0) noexcept
  {
    if (x == 0 && y == 0 && z == 0)
    {
      cancel();
      return;
    }

    uncancel();

    std::array<float, 16> shift;
    std::array<float, 16> r;

    gpu::matrix44_shift(shift, x, y, z);
    gpu::matrix44_mult(r, shift, transform_);

    transform_ = r;
  }


  void set_transform_zoom(float sx, float sy, float x, float y, float z = 0) noexcept // zoom from a point
  {
    if (sx == 1 && sy == 1)
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

    gpu::matrix44_shift(shift1, -x, -y, z);
    gpu::matrix44_scale(scale, sx, sy, 1);
    gpu::matrix44_shift(shift2, x, y, z);

    std::array<float, 16> temp1;

    gpu::matrix44_mult(temp1, shift1, transform_);

    std::array<float, 16> temp2;

    gpu::matrix44_mult(temp2, scale, shift1);
    gpu::matrix44_mult(transform_, shift2, temp2);
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
    gpu::matrix44_mult(transform_, shift2, temp);
  }


  void set_transform_alpha(float a) noexcept
  {   
    alpha_ = a;
  }


  std::string_view name() const noexcept
  {
    return "projection_aalpha";
  }


  projection::proj_store transform_;

  float alpha_;

}; // widget_projection_alpha

} // namespace plate
