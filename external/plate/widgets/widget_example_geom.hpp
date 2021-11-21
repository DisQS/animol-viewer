#pragma once
  
#include <cstddef>
#include <cstdlib>
#include <vector>

#include "../system/log.hpp"
#include "../system/common/ui_event_destination.hpp"

#include "../gpu/gpu.hpp"
#include "../gpu/api.hpp"
#include "../gpu/texture.hpp"

/* draw a triangle for a given 3d point

  src: list of points as: x1, y1, z1 ... xn, yn, zn

  objective: draw a triangle with centre at each point

  method:

    store x1,y1,z1 in a texture float32 texture (fixme - uint32t?)

    create an attirbute buffer with (1, 0), (1, 1), (1, 2) .. (n, 0), (n, 1), (n, 2)
     NB: in webgl2 should be able to use: gl_VertexID to get the vertex index position so buffer would become:
      (0),(1),(2),(0),(1),(2) ... or could calc that as well and buffer is simply 0,0,0,0,0 - just to identify
      number of points

      that is to say: first point is at point index 1 and this is the 1st corner... up to the 3rd

    then:

      the vertex shader gets the buffer and texture as inputs and calculates the triangle vertexes for each point
      and output the appropriate position

*/


namespace plate {

class widget_example_geom : public ui_event_destination
{


public:


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, const std::uint32_t prop,
                                              const std::array<gpu::color, 2>& c, float radius) noexcept
  {
    ui_event_destination::init(_ui, coords, prop);
    color_  = c;
    radius_ = radius;

    uniform_buffer_.usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  };


  ~widget_example_geom()
  {
    if (input_texture_)
      glDeleteTextures(1, &input_texture_);
  };
  

  void display() noexcept
  {
    ui_->shader_example_geom_->draw(ui_->projection_, uniform_buffer_, vertex_buffer_, input_texture_,
              color_[ui_->color_mode_]);
  };


  bool upload_points(std::span<float> p)
  {
    auto entries = p.size() / 3; // x,y,z

    if (entries < 4'096)
    {
      width_ = entries;
      height_ = 1;
    }
    else
    {
      width_ = 4'096;
      height_ = (entries / 4'096) + 1;
    }

    if (input_texture_)
      glDeleteTextures(1, &input_texture_);

    glGenTextures(1, &input_texture_);

    if (!input_texture_)
    {
      log_error("Unable to generate texture");
      return false;
    }

    glBindTexture(GL_TEXTURE_2D, input_texture_);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    auto T = ui_->version_ == 2.0 ? GL_RGB32F : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, T, width_, height_, 0, GL_RGB, GL_FLOAT, p.data());


    // create dummy attribute buffer, each point becomes 3 vertex points, each with an index, triangle_point pair

    auto ptr = vertex_buffer_.allocate_staging(entries * 3, 2 * sizeof(float));

    float* vertex = new (ptr) float[entries * 3 * 2];

    int j = 0;
    for (int i = 0; i < entries; ++i)
    {
      vertex[j++] = i;
      vertex[j++] = 0;
      vertex[j++] = i;
      vertex[j++] = 1;
      vertex[j++] = i;
      vertex[j++] = 2;
    }

    vertex_buffer_.upload();
    vertex_buffer_.free_staging();

    upload_uniform();

    return true;
  };


private:


  void upload_uniform()
  {
    auto ptr = uniform_buffer_.allocate_staging(1, sizeof(shader_example_geom::ubo));

    shader_example_geom::ubo* u = new (ptr) shader_example_geom::ubo;

    u->offset  = { {offset_x_}, {offset_y_}, {0}, {0} };
    u->tex_dim = { static_cast<float>(width_), static_cast<float>(height_) };
    u->radius  = radius_;

    uniform_buffer_.upload();
  };



  buffer vertex_buffer_;
  buffer uniform_buffer_;
  
  int width_{0};
  int height_{0};
  int depth_{3};

  std::array<gpu::color, 2> color_;
  float radius_;

  float offset_x_{0};
  float offset_y_{0};
  
  GLuint input_texture_{0};
  

};

} // namespace plate
