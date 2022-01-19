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


  // utility for creating spheres

  // temporary struct used in generate_sphere to create widget_object_instanced::verts
  struct double_vert
  {
    std::array<double, 3> p; //position

    static inline double radius{1.0};

    operator widget_object_instanced::vert() const noexcept
    {
      const double normalf = 32767.0 / std::sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
      const double posf    = radius  / std::sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);

      return widget_object_instanced::vert{{static_cast<short>(posf*p[0]), static_cast<short>(posf*p[1]), static_cast<short>(posf*p[2])}, {static_cast<short>(normalf*p[0]), static_cast<short>(normalf*p[1]), static_cast<short>(normalf*p[2])}};
    }

    double_vert operator+(const double_vert r) const noexcept
    {
      return double_vert{{p[0]+r.p[0],p[1]+r.p[1],p[2]+r.p[2]}};
    }

    double_vert operator*(const double d) const noexcept
    {
      return double_vert{{p[0]*d, p[1]*d, p[2]*d}};
    }

    double_vert operator/(const double d) const noexcept
    {
      return double_vert{{p[0]/d, p[1]/d, p[2]/d}};
    }

    static void set_radius(const double r) noexcept
    {
      radius = r;
    }
  };


  /*consteval*/ std::vector<widget_object_instanced::vert> generate_sphere(const double radius, const int quality) noexcept
  {
    double_vert::set_radius(radius);

    std::vector<double_vert> points;
    std::vector<std::uint32_t> indexes;
    std::vector<widget_object_instanced::vert> triangles;

    if (quality > 0) //icosahedron
    {
      // phi (golden ratio)
      const/*expr*/ double p = ((1.0 + std::sqrt(5.0))/2.0);

      // all rotations of (0, +-1, +-phi))
      points = {{
        {{ 0,  1,  p}}, {{ 0,  1, -p}}, {{ 0, -1,  p}}, {{ 0, -1, -p}},
        {{ 1,  p,  0}}, {{ 1, -p,  0}}, {{-1,  p,  0}}, {{-1, -p,  0}},
        {{ p,  0,  1}}, {{-p,  0,  1}}, {{ p,  0, -1}}, {{-p,  0, -1}}
      }};

      indexes = {{
        0, 4, 6,  1, 4, 6,                     // top 2
        0, 4, 8,  1, 4,10,  0, 6, 9,  1, 6,11, // next 4

        4, 8,10,  5, 8,10,  6, 9,11,  7, 9,11, // 2 vertical diamonds

        0, 2, 8,  0, 2, 9,  1, 3,10,  1, 3,11, // 2 horizontal diamonds

        2, 5, 8,  3, 5,10,  2, 7, 9,  3, 7,11, // 4 connected to bottom 2
        2, 5, 7,  3, 5, 7                      // bottom 2
      }};

      // each point used in 5 triangles of icosahedron
      triangles.reserve(5 * points.size() * quality);
    }
    else if (quality == 0) //octahedron
    {
      points = {{ {{1, 0, 0}}, {{-1, 0, 0}}, {{0, 1, 0}}, {{0, -1, 0}}, {{0, 0, 1}}, {{0, 0, -1}} }};

      indexes = {{ 0,2,4, 0,4,3, 0,3,5, 0,5,2, 1,4,2, 1,3,4, 1,5,3, 1,2,5 }};

      // each point used in 4 triangles of tetrahedron
      triangles.reserve(4 * points.size());
    }
    else //tetrahedron
    {
      // 1/sqrt2
      const/*expr*/ double s = 1.0/std::sqrt(2.0);

      points = {{ {{0, 1, s}}, {{0, -1, s}}, {{1, 0, -s}}, {{-1, 0, -s}} }};

      indexes = {{ 0,1,2, 2,1,3, 2,3,0, 0,3,1 }};

      // each point used in 3 triangles of tetrahedron
      triangles.reserve(3 * points.size());
    }

    std::uint32_t split = std::max(quality, 1);

    for (std::uint32_t i = 0; i < indexes.size(); i+=3)
    {
      auto p0 = points[indexes[i]];
      auto p1 = points[indexes[i+1]];
      auto p2 = points[indexes[i+2]];

      // number of triangles in sphere is 20*split^2

      // for each row
      for (std::uint32_t i = 0; i < split; i++)
      {
        // first triangle in row
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i  )      + p2*i     )/split));
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1) + p1 + p2*i     )/split));
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1)      + p2*(i+1) )/split));

        // for each diamond in the row
        for (std::uint32_t j = 1; j < split - i; j++)
        {
          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-j)   + p1*j     + p2*i     )/split));
          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1-j) + p1*j     + p2*(i+1) )/split));
          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-j)   + p1*(j-1) + p2*(i+1) )/split));

          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-j)   + p1*j     + p2*i     )/split));
          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1-j) + p1*(j+1) + p2*i     )/split));
          triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1-j) + p1*j     + p2*(i+1) )/split));
        }
      }
    }

    return triangles;
  }


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
