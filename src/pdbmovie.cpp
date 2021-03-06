#include "plate.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include "widgets/anim_projection.hpp"

#include "widgets/widget_main.hpp"
#include "widgets/widget_texture.hpp"


class movie {

public:


  movie(std::string canvas, std::string url, std::string code, std::string description, const std::string mode) noexcept
    : url_(url), code_(code), description_(description),
      mode_(magic_enum::enum_cast<animol::Mode>(mode).has_value() ? magic_enum::enum_cast<animol::Mode>(mode).value() : animol::Mode::invalid)
  {
    if (mode_ == animol::Mode::invalid)
      log_error(FMT_COMPILE("Attempt to create animol-viewer with invalid mode: \"{}\""), mode);

    s_ = plate::create(canvas, "/font/Roboto-Regular-32-small-msdf.ttf", [this] (bool status)
    {
      run();
    });
  }


  void set(std::string code)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    log_debug(FMT_COMPILE("set: {}"), code);

    code_ = code;

    is_remote_ = false;

    json_config_ = "";
    local_files_.clear();

    animate();
  }


  void set_without_changing_style(std::string code)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    log_debug(FMT_COMPILE("set_without_changing_style: {}"), code);

    code_ = code;

    is_remote_ = false;

    json_config_ = "";
    local_files_.clear();

    animate();

    if (!json_style_config_.empty())
    {
      w_->set_json_style_number(json_style_number_);
      if (!w_->start_style_json(json_style_config_))
        log_error("failed parsing json config in set_without_changing_style:");
    }
  }


  void set_protein_and_style_json(std::string code, std::string json_style_config, int json_style_number)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    log_debug(FMT_COMPILE("set_protein_and_style_json: {}, {}, {}"), code, json_style_config, json_style_number);

    code_ = code;

    is_remote_ = false;

    json_config_ = "";
    json_style_config_ = json_style_config;
    json_style_number_ = json_style_number;
    restyle_needed_ = true;

    local_files_.clear();

    animate();
  }


  void set_json(std::string json_config)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    log_debug(FMT_COMPILE("set_json: {}"), json_config);

    json_config_ = json_config;

    is_remote_ = false;

    code_ = "";
    description_ = "";
    url_ = "";

    local_files_.clear();

    animate();
  }


  void set_style_json(std::string json_style_config)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    log_debug(FMT_COMPILE("set_style_json: {}"), json_style_config);

    json_style_config_ = json_style_config;

    if (!w_->start_style_json(json_style_config_))
        log_error("failed parsing json config in set_style_json");
  }


  std::string get_json()
  {
    if (w_)
      return fmt::format(FMT_COMPILE("{}"), *w_);
    else
      return "";
  }


  std::string get_style_json()
  {
    if (w_)
      return fmt::format(FMT_COMPILE("{:s}"), *w_);
    else
      return "";
  }


  void open_local(std::string url_objects)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    code_ = "";
    description_ = "";

    is_remote_ = true;
    is_dcd_    = false;

    local_files_.clear();

    int start;
    int end = 0;

    while ((start = url_objects.find_first_not_of('\n', end)) != std::string::npos)
    {
      end = url_objects.find('\n', start);

      if (end - start > 0)
        local_files_.push_back(url_objects.substr(start, end - start));
    }

    animate();
  }


  void open_local_dcd(std::string url_objects)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    code_ = "";
    description_ = "";

    is_remote_ = true;
    is_dcd_    = true;

    local_files_.clear();

    int start;
    int end = 0;

    while ((start = url_objects.find_first_not_of('\n', end)) != std::string::npos)
    {
      end = url_objects.find('\n', start);

      if (end - start > 0)
        local_files_.push_back(url_objects.substr(start, end - start));
    }

    if (local_files_.size() != 2)
      log_debug(FMT_COMPILE("Bad number of dcd files, wanted 2, got: {}"), local_files_.size());
    else
      animate();
  }


private:


  void animate() noexcept
  {
    if (!w_)    // still waiting for run to start
      return;
 
    w_->stop();
  
    if (w_->has_frame()) // there is a protein displayed, so fade it away, and slide new one in (quickly)
    {
      auto fade_out = plate::ui_event_destination::make_anim<plate::anim_alpha>(s_, w_, plate::ui_anim::Dir::Reverse, 0.1f);
  
      fade_out->set_end_cb([w(w_)]
      {
        w->disconnect_from_parent();
      });

      run();

      auto slide_in = plate::ui_event_destination::make_anim<plate::anim_projection>(
             s_, w_, plate::anim_projection::Options::None, 0, w_->my_height(), 0, plate::ui_anim::Dir::Forward, 0.1f);
    }
    else // no protein displayed, so quickly fade the current screen away and show the new protein
    {
      auto fade_out = plate::ui_event_destination::make_anim<plate::anim_alpha>(s_, w_, plate::ui_anim::Dir::Reverse, 0.2f);

      fade_out->set_end_cb([w(w_)]
      {
        w->disconnect_from_parent();
      });

      run();
    }

    return;
  }


  void run() noexcept
  {
    if (!s_)
      return;

    // setup colors (index 0 is light, index 1 is dark)

    std::array<plate::gpu::color, 2> bg = { {{ 1.0f, 1.0f, 1.0f, 1.0f },
                                             { 0.0f, 0.0f, 0.0f, 1.0f }} };

    s_->set_bg_color(bg);

    s_->bg_color_inv_[0] = s_->bg_color_[1];
    s_->bg_color_inv_[1] = s_->bg_color_[0];

    s_->txt_color_[0] = { 31.0/255.0, 131.0/255.0, 201.0/255.0, 1.0 };
    s_->txt_color_[1] = { 31.0/255.0, 131.0/255.0, 201.0/255.0, 1.0 };

    s_->select_color_[0] = { (s_->txt_color_[0].r + s_->bg_color_[0].r)/2.0f,
                             (s_->txt_color_[0].g + s_->bg_color_[0].g)/2.0f,
                             (s_->txt_color_[0].b + s_->bg_color_[0].b)/2.0f,
                             1.0f };
    s_->select_color_[1] = { (s_->txt_color_[1].r + s_->bg_color_[1].r)/2.0f,
                             (s_->txt_color_[1].g + s_->bg_color_[1].g)/2.0f,
                             (s_->txt_color_[1].b + s_->bg_color_[1].b)/2.0f,
                             1.0f };

    s_->err_txt_color_[0] = { 215.0/255.0, 0.0/255.0, 0.0/255.0, 1.0 };
    s_->err_txt_color_[1] = { 215.0/255.0, 0.0/255.0, 0.0/255.0, 1.0 };

    plate::gpu::int_box b = { { 0, 0 }, { s_->pixel_width_, s_->pixel_height_ } };

    // start main widget

    w_ = plate::ui_event_destination::make_ui<animol::widget_main>(s_, b, url_, code_, description_, mode_);

    if (restyle_needed_)
    {
      restyle_needed_ = false;

      w_->set_json_style_number(json_style_number_);

      if (!w_->start_style_json(json_style_config_))
        log_error("failed parsing json config in run");
    }

    if (is_remote_)
    {
      if (is_dcd_)
      {
        w_->start_local_dcd(local_files_[0], local_files_[1]);
      }
      else
        w_->start_local("", local_files_);

      local_files_.clear();

      return;
    }

    if (!json_config_.empty())
    {
      if (!w_->start_json(json_config_))
        log_debug("failed parsing json config");

      return;
    }
  }


  std::shared_ptr<plate::state> s_;

  std::vector<std::string> local_files_;

  std::string json_config_;
  std::string json_style_config_;
  int json_style_number_{0};
  bool restyle_needed_{false}; // indicates json_style_config_ and json_style_number_ need to be applied

  std::string url_;
  std::string code_;
  std::string description_;

  bool is_remote_{false};
  bool is_dcd_{false};

  std::shared_ptr<animol::widget_main> w_{};

  const animol::Mode mode_;

}; // class movie



EMSCRIPTEN_BINDINGS(movie)
{
  emscripten::class_<movie>("movie")
    .constructor<std::string, std::string, std::string, std::string, std::string>()
    .function("set",                        &movie::set)
    .function("set_without_changing_style", &movie::set_without_changing_style)
    .function("set_protein_and_style_json", &movie::set_protein_and_style_json)
    .function("set_json",                   &movie::set_json)
    .function("set_style_json",             &movie::set_style_json)
    .function("get_json",                   &movie::get_json)
    .function("get_style_json",             &movie::get_style_json)
    .function("open_local",                 &movie::open_local)
    .function("open_local_dcd",             &movie::open_local_dcd)
    ;
}
