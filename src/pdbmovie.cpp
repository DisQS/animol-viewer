#include "plate.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include "widgets/anim_projection.hpp"

#include "widgets/widget_main.hpp"
#include "widgets/widget_texture.hpp"


class movie {

public:


  movie(std::string canvas, std::string url, std::string code, std::string description) noexcept
  {
    url_ = url;
    code_ = code;
    description_ = description;

    s_ = plate::create(canvas, "/font/Roboto-Regular-32-small-msdf.ttf", [this] (bool status)
    {
      run();
    });
  }


  void set(std::string code, std::string description)
  {
    emscripten_webgl_make_context_current(s_->ctx_);

    code_ = code;
    description_ = description;

    if (!w_)    // still waiting for run to start
      return;

    w_->stop();

    if (w_->has_frame()) // there is a protein displayed, so slide it away, and slide new one in
    {
      auto slide_out = plate::ui_event_destination::make_anim<plate::anim_projection>(
                s_, w_, plate::anim_projection::Options::None, -w_->my_width(), 0, 0, plate::ui_anim::Dir::Reverse, 0.4f);

      slide_out->set_end_cb([w(w_)]
      {
        w->disconnect_from_parent();
      });

      run();

      auto slide_in = plate::ui_event_destination::make_anim<plate::anim_projection>(
               s_, w_, plate::anim_projection::Options::None, w_->my_width(), 0, 0, plate::ui_anim::Dir::Forward, 0.4f);
    }
    else // no protein displayed, so quickly fade the current sreen away and show the new protein
    {
      auto fade_out = plate::ui_event_destination::make_anim<plate::anim_alpha>(s_, w_, plate::ui_anim::Dir::Reverse, 0.2f);

      fade_out->set_end_cb([w(w_)]
      {
        w->disconnect_from_parent();
      });

      run();
    }
  }


private:


  inline void run() noexcept
  {
    if (!s_)
      return;

    // setup colors

    s_->bg_color_[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
    s_->bg_color_[1] = { 0.0f, 0.0f, 0.0f, 1.0f };

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

    w_ = plate::ui_event_destination::make_ui<pdbmovie::widget_main<false>>(s_, b, url_, code_, description_);
  }


  std::shared_ptr<plate::state> s_;

  std::string url_;
  std::string code_;
  std::string description_;

  std::shared_ptr<pdbmovie::widget_main<false>> w_;

}; // class movie



EMSCRIPTEN_BINDINGS(movie)
{
  emscripten::class_<movie>("movie")
    .constructor<std::string, std::string, std::string, std::string>()
    .function("set", &movie::set)
    ;
}
