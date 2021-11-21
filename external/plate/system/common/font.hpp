#pragma once

#include <functional>
#include <map>
#include <memory>

#include "ui_event_destination.hpp"
#include "ttf_client.hpp"

#include "../../gpu/api.hpp"
#include "../../gpu/gpu.hpp"
#include "../../gpu/texture.hpp"

#include "../async.hpp"

/*
  languages to scripts:

    https://unicode-org.github.io/cldr-staging/charts/37/supplemental/scripts_and_languages.html

    eg: cyrl => Russian,
        grek => Greek,
        latn => English, most European Languages,
        Devanagari => Hindi, most Indian Languages

  Robotico supports scripts:

    cyrl grek latn

  Noto Sans contains most other formats needed: eg: chinese, japanese, korean, Devanagari, Arabic, etc
*/


namespace plate {

class font : public std::enable_shared_from_this<font>
{

public:

  font(std::shared_ptr<state> s) :
    ui_(s)
  {
  };


  ~font()
  {
  };


  void load_font(std::string font_file_name, std::function< void (bool)> cb)
  {
    on_load_callback_ = cb;
    font_file_name_ = font_file_name;
  
    // pull font files accoring to gpu::pixel_ratio

    auto self(shared_from_this());

    async::request(font_file_name, "GET", "",
      [this, self] (unsigned int handle, data_store&& d) mutable // on_load
      {
        this->onload_font(std::move(d));
      },
  
      [this, self] (unsigned int handle, int error_code, std::string error_msg) // on error
      {
        log_error(FMT_COMPILE("failed to get font file: {} with error code: {} msg: {}"),
                                                        font_file_name_, error_code, error_msg);

        if (on_load_callback_)
        {
          on_load_callback_(false);
          on_load_callback_ = nullptr;
        }
      },
  
      [this, self] (unsigned int handle, int number_of_bytes, int total_bytes) // on progress
      {
        log_debug(FMT_COMPILE("file: {} received: {} total is: {}"),
                                              font_file_name_, number_of_bytes, total_bytes);
      }
    );
  };


  std::uint32_t get_max_height() const noexcept
  {
    return (static_cast<double>(size_) * (ttf_.get_yMax() - ttf_.get_yMin())) /
                                          static_cast<double>(ttf_.get_units_per_em());
  };

  std::int32_t get_min_y() const noexcept
  {
    return (static_cast<double>(size_) * ttf_.get_yMin()) /
                                          static_cast<double>(ttf_.get_units_per_em());
  };

  std::uint32_t get_max_width() const noexcept
  {
    return ttf_.get_xMax() - ttf_.get_xMin();
  };


  struct offset
  {
    float x;       // gpu coords of start of letter
    float advance; // gpu coords of start of next letter
    int   index;   // index into text for character
    int   len;     // number of bytes for character
  };

  gpu::float_box generate_vertex(const std::string_view text, std::vector<float>& vertex, float zoom = 1.0,
                                              std::vector<offset>* offset = nullptr)
  {
    float pen_x = 0;
    float pen_y = 0;

    float y_low = 999;
    float y_high = -999;
  
    const char * t = text.data();

    std::uint32_t prev_glyph_id = 0;
  
    for (int i = 0; i < text.size();)
    {
      auto this_i = i;

      const char * current = t + i;

      std::uint32_t unicode = utf8_to_utf32(current);

      i += utf8_surrogate_len(current);

      // find glyph id

      auto glyph_id = ttf_.get_glyph_id(unicode);

      if (glyph_id == 0)
      {
        log_error(FMT_COMPILE("Unable to find: {} unicode: {}"), current, unicode);
        continue;
      }

      // get bounding boxes

      ttf_client::boundings b;

      ttf_.get_bounding(glyph_id, b);

      // get advance

      auto advance = ttf_.get_advance_x(glyph_id);

      //if (advance == 0)
      //{
        //log_debug(FMT_COMPILE("Got a 0 advance for glyph_id: {}"), glyph_id);
        //continue;
      //}

      // get em scale

      float scale = static_cast<double>(size_) / static_cast<double>(ttf_.get_units_per_em());

      // get kerning

      std::pair<std::int16_t, std::int16_t> kern{0,0};

      if (prev_glyph_id)
        kern = ttf_.get_kern_glyph_id(prev_glyph_id, glyph_id);

      if (kern.first)
        pen_x += kern.first * scale * zoom; // adjust pen_x by kerning amount
  
      float x = (pen_x + (b.bounds_l * scale) * zoom) + 0.5;

      b.atlas_l /= atlas_width_;
      b.atlas_b /= atlas_height_;
      b.atlas_r /= atlas_width_;
      b.atlas_t /= atlas_height_;

      float y = (pen_y + (b.bounds_b * scale) * zoom) + 0.5;

      const float x2 = x + (b.bounds_r - b.bounds_l) * scale * zoom;
      const float y2 = y + (b.bounds_t - b.bounds_b) * scale * zoom;
  
      GLfloat glyph_vertex[36] = { x,  y,  0, 1,    b.atlas_l, b.atlas_b,
                                   x2, y,  0, 1,    b.atlas_r, b.atlas_b,
                                   x,  y2, 0, 1,    b.atlas_l, b.atlas_t,
                                   x2, y,  0, 1,    b.atlas_r, b.atlas_b,
                                   x,  y2, 0, 1,    b.atlas_l, b.atlas_t,
                                   x2, y2, 0, 1,    b.atlas_r, b.atlas_t };

      vertex.insert(vertex.end(), glyph_vertex, glyph_vertex + 36);

      if (offset)
        offset->push_back({ pen_x, advance * scale * zoom, this_i, i - this_i });
  
      pen_x += advance * scale * zoom;
  
      if (y < y_low)
        y_low  = y;

      if (y2 > y_high)
        y_high = y2;

      prev_glyph_id = glyph_id;
    }
  
    return { { 0, y_low }, { pen_x, y_high } };
  };


  // utf8 functions


  static int utf8_surrogate_len(const char* c)
  {
    if (!c) return 0;
  
    if ((c[0] & 0x80) == 0) return 1;
  
    auto test_char = c[0];
  
    int result = 0;
  
    while (test_char & 0x80)
    {
      test_char <<= 1;
      result++;
    }
  
    return result;
  };


  static int utf8_strlen(const char* c)
  {
    const char* ptr = c;
    int result = 0;
  
    while (*ptr)
    {
      ptr += utf8_surrogate_len(ptr);
      result++;
    }
  
    return result;
  };


  static std::uint32_t utf8_to_utf32(const char * c)
  {
    std::uint32_t result = -1;
  
    if (!c) return result;
  
    if ((c[0] & 0x80) == 0) result = c[0];
  
    if ((c[0] & 0xC0) == 0xC0 && c[1]) result = ((c[0] & 0x3F) << 6) | (c[1] & 0x3F);
  
    if ((c[0] & 0xE0) == 0xE0 && c[1] && c[2]) result = ((c[0] & 0x1F) << (6 + 6)) | ((c[1] & 0x3F) << 6) | (c[2] & 0x3F);
  
    if ((c[0] & 0xF0) == 0xF0 && c[1] && c[2] && c[3]) result = ((c[0] & 0x0F) << (6 + 6 + 6)) | ((c[1] & 0x3F) << (6 + 6)) | ((c[2] & 0x3F) << 6) | (c[3] & 0x3F);
  
    if ((c[0] & 0xF8) == 0xF8 && c[1] && c[2] && c[3] && c[4]) result = ((c[0] & 0x07) << (6 + 6 + 6 + 6)) | ((c[1] & 0x3F) << (6 + 6 + 6)) | ((c[2] & 0x3F) << (6 + 6)) | ((c[3] & 0x3F) << 6) | (c[4] & 0x3F);
  
    return result;
  };


  auto get_size()         const noexcept { return size_; };
  auto get_atlas_type()   const noexcept { return atlas_type_; };
  auto get_atlas_width()  const noexcept { return atlas_width_; };
  auto get_atlas_height() const noexcept { return atlas_height_; };


  texture texture_;


private:


  void onload_font(data_store&& d)
  {
    font_data_ = std::make_unique<data_store>(std::move(d));

    std::byte* p = font_data_->data();

    if (!ttf_.prepare(p, p + font_data_->size()))
    {
      log_error("Unable to prepare ttf file");
      if (on_load_callback_)
      {
        on_load_callback_(false);
        on_load_callback_ = nullptr;
      }
      return;
    }

    // extract atlas from ttf

    std::byte*    image_data;

    ttf_.get_atlas(atlas_type_, size_, atlas_width_, atlas_height_, atlas_depth_, image_data);
    
    log_debug(FMT_COMPILE("Parsed font file: {} type: {} size: {} dims: {}x{}x{}"),
                                    font_file_name_, atlas_type_, size_, atlas_width_, atlas_height_, atlas_depth_);

    auto self(shared_from_this());

    ui_->add_to_command_queue([this, self, image_data] () mutable
    {
      texture_.upload(image_data, atlas_width_ * atlas_height_ * atlas_depth_, atlas_width_, atlas_height_, atlas_depth_);

      if (on_load_callback_)
      {
        on_load_callback_(true);
        on_load_callback_ = nullptr;
      }
    });
  };


  std::function< void (bool)> on_load_callback_;

  std::string font_file_name_;

  std::uint16_t size_{0};
  ttf_client::ATLAS_TYPE atlas_type_{ttf_client::ATLAS_TYPE::None};

  std::uint16_t atlas_width_{0};
  std::uint16_t atlas_height_{0};
  std::uint16_t atlas_depth_{0};

  int request_handle_{0};

  std::shared_ptr<state> ui_;

  std::unique_ptr<data_store> font_data_; // ugh! shouldn't need this - should be in lambda...??

  ttf_client ttf_;

};


} // namespace movency
