#pragma once

#include <cstddef>
#include <cstdlib>
#include <vector>

#include <GLES3/gl3.h>

#include "../../system/common/ui_event_destination.hpp"
#include "shaders/compute_template/shader.hpp"

#define VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT 1


/*

  Represents a compute_template

*/


namespace plate {


class compute {

public:

  compute()
  {
  };


  ~compute()
  {
    if (fbo_) glDeleteFramebuffers(1, &fbo_);

    if (input_texture_)  glDeleteTextures(1, &input_texture_);
    if (output_texture_[0]) glDeleteTextures(2, &output_texture_[0]);
  };

  
  std::vector<float> process_cpu()
  {
    log_debug(FMT_COMPILE("computing with cpu width: {} height: {}"), width_, height_);

    std::vector<float> res;

    for (unsigned int i = 0; i < input_values_.size(); ++i)
    {
      float m = input_values_[i * depth_];

      float calc = 0;

      for (float y = 0; y < height_; ++y)
      { 
        for (float x = 0; x < width_; ++x)
        { 
          calc += input_values_[(y * width_ + x) * depth_] * m;
        } 
      } 

      res.push_back(calc);
    }

    return res;
  }


  std::vector<float> process(std::shared_ptr<state>& s)
  {
    log_debug(FMT_COMPILE("computing with gpu width: {} height: {}"), width_, height_);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glDisable(GL_BLEND);

    if (auto err = glGetError(); err != GL_NO_ERROR) { log_error(FMT_COMPILE("got error pA: {}"), err); }

    std::vector<float> res;

    // bind output to a framebuffer with texture

    gpu::float_vec4 inputs = { static_cast<float>(width_), static_cast<float>(height_), 0.0, 0.0};

    s->shader_compute_template_->draw(vertex_buffer_, input_texture_, inputs);

    if (auto err = glGetError(); err != GL_NO_ERROR) { log_error(FMT_COMPILE("got error pB: {}"), err); }

    // download texture

    res.resize(width_ * height_ * depth_);

//    glReadBuffer(GL_COLOR_ATTACHMENT0);

    if (depth_ == 1)
      glReadPixels(0, 0, width_, height_, GL_RED, GL_FLOAT, res.data());
    else // assume 4
      glReadPixels(0, 0, width_, height_, GL_RGBA, GL_FLOAT, res.data());

//    std::vector<float> res2;
//    res2.resize(width_ * height_);
//  
//    glReadBuffer(GL_COLOR_ATTACHMENT1);
//    glReadPixels(0, 0, width_, height_, GL_RED, GL_FLOAT, res2.data());
//
//    LOG_DEBUG("res2 value: %f", res2[0]);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_BLEND);

    if (auto err = glGetError(); err != GL_NO_ERROR) { log_error(FMT_COMPILE("got error pC: {}"), err); }

    return res;
  };


  bool upload(std::shared_ptr<state>& s, std::byte* data, int width, int height, int depth) // depth is number of fields per pixel (eg RGB)
  {
    width_  = width;
    height_ = height;
    depth_  = depth;

    auto T = s->version_ == 2.0 ? GL_RGBA32F : GL_RGBA;

    if (!input_texture_)
    {
      upload_vertex();

      glGenFramebuffers(1, &fbo_);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

      glGenTextures(2, &output_texture_[0]);

      if (!output_texture_[0])
      {
        log_error("Unable to generate texture");
        return false;
      }

      glBindTexture(GL_TEXTURE_2D, output_texture_[0]);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      if (depth_ == 1)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width_, height_, 0, GL_RED, GL_FLOAT, data);
      else // assume 4
        glTexImage2D(GL_TEXTURE_2D, 0, T, width_, height_, 0, GL_RGBA, GL_FLOAT, data);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_texture_[0], 0);

      glBindTexture(GL_TEXTURE_2D, output_texture_[1]);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      if (depth_ == 1)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width_, height_, 0, GL_RED, GL_FLOAT, data);
      else // assume 4
        glTexImage2D(GL_TEXTURE_2D, 0, T, width_, height_, 0, GL_RGBA, GL_FLOAT, data);

//      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, output_texture_[1], 0);

      glGenTextures(1, &input_texture_);

      if (!input_texture_)
      {
        log_error("Unable to generate texture");
        return false;
      }

      glBindTexture(GL_TEXTURE_2D, input_texture_);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      if (auto err = glGetError(); err != GL_NO_ERROR) { log_error(FMT_COMPILE("got error D: {}"), err); }
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, input_texture_);
    }

    if (depth_ == 1)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width_, height_, 0, GL_RED, GL_FLOAT, data);
    else // assume 4
      glTexImage2D(GL_TEXTURE_2D, 0, T, width_, height_, 0, GL_RGBA, GL_FLOAT, data);

    input_values_.clear();
    input_values_.insert(input_values_.begin(), reinterpret_cast<float*>(data),
                                                reinterpret_cast<float*>(data) + (width_ * height_ * depth_));

    if (auto err = glGetError(); err != GL_NO_ERROR) { log_error(FMT_COMPILE("got error Z: {}"), err); }

    return true;
  };


  void upload_vertex()
  {
     auto ptr = vertex_buffer_.allocate_staging(4, 4 * sizeof(float));
     float* vertex = new (ptr) float[4 * 4];

     int offset = 0;

     vertex[offset++] = -1.0f;
     vertex[offset++] = -1.0f;
     vertex[offset++] = 0.0f;
     vertex[offset++] = 1.0f;

     vertex[offset++] = -1.0f;
     vertex[offset++] = 1.0f;
     vertex[offset++] = 0.0f;
     vertex[offset++] = 1.0f;

     vertex[offset++] = 1.0f;
     vertex[offset++] = -1.0f;
     vertex[offset++] = 0.0f;
     vertex[offset++] = 1.0f;

     vertex[offset++] = 1.0f;
     vertex[offset++] = 1.0f;
     vertex[offset++] = 0.0f;
     vertex[offset++] = 1.0f;

     vertex_buffer_.upload();
     vertex_buffer_.free_staging();
  };



  buffer vertex_buffer_;

  int width_{0};
  int height_{0};
  int depth_{0};

  GLuint input_texture_{0};
  GLuint output_texture_[2] = {0};

  GLuint fbo_{0};

  // temp for checking with cpu

  std::vector<float> input_values_;
};
 
} // namespace plate
