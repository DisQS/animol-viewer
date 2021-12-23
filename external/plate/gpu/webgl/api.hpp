#pragma once

#include <cmath>
#include <unistd.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <GLES3/gl3.h>

#include "../../system/common/ui_event.hpp"
#include "../../system/common/font.hpp"
#include "../../system/webgl/ui_event_impl.hpp"
#include "../../system/common/ui_event_destination.hpp"

//#include "shaders/basic/shader.hpp"
#include "shaders/texture/shader.hpp"
//#include "shaders/compute_template/shader.hpp"
//#include "shaders/text/shader.hpp"
#include "shaders/text_msdf/shader.hpp"
#include "shaders/object/shader.hpp"
//#include "shaders/object_instanced/shader.hpp"
//#include "shaders/example_geom/shader.hpp"
//#include "shaders/circle/shader.hpp"
#include "shaders/rounded_box/shader.hpp"

#include "stencil.hpp"
#include "scissor.hpp"


EM_JS(void, js_set_dims, (float w, float h, const char* name, int name_len),
{
  var w_name = UTF8ToString(name, name_len);

  //console.log("name: " + w_name);

  var context = document.getElementById(w_name);
  context.style.width  = w + "px";
  context.style.height = h + "px";
});

EM_JS(void, js_focus, (const char* name, int name_len),
{
  var w_name = UTF8ToString(name, name_len);

  document.getElementById(w_name).focus();
});


namespace plate {


bool is_darkmode()
{
  int x = EM_ASM_INT(
  {
    if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches)
      return 1;
    else
      return 0;
  });

  return x;
};


float get_font_size() // 16 is default/standard
{
  float s = EM_ASM_DOUBLE(
  {
    return parseFloat(getComputedStyle(document.querySelector("body")).fontSize);
  });

  return s;
};


//int ctx_{0};

std::vector<std::pair<int, std::shared_ptr<state>>> windows_;

std::uint64_t current_frame_{0};

// at the moment all callbacks are in main thread - so just run f..

//inline void add_to_command_queue(std::function<void ()> f)
//{
//  f();
//};



EM_BOOL generate_frame(double now, void *userData)
{
//  log_debug("anim");

  for (auto& [ctx, s] : windows_)
  {
    s->set_frame();

    if (s->to_draw_)
    {
      s->set_frame_time(now);

      emscripten_webgl_make_context_current(ctx);

      glViewport(0, 0, s->pixel_width_, s->pixel_height_);

      s->to_draw_ = false;

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // this happens by default

      if (s->pre_draw_)
        s->pre_draw_(current_frame_);

      s->run_command_queue();

      s->display_all();

      s->run_post_command_queue();
    }
  }

  ++current_frame_;

  return EM_TRUE;
}


void generate_frame() // manually force a frame to draw
{
  auto now = ui_event::now();

  generate_frame(now, nullptr);
}



const char* quit([[maybe_unused]] int eventType, [[maybe_unused]] const void *reserved, [[maybe_unused]] void *userData)
{
  return "";
}


std::shared_ptr<plate::state> arch_create(std::string canvas_name, std::string font_file, std::function< void (bool)> cb)
{
  auto s = std::make_shared<plate::state>();

  double w, h;

  //[[maybe_unused]] EMSCRIPTEN_RESULT r = emscripten_get_element_css_size(canvas_name.c_str(), &w, &h);
  //log_debug(FMT_COMPILE("emscripten_get_element_css_size: {}"), r);

  s->viewport_width_ = (int)w;
  s->viewport_height_ = (int)h;

  s->pixel_ratio_screen_ = emscripten_get_device_pixel_ratio();
  s->pixel_ratio_        = s->pixel_ratio_screen_;

  // use float or int versions...?

  s->pixel_width_  = std::floor(s->viewport_width_  * s->pixel_ratio_);
  s->pixel_height_ = std::floor(s->viewport_height_ * s->pixel_ratio_);

  s->stencil_ = std::make_unique<stencil>();
  s->scissor_ = std::make_unique<scissor>();

  log_debug(FMT_COMPILE("canvas: {} viewport: {} {} pixels: {} {} ratio: {}"), canvas_name, w, h,
                                              s->pixel_width_, s->pixel_height_, s->pixel_ratio_);

//  emscripten_set_canvas_element_size(canvas_name.c_str(), s->pixel_width_, s->pixel_height_);
//  js_set_dims((s->pixel_width_ / s->pixel_ratio_), (s->pixel_height_ / s->pixel_ratio_), canvas_name.data()+1, canvas_name.length()-1);

  EmscriptenWebGLContextAttributes ctxAttrs;
  emscripten_webgl_init_context_attributes(&ctxAttrs);

  ctxAttrs.alpha                     = true;
  ctxAttrs.depth                     = true;
  ctxAttrs.stencil                   = true;
  ctxAttrs.antialias                 = false;
  ctxAttrs.preserveDrawingBuffer     = false;
  ctxAttrs.powerPreference           = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
  ctxAttrs.enableExtensionsByDefault = true;
  //ctxAttrs.enableExtensionsByDefault = false;
  ctxAttrs.failIfMajorPerformanceCaveat = false;

  // try and create a webgl2 context first
  // fixme - disabled webgl2

  ctxAttrs.majorVersion = 2;
  ctxAttrs.minorVersion = 0;

//  s->ctx_ = emscripten_webgl_create_context(canvas_name.c_str(), &ctxAttrs);
//
//  if (s->ctx_ <= 0) // unable to create w webgl2 context so try a webgl1 context
//  {
    ctxAttrs.majorVersion = 1;
    s->ctx_ = emscripten_webgl_create_context(canvas_name.c_str(), &ctxAttrs);
    
    if (s->ctx_ <= 0)
    {
      log_error(FMT_COMPILE("Unable to create context for: {}"), canvas_name);
      s.reset();
      return s;
    }
//  }

  emscripten_webgl_make_context_current(s->ctx_);

  // get font loading first

  s->font_ = std::make_shared<plate::font>(s);

  s->font_->load_font(font_file, cb);

  const char ES_VERSION_2_0[] = "OpenGL ES 2.0";
  const char ES_VERSION_3_0[] = "OpenGL ES 3.0";

  std::string version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  if (version.find(ES_VERSION_2_0) != std::string::npos) s->version_ = 1.0;
  if (version.find(ES_VERSION_3_0) != std::string::npos) s->version_ = 2.0;
 
  log_debug(FMT_COMPILE("webgl context version: {} string: {}"), s->version_, version);

  emscripten_set_beforeunload_callback(nullptr, quit);

//  s->shader_basic_                  = new shader_basic(true);
  s->shader_texture_                = new shader_texture(true);
//  s->shader_compute_template_       = new shader_compute_template(true);
//  s->shader_text_                   = new shader_text(true);
  s->shader_text_msdf_              = new shader_text_msdf(true, s->version_);
  s->shader_object_                 = new shader_object(true);
//  s->shader_object_instanced_       = new shader_object_instanced(true);
//  s->shader_example_geom_           = new shader_example_geom(true);
//  s->shader_circle_                 = new shader_circle(true, s->version_);
  s->shader_rounded_box_            = new shader_rounded_box(true, s->version_);

//  s->shader_basic_->link();
  s->shader_texture_->link();
//  s->shader_compute_template_->link();
//  s->shader_text_->link();
  s->shader_text_msdf_->link();
  s->shader_object_->link();
//  s->shader_object_instanced_->link();
//  s->shader_example_geom_->link();
//  s->shader_circle_->link();
  s->shader_rounded_box_->link();

//  s->shader_basic_->check();
  s->shader_texture_->check();
//  s->shader_compute_template_->check();
//  s->shader_text_->check();
  s->shader_text_msdf_->check();
  s->shader_object_->check();
//  s->shader_object_instanced_->check();
//  s->shader_example_geom_->check();
//  s->shader_circle_->check();
  s->shader_rounded_box_->check();

  s->projection_.set(s->pixel_width_, s->pixel_height_);
  //glViewport(0, 0, gpu::viewport_width_, gpu::viewport_height_);
  glViewport(0, 0, s->pixel_width_, s->pixel_height_);

  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

  windows_.emplace_back(s->ctx_, s);

  s->set_name(canvas_name);

  //if (windows_.size() == 1) emscripten_set_main_loop(generate_frame, -1, 0);
  if (windows_.size() == 1) emscripten_request_animation_frame_loop(generate_frame, nullptr);


  std::string add_observer = "Plate_RO.observe(document.querySelector('" + canvas_name + "div'))";
  emscripten_run_script(add_observer.c_str());

  auto device_type = static_cast<ui_event::DeviceType>(ui_event::js_get_device_type());
  auto device_touch = static_cast<ui_event::DeviceHasTouch>(ui_event::js_is_touch_device());

  s->set_darkmode(is_darkmode());

  s->set_font_size(get_font_size() / 20.0);

  log_debug(FMT_COMPILE("canvas: {} device_type: {} device_touch: {} darkmode: {} font_size: {}"),
          canvas_name, ui_event::DeviceType_string(device_type), ui_event::DeviceHasTouch_string(device_touch),
          s->is_darkmode(), s->get_font_size());

  if (s->is_darkmode())
    glClearColor(0.0, 0.0, 0.0, 1.0);
  else
	  glClearColor(1.0, 1.0, 1.0, 1.0);

  js_focus(canvas_name.data() + 1, canvas_name.size() - 1);

  return s;
}



void arch_resize(plate::state* s, int w, int h)
{
  s->pixel_ratio_screen_ = emscripten_get_device_pixel_ratio();
  s->pixel_ratio_        = s->pixel_ratio_screen_;

  s->pixel_width_  = w;
  s->pixel_height_ = h;

  s->viewport_width_ = w / s->pixel_ratio_;
  s->viewport_height_ = h / s->pixel_ratio_;
  
  //log_debug(FMT_COMPILE("canvas: {} viewport: {} {} pixels: {} {} ratio: {}"), s->name_, w, h, s->pixel_width_, s->pixel_height_, s->pixel_ratio_);
  
  emscripten_set_canvas_element_size(s->name_.c_str(), s->pixel_width_, s->pixel_height_);
  js_set_dims((s->pixel_width_ / s->pixel_ratio_), (s->pixel_height_ / s->pixel_ratio_),
                                                               s->name_.data()+1, s->name_.length()-1);

  s->projection_.set(s->pixel_width_, s->pixel_height_);
  glViewport(0, 0, s->pixel_width_, s->pixel_height_);

  s->do_reshape();

  generate_frame(); // generate a frame stright away as otherwise we get an annoying screen blank frame
}


void arch_resize(plate::state* s)
{
  std::string div = s->name_ + "div";

  double w, h;
  [[maybe_unused]] EMSCRIPTEN_RESULT r = emscripten_get_element_css_size(div.c_str(), &w, &h);
  //log_debug(FMT_COMPILE("emscripten_get_element_css_size: {} {} {}"), r, w, h);

  s->viewport_width_ = (int)w;
  s->viewport_height_ = (int)h;

  // use float or int versions...?
  
  s->pixel_width_  = std::floor(s->viewport_width_  * s->pixel_ratio_);
  s->pixel_height_ = std::floor(s->viewport_height_ * s->pixel_ratio_);
  
  //log_debug(FMT_COMPILE("canvas: {} viewport: {} {} pixels: {} {} ratio: {}"), s->name_, w, h, s->pixel_width_, s->pixel_height_, s->pixel_ratio_);
  
  emscripten_set_canvas_element_size(s->name_.c_str(), s->pixel_width_, s->pixel_height_);

  js_set_dims((s->pixel_width_ / s->pixel_ratio_), (s->pixel_height_ / s->pixel_ratio_),
                                                    s->name_.data()+1, s->name_.length()-1);

  s->projection_.set(s->pixel_width_, s->pixel_height_);
  //glViewport(0, 0, gpu::viewport_width_, gpu::viewport_height_);
  glViewport(0, 0, s->pixel_width_, s->pixel_height_);

  s->do_reshape();
}


} // namespace plate

