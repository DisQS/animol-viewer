#pragma once

#include <memory>

#include "../system/common/ui_event_destination.hpp"
#include "../gpu/shader_texture.hpp"
#include "../system/async.hpp"


namespace plate {

class widget_texture : public ui_event_destination
{

public:

  enum class source  { URL, TEXTURE };
  enum class options { EMPTY, SCALE, STRETCH, CROP, PIXEL_PERFECT };


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, std::uint32_t prop,
                      const std::shared_ptr<ui_event_destination>& parent,
                      texture&& t, options opt) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);

    texture_ = std::move(t);
    source_  = source::TEXTURE;
    options_ = opt;

    upload_vertex();

    upload_uniform();
  };


  void init(const std::shared_ptr<state>& _ui, const gpu::int_box& coords, std::uint32_t prop,
                      const std::shared_ptr<ui_event_destination>& parent,
                      const std::string& url, options opt) noexcept
  {
    ui_event_destination::init(_ui, coords, prop, parent);
    source_  = source::URL;
    url_     = url;
    options_ = opt;

    upload_vertex();

    upload_uniform();

    async_load();
  };


  void async_load()
  {
    auto self(shared_from_this());

    request_handle_ = async::request(url_, "GET", "",
      [this, self] (std::uint32_t handle, data_store&& d) -> void // on_load
      {
        // do decode
        log_debug(FMT_COMPILE("Loaded:: {} size: {}"), url_, d.size());

        if (d.size() > 0)
          async::decode(d, "png", [this, self] (int w, int h, int d, std::byte* data) -> void
          {
            if (data)
            {
              ui_->add_to_command_queue([this, self, data, w, h, d] () -> void
              {
                texture_.upload(data, w * h * d, w, h, d);
                free(data);
              });
            }
          });
      },
  
      [this] (std::uint32_t handle, int error_code, std::string error_msg) -> void // on_error
      {
        log_debug(FMT_COMPILE("Error loading url: {} error: {}"), url_, error_msg);
      },
  
      [this] (std::uint32_t handle, int number_of_bytes, int total_bytes) -> void // on_progress
      {
        log_debug(FMT_COMPILE("on_progress: {} {} / {}"), url_, number_of_bytes, total_bytes);
      }
    );
  }


  void set_click_cb(std::function<void ()> cb) noexcept
  {
    set_input();
    click_cb_ = std::move(cb);
  }
  

  void display() noexcept override
  {
    if (!texture_.ready()) [[unlikely]]
      return;

    ui_->shader_texture_->draw(ui_->projection_, ui_->alpha_.alpha_, uniform_buffer_, vertex_buffer_, texture_);
  }


  void set_geometry(const gpu::int_box& coords) noexcept override
  {
    ui_event_destination::set_geometry(coords);

    upload_uniform();
    upload_vertex();
  }


  //void ui_out() noexcept override
  //{
  //  log_debug("texture got mouse out");
  //}


  bool ui_mouse_position_update() noexcept override
  {
    return true;
  }


  void ui_mouse_button_update() noexcept override
  {
    auto& m = ui_->mouse_metric_;

    if (m.click && click_cb_)
      click_cb_();
  }


  bool ui_touch_update(int id) noexcept override
  {
    auto& m = ui_->touch_metric_[id];

    if (m.click && click_cb_)
      click_cb_();

    return true;
  }


  std::string_view name() const noexcept override
  {
    return "texture";
  }


private:


  void upload_vertex()
  {
    float fw = coords_.p2.x - coords_.p1.x;
    float fh = coords_.p2.y - coords_.p1.y;
  
     auto entry = vertex_buffer_.allocate_staging(4, GL_TRIANGLE_STRIP);

     *entry++ = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
     *entry++ = { 0.0f,   fh, 0.0f, 1.0f, 0.0f, 0.0f };
     *entry++ = {   fw, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
     *entry++ = {   fw,   fh, 0.0f, 1.0f, 1.0f, 0.0f };

     vertex_buffer_.upload();
     vertex_buffer_.free_staging();
  }


  void upload_uniform()
  {
    auto u = uniform_buffer_.allocate_staging(1);
  
    u->offset = { { static_cast<float>(coords_.p1.x) }, { static_cast<float>(coords_.p1.y) }, { 0.0f }, { 0.0f } };
    u->rot    = { { 0.0f }, { 0.0f }, { 0.0f }, { 0.0f } };
    u->scale  = { 1.0f, 1.0f };
  
    uniform_buffer_.upload();
  }



//  void onload(unsigned int handle, void * data, unsigned int data_size) {};
//  void ondecoded(const char * file) {};
//  void ondecodederror() {};
//  void onerror(unsigned int handle, int error_code, const char * error_msg) {};
//  void onprogress(unsigned int handle, int number_of_bytes, int total_bytes) {};

  buffer<shader_texture::basic> vertex_buffer_;
  buffer<shader_texture::ubo>   uniform_buffer_;

  texture texture_;

  bool in_touch_{false};

  source  source_;
  options options_;

  std::string url_;

  int request_handle_{0};

  std::unique_ptr<std::byte>    raw_data_;
  std::uint32_t raw_data_size_{0};

  std::function<void ()> click_cb_;

}; // widget_texture

} // namespace plate
