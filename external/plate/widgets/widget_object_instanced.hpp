#pragma once

#include <memory>
#include <span>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_object_instanced.hpp"
#include "../gpu/buffer.hpp"


namespace plate {

class widget_object_instanced : public ui_event_destination
{

public:

  struct vert
  {
    float position[4];
    float normal[4];
    float color[4];
  };

  struct inst
  {
    float offset[4];
    float rot[4];
    float scale[4];
  };


  void init(const std::shared_ptr<state>& _ui) noexcept
  {
    ui_event_destination::init(_ui, Prop::Display);
  }


  ~widget_object_instanced()
  {
    if (vertex_array_object_) glDeleteVertexArrays(1, &vertex_array_object_);
  }


  void display()
	{
    ui_->shader_object_instanced_->draw(vertex_buffer_, instance_buffer_, &vertex_array_object_);
  }


  void set_vertex(std::span<std::byte> data)
	{
    vertex_buffer_.upload(data.data(), data.size() / sizeof(vert), sizeof(vert));
  }


  void set_instances(std::span<inst> data)
  {
    instance_buffer_.upload(reinterpret_cast<std::byte*>(data.data()), data.size(), sizeof(inst));
  }


  void set_instances(std::span<std::byte> data)
	{
    instance_buffer_.upload(data.data(), data.size() / sizeof(inst), sizeof(inst));
  }


  std::string_view name() const noexcept
  {
    return "object_instanced";
  }


private:


  buffer vertex_buffer_;
  buffer instance_buffer_;

  std::uint32_t vertex_array_object_{0};


}; // widget_object_instanced

} // namespace plate
