#pragma once

#include <memory>
#include <span>

#include "../system/log.hpp"
#include "../system/common/quaternion.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_object.hpp"


namespace plate {

// the vertex type and color type can vary, eg: you could use floats or shorts to represent 3d coordinates

template<class VTYPE, class CTYPE = bool>
class widget_object : public ui_event_destination
{

  using Vert = buffer<VTYPE>;
  using Col  = buffer<CTYPE>;

public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, std::uint32_t prop,
                     const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    direction_.set_to_default_angle();

    ui_event_destination::init(_ui, coords, prop, parent);

    upload_uniform();
  }


  void display() noexcept
	{
    glEnable(GL_DEPTH_TEST);

    if (!mouse_down_ && (x_speed_ != 0 || y_speed_ != 0))
      apply_momentum();

    gpu::color bg = ui_->bg_color_[ui_->color_mode_];
    bg.a *= ui_->alpha_.alpha_;

    if (model_ptrs_.empty()) // use built in vertex
    {
      if constexpr (std::is_same_v<bool, CTYPE>)
        ui_->shader_object_->draw(ui_->projection_, bg, uniform_buffer_, vertex_buffer_);
      else
        ui_->shader_object_->draw(ui_->projection_, bg, uniform_buffer_, vertex_buffer_, vertex_color_buffer_);
    }
    else // use supplied buffers
    {
      for (auto& m : model_ptrs_)
      {
        if constexpr (std::is_same_v<bool, CTYPE>)
          ui_->shader_object_->draw(ui_->projection_, bg, uniform_buffer_, *m.vertex);
        else
          ui_->shader_object_->draw(ui_->projection_, bg, uniform_buffer_, *m.vertex, *m.color);
      }
    }

    glDisable(GL_DEPTH_TEST);
  }


  void set_vertex(std::span<VTYPE> data, int mode = GL_TRIANGLES) noexcept
	{
    vertex_buffer_.upload(data.data(), data.size(), mode);
  }


  void set_model(Vert* buf, Col* col_buf = nullptr) noexcept
  {
    model_ptrs_.clear();
    model_ptrs_.push_back({buf, col_buf});
  }


  void add_model(Vert* buf, Col* col_buf = nullptr) noexcept
  {
    model_ptrs_.push_back({buf, col_buf});
  }


  void set_scale(float scale) noexcept
  {
    scale_ = scale;
    upload_uniform();
  }


  void set_user_interacted_cb(std::function<void ()> cb) noexcept
  {
    interaction_cb_ = std::move(cb);
  }


  std::string_view name() const noexcept
  {
    return "#object";
  }


  bool ui_mouse_position_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    if (m.swipe)
      move(m);

    if (interaction_cb_)
      interaction_cb_();

    return true;
  }


  void ui_mouse_button_update() noexcept
  {
    auto& m = ui_->mouse_metric_;

    if (mouse_down_ && m.id)
      return;

    if (m.id) // button down
    {
      mouse_down_ = true;
    }
    else // button up
    {
      mouse_down_ = false;

      x_speed_ = m.speed.x;
      y_speed_ = m.speed.y;
    }

    if (interaction_cb_)
      interaction_cb_();
  }


  bool ui_touch_update(int id) noexcept
  {
    if (mouse_down_ && touch_id_ != id) // can only touch with one finger
      return true;

    if (interaction_cb_)
      interaction_cb_();

    auto& m = ui_->touch_metric_[id];

    if (m.st == ui_event::TouchEventDown)
    {
      mouse_down_ = true;
      touch_id_   = id;
      return true;
    }

    if (m.st == ui_event::TouchEventUp)
    {
      mouse_down_ = false;

      x_speed_ = m.speed.x;
      y_speed_ = m.speed.y;

      return true;
    }

    if (m.swipe)
			move(m);

    return true;
  }

  bool ui_zoom_update() noexcept
	{
    zoom(ui_->multi_touch_metric_.delta);

		return true;
	}

  bool ui_scroll(double x_delta, double y_delta) noexcept // zoom
  {
    if (y_delta < 0)
      zoom(1.05);
    else
      zoom(1.0 / 1.05);

    return true;
  }


  inline void zoom(double z) noexcept
  {
    scale_ *= z;

    upload_uniform();
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    ui_event_destination::set_geometry(coords);

    upload_uniform();
  }


private:


  template<class T>
  void move(const T& m) noexcept
  {
    quaternion t;
    t.set_to_xy_angle(-m.delta.y / sensitivity_, m.delta.x / sensitivity_);

    quaternion r;
    r = quaternion::multiply(t, direction_);

    direction_ = r;

    upload_uniform();
  }


  void apply_momentum() noexcept // for movement during spinning after letting go of mouse/touch
  {
    float speed = std::sqrt(x_speed_ * x_speed_ + y_speed_ * y_speed_); // pythagoras

    float accel;

    if (speed < 200)
      accel = (speed + 100) / 3;
    else
      accel = speed - 100;

    float x_accel = - x_speed_ / speed * accel;
    float y_accel = - y_speed_ / speed * accel;

    // SUVAT: s = ut + 0.5at^2
    float x_distance = -((x_speed_ * ui_->frame_diff_) + 0.5 * x_accel * (ui_->frame_diff_ * ui_->frame_diff_));
    float y_distance = -((y_speed_ * ui_->frame_diff_) + 0.5 * y_accel * (ui_->frame_diff_ * ui_->frame_diff_));

    quaternion t;
    t.set_to_xy_angle(y_distance / sensitivity_, -x_distance / sensitivity_);

    direction_ = quaternion::multiply(t, direction_);

    // v = u + at
    float new_x_speed = x_speed_ + x_accel * ui_->frame_diff_;
    float new_y_speed = y_speed_ + y_accel * ui_->frame_diff_;

    // if new speeds have overshot 0, set to 0
    if (x_speed_ * new_x_speed < 0)
      x_speed_ = 0;
    else
      x_speed_ = new_x_speed;

    if (y_speed_ * new_y_speed < 0)
      y_speed_ = 0;
    else
      y_speed_ = new_y_speed;

    upload_uniform();

    ui_->do_draw();
  }


  void upload_uniform()
  {
    auto u = uniform_buffer_.allocate_staging(1);

    // assumes object is centred around 0,0,0

    u->offset = { { my_width() / 2.0f }, { my_height() / 2.0f }, {0.0f}, {0.0f} };
    u->scale = { {scale_}, {scale_}, {scale_}, {1.0f} };

    direction_.get_euler_angles(u->rot.x, u->rot.y, u->rot.z);
    u->rot.w = 0.0f;

    uniform_buffer_.upload();
  }


  buffer<shader_object::ubo> uniform_buffer_;

  // A buffer can be owned by us, or buffers can be supplied

  Vert vertex_buffer_;
  Col vertex_color_buffer_;

  struct model
  {
    Vert* vertex;
    Col* color;
  };

  std::vector<model> model_ptrs_;

  bool mouse_down_{false};
  int  touch_id_;

  float scale_{1.0f};

  quaternion direction_;

  float x_speed_{0};
  float y_speed_{0};

  constexpr static float sensitivity_ = 2.0 * M_PI * 30.0; // higher => more movement from mouse/touch needed

  std::function<void ()> interaction_cb_;

}; // widget_object

} // namespace plate
