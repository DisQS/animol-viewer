#pragma once

#include <memory>
#include <span>

#include "../system/log.hpp"
#include "../system/common/quaternion.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/webgl/shaders/object_instanced/shader.hpp"
#include "../gpu/webgl/buffer.hpp"


namespace plate {

class widget_object_instanced : public ui_event_destination
{

public:

  struct vert
  {
    std::array<short, 3> position;
    std::array<short, 3> normal;
  };

  struct inst
  {
    std::array<short,        3> offset;
    std::array<short,        1> scale;
    std::array<std::uint8_t, 4> color;
  };


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, const std::shared_ptr<ui_event_destination>& parent,
                                                                    buffer<shader_object::ubo>* ext = nullptr) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display, parent);

    if (ext)
    {
      ext_ubuf_ = ext;
    }
    else
    {
      direction_.set_to_default_angle();

      upload_uniform();
    }
  }


  ~widget_object_instanced() noexcept
  {
    if (vertex_array_object_)
      glDeleteVertexArrays(1, &vertex_array_object_);
  }


  void display() noexcept
	{
    if (!vbuf_.is_ready() || !ibuf_.is_ready())
      return;

    glEnable(GL_DEPTH_TEST);

    ui_->shader_object_instanced_->draw(ui_->projection_, ext_ubuf_ ? *ext_ubuf_ : ubuf_, vbuf_, ibuf_, &vertex_array_object_);

    glDisable(GL_DEPTH_TEST);
  }


  void set_vertex(std::span<vert> data, int mode = GL_TRIANGLES) noexcept
	{
    vbuf_.upload(data.data(), data.size(), mode);
  }


  void set_instance(std::span<inst> data) noexcept
  {
    ibuf_.upload(data.data(), data.size(), 0);
  }


  void set_scale(float scale) noexcept
  {
    scale_ = scale;
    upload_uniform();
  }


  void set_scale_and_direction(float scale, quaternion d) noexcept
  {
    scale_ = scale;
    direction_ = d;

    upload_uniform();
  }


  void set_geometry(const gpu::int_box& coords) noexcept
  {
    ui_event_destination::set_geometry(coords);
  
    upload_uniform();
  }


  std::string_view name() const noexcept
  {
    return "object_instanced";
  }


private:


  void upload_uniform()
  {
    auto u = ubuf_.allocate_staging(1);
  
    // assumes object is centred around 0,0,0

    u->offset = { { my_width() / 2.0f }, { my_height() / 2.0f }, {0.0f}, {0.0f} };
    u->scale = { {scale_}, {scale_}, {scale_}, {1.0f} };

    direction_.get_euler_angles(u->rot.x, u->rot.y, u->rot.z);
    u->rot.w = 0.0f;
  
    ubuf_.upload();
  }


  float scale_{1.0};
  quaternion direction_;

  buffer<shader_object::ubo> ubuf_;

  buffer<shader_object::ubo>* ext_ubuf_{nullptr}; // if we share the ubuf with an external object

  buffer<vert> vbuf_;
  buffer<inst> ibuf_;

  std::uint32_t vertex_array_object_{0};


}; // widget_object_instanced

} // namespace plate
