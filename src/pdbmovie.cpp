#include "plate.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include "widgets/widget_main.hpp"
#include "widgets/widget_texture.hpp"


class movie {

public:

  movie(std::string canvas, std::string url, std::string code) noexcept
  {
    s_ = plate::create(canvas, "/font/Roboto-Regular-32-small-msdf.ttf", [this, url, code] (bool status)
    {
      run(url, code);
    });
  }


private:


  inline void run(std::string url, std::string code) noexcept
  {
    if (!s_)
      return;

    // setup colors

    s_->bg_color_[0] = { 0.99f, 0.99f, 0.99f, 1.0f };
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

    auto w = plate::ui_event_destination::make_ui<pdbmovie::widget_main<false>>(s_, b, url, code);
  };


  std::shared_ptr<plate::state> s_;

}; // class movie



EMSCRIPTEN_BINDINGS(movie)
{
  emscripten::class_<movie>("movie")
    .constructor<std::string, std::string, std::string>()
    ;
}
