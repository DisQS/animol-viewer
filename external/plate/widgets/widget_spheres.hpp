#pragma once

#include <memory>
#include <span>

#include "../system/log.hpp"
#include "../system/common/quaternion.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/webgl/shaders/object_instanced/shader.hpp"
#include "../gpu/webgl/buffer.hpp"


namespace plate {

class widget_spheres : public ui_event_destination
{

public:

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
      ext_ubuf_ = ext;
    else
    {
      direction_.set_to_default_angle();

      upload_uniform();
    }

    upload_vertex();
  }


  ~widget_spheres() noexcept
  {
    if (vertex_array_object_)
      glDeleteVertexArrays(1, &vertex_array_object_);
  }


  void display() noexcept
	{
    if (!ibuf_ptr_)
      return;

    glEnable(GL_DEPTH_TEST);

    ui_->shader_spheres_->draw(ui_->projection_, ext_ubuf_ ? *ext_ubuf_ : ubuf_, vbuf_, *ibuf_ptr_, vao_ptr_);

    glDisable(GL_DEPTH_TEST);
  }


  void set_instance(std::span<inst> data) noexcept
  {
    ibuf_.upload(data.data(), data.size(), 0);
    ibuf_ptr_ = &ibuf_;
    vao_ptr_  = &vertex_array_object_;
  }


  void set_instance_ptr(buffer<inst>* ibuf, std::uint32_t* vao) noexcept
  {
    ibuf_ptr_ = ibuf;
    vao_ptr_  = vao;
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
    return "spheres";
  }


private:


  void upload_vertex()
  {
    auto entry = vbuf_.allocate_staging(6, GL_TRIANGLES);

    int offset = 0;

    *entry++ = { { -1.0f, -1.0f, 0.0f } };
    *entry++ = { { -1.0f,  1.0f, 0.0f } };
    *entry++ = { {  1.0f,  1.0f, 0.0f } };

    *entry++ = { { -1.0f, -1.0f, 0.0f } };
    *entry++ = { {  1.0f,  1.0f, 0.0f } };
    *entry++ = { {  1.0f, -1.0f, 0.0f } };

    vbuf_.upload();
    vbuf_.free_staging();
  }


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

  buffer<shader_object::ubo>* ext_ubuf_{nullptr}; // if we share a ubuf with an external object

  struct vert
  {
    std::array<float, 3> position;
  };

  buffer<vert> vbuf_;

  buffer<inst> ibuf_;
  std::uint32_t vertex_array_object_{0};

  buffer<inst>* ibuf_ptr_{nullptr}; // for using an external ibuf
  std::uint32_t* vao_ptr_{nullptr};

}; // widget_spheres

} // namespace plate
