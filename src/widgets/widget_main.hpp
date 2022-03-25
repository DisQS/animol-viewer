#pragma once

#include <array>
#include <type_traits>
#include <utility>

#include "plate.hpp"

#include "system/webgl/worker.hpp"

#include "widgets/widget_rounded_box.hpp"
#include "widgets/widget_object.hpp"
#include "widgets/widget_text.hpp"

#include "widget_layer.hpp"
#include "widget_layer_cartoon.hpp"
#include "widget_layer_spacefill.hpp"

#include "widget_control.hpp"
#include "widget_menu_option.hpp"
#include "widget_menu_download_button.hpp"
#include "widget_menu_fullscreen_button.hpp"
#include "widget_menu_projection_button.hpp"
#include "widget_menu_layer.hpp"

// c++23 std::to_underlying funtion
// should be in #include <utility>
namespace std {
template< class Enum >
constexpr underlying_type_t<Enum> to_underlying( Enum e ) noexcept
{
  return static_cast<std::underlying_type_t<Enum>>(e);
}
} //namespace std


namespace animol {

enum class Mode
{
  invalid,
  viewer,
  window,
  creator,
  example
};


using namespace plate;

class widget_main : public plate::ui_event_destination
{

  struct vert_with_color
  {
    std::array<short,3>        position;
    std::array<short,3>        normal;
    std::array<std::uint8_t,4> color;
  };


public:

  enum class visible { show, hide };
  enum class direction { forward = 1, reverse = -1 };

  friend fmt::formatter<widget_main>;

  std::unique_ptr<worker> worker_; // a multi-threaded worker for performaing background tasks


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords, std::string url, std::string code, std::string description, Mode mode) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display | Prop::Reshape);

    url_         = url;
    item_        = code;
    description_ = description;
    mode_        = mode;

    widget_object_ = ui_event_destination::make_ui<widget_object<vert_with_color>>(ui_, coords_,
                                          Prop::Display | Prop::Input | Prop::Swipe | Prop::Priority, shared_from_this());

    widget_object_->set_display_geometry(false); // this is only used as a controller

    widget_object_->set_user_interacted_cb([this] ()
    {
      interacted();
    });

    worker_ = std::make_unique<worker>();

    worker_->set_path("/animol-viewer/version/1/decoder_worker.js");

    if (url != "" && code != "")
    {
      log_debug(FMT_COMPILE("url: '{}'  item:'{}'"), url_, item_);

      is_remote_ = true;

      if (code.ends_with(".pdb")) // just a single pdb file, so no need for a movie plan
      {
        master_frame_id_ = 0;
        current_entry_   = 0;
        total_frames_    = 1;
        playing_         = false;

        per_frame_files_.push_back(item_);

        item_            = item_.substr(0, item_.size() - 4);

        auto l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);

        layers_.push_back(l);
      }
      else
        load_movie_plan();

      set_title();
    }
    else
      interacted();

    send_keyboard();
  }


  void display() noexcept
  {
    if (current_entry_ < 0 || !playing_ || total_frames_ <= 1)
      return;

    // must be in an animation

    current_time_ += ui_->frame_diff_;

    if (current_time_ >= frame_time_)
      set_next_frame();

    ui_->do_draw(); // keep drawing
  }


  void reshape() noexcept
  { 
    // zoom object if there is more or less space available

    auto cur_min_dim = static_cast<double>(std::min(my_width(), my_height()));
    auto new_min_dim = static_cast<double>(std::min(ui_->pixel_width_, ui_->pixel_height_));

    double z = new_min_dim / cur_min_dim;

    coords_ = { { 0, 0 }, { ui_->pixel_width_, ui_->pixel_height_ } };

    set_title();
    update_loading();

    if (widget_object_)
    {
      widget_object_->set_geometry(coords_);
      widget_object_->zoom(z);
    }

    if (auto w = widget_control_.lock(); w)
    {
      gpu::int_box cc = { coords_.p1, { coords_.p2.x, static_cast<int>(coords_.p1.y + (control_height_ * ui_->pixel_ratio_)) } };
      w->set_geometry(cc);
    }

    for (auto& l : layers_)
      l->set_geometry(coords_);
  }


  void stop() noexcept
  {
    worker_.reset();
  }


  ~widget_main() noexcept
  {
    if (ui_)
      emscripten_webgl_make_context_current(ui_->ctx_);

    log_debug(FMT_COMPILE("widget for {} about to be deleted"), item_);

    clear();
  }


  inline bool is_playing() const noexcept
  {
    return playing_;
  }


  inline void set_playing(bool playing) noexcept
  {
    playing_ = playing;
  }


  inline int get_current_frame() const noexcept
  {
    return current_entry_;
  }

  // get sub-frame position and total frames

  std::pair<float, int> get_frame() const noexcept
  {
    float partial_frame = std::min(1.0f, (current_time_ / frame_time_));

    if (frame_direction_ == direction::forward && current_entry_ < total_frames_ - 1)
      return { current_entry_ + partial_frame, total_frames_ };
    else
      return { current_entry_ - partial_frame, total_frames_ };
  }


  inline bool has_frame() const noexcept
  {
    return total_frames_ > 0;
  }


  bool set_frame(int frame) noexcept
  {
    if (frame < 0)
      frame = 0;

    if (frame >= total_frames_)
      frame = total_frames_ - 1;

    current_time_ = 0;
    current_entry_ = frame;

    for (auto& l : layers_)
      l->set_frame(current_entry_);
  
    return true;
  }


  inline int get_total_frames() const noexcept
  {
    return total_frames_;
  }


  inline bool set_total_frames(int nf) noexcept
  {
    if (total_frames_ == nf)
      return false;

    total_frames_ = nf;

    return true;
  }


  inline auto get_shared_ubuf() noexcept
  {
    return widget_object_->get_uniform_buffer();
  }


  inline void set_scale(float s) noexcept
  {
    scale_has_been_set_ = true;
    widget_object_->set_scale(s);
  }


  inline float get_scale() const noexcept
  {
    return widget_object_->get_scale();
  }


  inline bool scale_has_been_set() const noexcept
  {
    return scale_has_been_set_;
  }


  inline bool is_remote() const noexcept
  {
    return is_remote_;
  }

  constexpr void set_remote(const bool i) noexcept
  {
    is_remote_ = i;
  }


  inline const std::string& get_item() const noexcept
  {
    return item_;
  }


  inline const std::string& get_url() const noexcept
  {
    return url_;
  }


  inline const std::vector<std::string>& get_frame_files() const noexcept
  {
    return per_frame_files_;
  }


  inline int get_master_frame_id() const noexcept
  {
    return master_frame_id_;
  }


  void show_load() noexcept
  {
    ui_event::open_file_show_picker([this] (std::vector<std::string>& files)
    {
      emscripten_webgl_make_context_current(ui_->ctx_);

      log_debug(FMT_COMPILE("selected: {} local files"), files.size());

      if (files.size() > 0)
        start_local("", files);
    });
  }


  void key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8, ui_event::KeyMod mod)
  {
    if (event == ui_event::KeyEventDown)
    {
      interacted();

      if (code_utf8 == "Space" || code_utf8 == "KeyK") // toggle pause
      {
        set_playing(!playing_);

        if (auto w = widget_control_.lock())
          w->update_status();

        return;
      }

      if (code_utf8 == "ArrowLeft" || code_utf8 == "KeyJ") // go back one second
      {
        set_frame(current_entry_ - (1.0 / frame_time_));
        return;
      }

      if (code_utf8 == "ArrowRight" || code_utf8 == "KeyL") // go forwards one second
      {
        set_frame(current_entry_ + (1.0 / frame_time_));
        return;
      }

      if (code_utf8 == "ArrowUp" || code_utf8 == "Period") // go forwards one frame
      {
        set_frame(current_entry_ + 1);
        return;
      }

      if (code_utf8 == "ArrowDown" || code_utf8 == "Comma") // go back one frame
      {
        set_frame(current_entry_ - 1);
        return;
      }

      if (code_utf8 == "KeyP") // cycle visual
      {
        // if there is only one layer, hide it and add the next layer

        if (layers_.size() == 1)
        {
          layers_[0]->set_hidden();

          if (layers_[0]->name() == "layer_spacefill")
          {
            auto l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);
            layers_.push_back(l);
          }

          if (layers_[0]->name() == "layer_cartoon")
          {
            auto l = ui_event_destination::make_ui<widget_layer_spacefill<widget_main>>(ui_, coords_, widget_object_, this);
            layers_.push_back(l);
          }
        }
        else // size == 2
        {
          if (layers_[0]->is_hidden())
          {
            layers_[0]->unset_hidden();
            layers_[1]->set_hidden();
          }
          else
          {
            layers_[0]->set_hidden();
            layers_[1]->unset_hidden();
          }
        }

        return;
      }

      if (code_utf8 == "KeyV") // toggle orthogonal view
      {
        ui_->projection_.set_view(ui_->projection_.is_orthogonal() ?
          projection::view::perspective : projection::view::orthogonal, &ui_->anim_);

        return;
      }

      if (code_utf8 == "KeyF") // toggle fullscreen
      {
        if (auto w = widget_control_.lock())
        {
          if (auto button = dynamic_cast<widget_menu_fullscreen_button<widget_main>*>(w->get_button("fullscreen_button").get()))
            button->toggle_fullscreen();
          else
            log_error(FMT_COMPILE("Attempt to cast to widget_menu_fullscreen_button from incompatible type with name {}"), button->name());
        }
        else
          log_debug(FMT_COMPILE("Attempt to switch to fullscreen ({}) when no fullscreen button present"), code_utf8);

        return;
      }

      if (code_utf8 == "KeyS") // test print style json
      {
        std::string s = fmt::format(FMT_COMPILE("{:s}"), *this);
        log_debug(FMT_COMPILE("json: {}"), s);
        return;
      }
      if (code_utf8 == "KeyD") // test print full json
      {
        std::string s = fmt::format(FMT_COMPILE("{}"), *this);
        log_debug(FMT_COMPILE("json: {}"), s);
        return;
      }
      /*if (code_utf8 == "KeyA") // test set json
      {
        std::string s = "{\"direction\":{ \"i\":0.21085691851366614, \"j\":-0.29399037430736813, \"k\":0.04799895168768374, \"w\":0.9310236948469239, \"normalized\":true }, \"scale\":945.2565}";
        widget_object_->set_to_json(s);
        return;
      }*/

      log_debug(FMT_COMPILE("ignored key: {}"), code_utf8);
    }
  }


  void switch_to_layer(std::string_view name) noexcept
  {
    bool found = false;

    for (auto& l : layers_)
    {
      if (l->name() == name)
      {
        l->unset_hidden();
        found = true;
      }
      else
        l->set_hidden();
    }

    if (!found)
    {
      if (name == "layer_spacefill")
      {
        auto l = ui_event_destination::make_ui<widget_layer_spacefill<widget_main>>(ui_, coords_, widget_object_, this);
        layers_.push_back(l);

        return;
      }
      if (name == "layer_cartoon")
      {
        auto l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);
        layers_.push_back(l);

        return;
      }
    }
  }


  void update_loading() noexcept
  {
    ui_->do_draw();

    std::string loading = loading_error_;

    if (loading.empty())
    {
      if (total_frames_ == 0) // haven't loaded plan/directory listing
      {
        if (item_ == "")
          loading = "";
        else
          loading = "loading: plan";
      }
      else
      {
        int loaded_count = 0;

        for (auto& l : layers_)
          loaded_count += l->get_loaded_count();

        if (loaded_count == layers_.size() * total_frames_) // all loaded
        {
          if (widget_loading_)
            widget_loading_->hide();

          // fixme worker_.reset(); // no more work to do

          return;
        }

        loading = fmt::format(FMT_COMPILE("loaded: {}/{}"), loaded_count / layers_.size(), total_frames_);
      }
    }

    gpu::int_box c = { { coords_.p1.x, static_cast<int>(coords_.p1.y + (control_height_ * ui_->pixel_ratio_)) },
                       coords_.p2 };

    if (widget_loading_)
    {
      widget_loading_->show();
      widget_loading_->set_geometry(c);
      widget_loading_->set_text(loading);
    }
    else
    {
      widget_loading_ = ui_event_destination::make_ui<widget_text>(ui_, c, Prop::Display, shared_from_this(), loading,
                                  gpu::align::BOTTOM | gpu::align::OUTSIDE, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);

      auto fade_in = plate::ui_event_destination::make_anim<plate::anim_alpha>(ui_, widget_loading_, plate::ui_anim::Dir::Forward, 2.0f);
    }
  }


  void set_error(std::string_view error_msg) noexcept
  {
    loading_error_ = error_msg;

    update_loading();
  }


  std::string_view name() const noexcept
  {
    return "#main";
  }


  void start_local(const std::string& dir_name, std::vector<std::string>& files) noexcept
  {
    clear();

    is_remote_       = false;
    description_     = "";
    url_             = "";
    item_            = dir_name;
    per_frame_files_ = files;
    master_frame_id_ = 0;
    current_entry_   = 0;
    current_time_    = 0;
    total_frames_    = files.size();

    auto l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);

    layers_.push_back(l);

    log_debug(FMT_COMPILE("starting local: {} frames: {}"), dir_name, files.size());

    set_title();

    if (auto w = widget_control_.lock())
      w->update_status();
  }


  void interacted() noexcept
  {
    if (auto w = widget_control_.lock(); w)
    {
      w->user_interacted();
    }
    else
    {
      if (visible_control_ == visible::hide)
        return;

      gpu::int_box cc = { coords_.p1, { coords_.p2.x, static_cast<int>(coords_.p1.y + (control_height_ * ui_->pixel_ratio_)) } };
      auto new_w = ui_event_destination::make_ui<widget_control<widget_main>>(ui_, cc, shared_from_this(), this);

      // create menu
      
      new_w->attach_button(ui_event_destination::make_ui<widget_menu_fullscreen_button<widget_main>>(ui_, cc, new_w, this));

      if (visible_menu_option_ == visible::show)
      {
        if (mode_ == Mode::viewer || mode_ == Mode::creator)
        {
          new_w->attach_button(ui_event_destination::make_ui<widget_menu_option           <widget_main>>(ui_, cc, new_w, this));
          new_w->attach_button(ui_event_destination::make_ui<widget_menu_layer            <widget_main>>(ui_, cc, new_w, this));
          new_w->attach_button(ui_event_destination::make_ui<widget_menu_projection_button<widget_main>>(ui_, cc, new_w, this));
        }

        if (mode_ == Mode::viewer || mode_ == Mode::example)
          new_w->attach_button(ui_event_destination::make_ui<widget_menu_download_button  <widget_main>>(ui_, cc, new_w, this));
      }

      widget_control_ = new_w;
    }
  }


  void hold_control() noexcept
  {
    if (auto w = widget_control_.lock(); w)
      w->hold_control();
  }


  void release_control() noexcept
  {
    if (auto w = widget_control_.lock(); w)
      w->release_control();
  }


  // for json

  struct json_main
  {
    std::string_view protein_db;
    std::string_view protein_id;
    std::string_view description;
    projection::view projection;
    std::string_view view;
    int              current_frame;
    bool             playing;
    direction        direction;
    std::string_view controls;
    std::string_view layers;

    static constexpr std::array<std::string_view, 10> lookup_ =
    {{
      "protein_db", "protein_id", "description", "projection", "view", "current_frame", "playing", "direction", "controls", "layers"
    }};
    static constexpr std::uint64_t must_ = mask_of(std::array<std::string_view,0>{}/* 3>{{ "protein_db", "protein_id", "description" }}*/, lookup_);
    static constexpr std::uint64_t may_  = std::numeric_limits<std::uint64_t>::max();
  };


  struct json_layers
  {
    std::string type;
    visible     display;

    static constexpr std::array<std::string_view, 2> lookup_ = {{ "type", "display" }};
    static constexpr std::uint64_t must_ = mask_of(lookup_, lookup_);
    static constexpr std::uint64_t may_  = 0;
  };


  bool start_style_json(std::string_view s) noexcept
  {
    auto h = json_parse_struct<json_main>(s);

    if (!h.ok)
      return false;

    if (h.has(h.mask_of("protein_db")) || h.has(h.mask_of("protein_id")))
    {
      log_error(FMT_COMPILE("Error in start_style_json: attempting to set protein_db and/or protein_id (to {} and {})"), h.data.protein_db, h.data.protein_id);
      return false;
    }

    if (h.has(h.mask_of("description")))
      description_ = h.data.description;

    set_title();

    if (h.has(h.mask_of("projection")))
      ui_->projection_.set_view(h.data.projection);

    if (h.has(h.mask_of("layers")))
    {
      auto hl = json_parse_vector<json_layers>(h.data.layers);

      if (!hl.ok)
        return false;

      layers_config_ = std::move(hl.data);
    }

    if (h.has(h.mask_of("view")))
    {
      if (!widget_object_->set_to_json(h.data.view))
        return false;

      scale_has_been_set_ = true;
    }

    ui_->do_draw();

    return true;
  }

  bool start_json(std::string_view s) noexcept
  {
    auto h = json_parse_struct<json_main>(s);

    if (!h.ok)
      return false;

    clear();

    is_remote_ = true;

    if (h.has(h.mask_of("protein_db")))
      url_         = h.data.protein_db;

    if (h.has(h.mask_of("protein_id")))
      item_        = h.data.protein_id;

    if (h.has(h.mask_of("description")))
      description_ = h.data.description;

    if (h.has(h.mask_of("projection")))
      ui_->projection_.set_view(h.data.projection);

    if (h.has(h.mask_of("view")))
    {
      if (!widget_object_->set_to_json(h.data.view))
        return false;

      scale_has_been_set_ = true;
    }

    if (h.has(h.mask_of("current_frame")))
      current_entry_ = h.data.current_frame;

    if (h.has(h.mask_of("playing")))
      playing_ = h.data.playing;

    if (h.has(h.mask_of("direction")))
      frame_direction_ = h.data.direction;

    set_title();

    if (h.has(h.mask_of("controls")))
    {
      std::array<json_handler_funcs::lookup, 3> cbs = {{
        { "control", false, [&] (std::string_view s) -> bool { return convert(visible_control_,     s); } },
        { "option",  false, [&] (std::string_view s) -> bool { return convert(visible_menu_option_, s); } },
        { "layer",   false, [&] (std::string_view s) -> bool { return convert(visible_menu_layer_,  s); } }
      }};

      auto hc = json_parse_funcs(h.data.controls, cbs);

      if (!hc.ok)
        return false;
    }

    if (h.has(h.mask_of("layers")))
    {
      auto hl = json_parse_vector<json_layers>(h.data.layers);

      if (!hl.ok)
        return false;

      layers_config_ = std::move(hl.data);
    }

    load_movie_plan();

    if (auto w = widget_control_.lock())
    {
      w->disconnect_from_parent();
      w.reset();
    }
      
    return true;
  }


private:


  void load_movie_plan() noexcept
  {
    auto h = plate::async::request(url_ + item_ + "/movie.plan", "GET", "", [this] (std::uint32_t handle, plate::data_store&& d)
    {
      emscripten_webgl_make_context_current(this->ui_->ctx_);

      if (d.empty())
      {
        loading_error_ = "movie.plan is empty";

        log_debug(FMT_COMPILE("{}"), loading_error_);

        update_loading();

        return;
      }

      master_frame_id_ = -1;

      // read in list of pdb files

      std::string_view sv{reinterpret_cast<char*>(d.span().data()), d.span().size()};

      int start;
      int end = -1;

      while ((start = end + 1) != sv.length())
      {
        end = sv.find('\n', start);

        // if there's a blank line, the previous line was the initial structure
        if (end == start)
        {
          master_frame_id_ = std::max(0ul, per_frame_files_.size() - 1);
          continue;
        }

        per_frame_files_.push_back(std::string(sv.substr(start, end - start)));
      }

      total_frames_ = per_frame_files_.size();

      if (total_frames_ == 0)
      {
        loading_error_ = "movie.plan contains no frames";

        log_debug(FMT_COMPILE("{}"), loading_error_);

        update_loading();

        return;
      }

      // if an initial structure wasn't specified, assume it's the middle one

      if (master_frame_id_ == -1)
        master_frame_id_ = std::max(0, total_frames_ / 2 - 1);

      if (current_entry_ == -1)
        current_entry_ = master_frame_id_;

      log_debug(FMT_COMPILE("remote has: {} frames, main: {}"), total_frames_, master_frame_id_);

      std::shared_ptr<widget_layer<widget_main>> l;

      for (auto& lc : layers_config_)
        if (lc.display == visible::show)
        {
          if (lc.type == "layer_cartoon")
          {
            l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);
            break;
          }
          if (lc.type == "layer_spacefill")
          {
            l = ui_event_destination::make_ui<widget_layer_spacefill<widget_main>>(ui_, coords_, widget_object_, this);
            break;
          }
        }

      if (!l)
        l = ui_event_destination::make_ui<widget_layer_cartoon<widget_main>>(ui_, coords_, widget_object_, this);

      layers_.push_back(l);
    },

    [this] (std::uint32_t handle, int error_code, std::string error_string)
    {
      loading_error_ = fmt::format(FMT_COMPILE("unable to load movie.plan: {} {}"), error_code, error_string);

      log_debug(FMT_COMPILE("{}"), loading_error_);

      update_loading();
    },

    {});

    update_loading();
  }


  void set_next_frame() noexcept
  {
    auto next_entry = current_entry_ + std::to_underlying(frame_direction_);
    auto next_frame_direction = frame_direction_;

    if (next_entry < 0)
    {
      next_entry = 0;
      next_frame_direction = direction::forward;
    }
    else
    {
      if (next_entry == total_frames_)
      {
        next_entry = total_frames_ - 1;
        next_frame_direction = direction::reverse;
      }
    }

    for (auto& l : layers_)
      if (!l->set_frame(next_entry))
        return; // not yet ready for next frame

    current_time_    = 0;
    current_entry_   = next_entry;
    frame_direction_ = next_frame_direction;
  }


  void clear()
  {
    playing_         = true;
    current_entry_   = -1;
    total_frames_    = 0;
    frame_direction_ = direction::forward;

    description_ = "";
    item_        = "";
    url_         = "";

    if (!is_remote_)
      for (auto& f : per_frame_files_)
        plate::ui_event::clear_file_object(f);

    per_frame_files_.clear();

    master_frame_id_ = 0;

    for (auto& l : layers_)
      l->disconnect_from_parent();

    layers_.clear();

    widget_object_->set_scale(1.0);

    scale_has_been_set_ = false;

    visible_control_     = visible::show;
    visible_menu_layer_  = visible::show;
    visible_menu_option_ = visible::show;
  }


  void set_title() noexcept
  {
    std::string title{};

    if (description_ == "")
    {
      if (is_remote_)
        title = item_;
      else
      {
        if (per_frame_files_.empty())
          title = "";
        else
          title = fmt::format(FMT_COMPILE("{} local file{}"), per_frame_files_.size(), per_frame_files_.size() == 1 ? "" : "s");
      }
    }
    else
      title = item_ + ": " + description_;

    gpu::int_box c = coords_;
    c.p1.y = my_height() - (70 * ui_->pixel_ratio_);
    c.p2.y = my_height() - (10 * ui_->pixel_ratio_);
    
    if (widget_title_)
    {
      widget_title_->set_text(title);
      widget_title_->set_geometry(c);
    }
    else
    {
      widget_title_ = ui_event_destination::make_ui<widget_text>(ui_, c, Prop::Display, shared_from_this(), title,
                                                        gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);

      auto fade_in = plate::ui_event_destination::make_anim<plate::anim_alpha>(ui_, widget_title_, plate::ui_anim::Dir::Forward, 0.4f);
    }
  }



  std::shared_ptr<plate::widget_text>         widget_title_;
  std::shared_ptr<plate::widget_text>         widget_loading_;

  std::shared_ptr<plate::widget_object<vert_with_color>> widget_object_;

  std::weak_ptr<widget_control<widget_main>> widget_control_;

  std::string loading_error_;

  Mode mode_;

  bool playing_{true};

  int current_entry_{-1};
  int total_frames_{0};

  direction frame_direction_{direction::forward};
  float current_time_{0};
  float frame_time_{1.0/30.0}; // 30 fps

  bool is_remote_{true};
  std::string description_{}; // description of animation

  std::string item_;   // item to load (either a protein code if remote, or a local directory name)
  std::string url_;    // location of remote pdb files

  std::vector<std::string> per_frame_files_;
  int master_frame_id_;

  std::vector<std::shared_ptr<widget_layer<widget_main>>> layers_;

  bool scale_has_been_set_ = false;

  visible visible_control_     {visible::show};
  visible visible_menu_layer_  {visible::show};
  visible visible_menu_option_ {visible::show};

  std::vector<json_layers> layers_config_;

  constexpr static int control_height_ = 60;

}; // class widget_main


} // namespace animol


template <>
struct fmt::formatter<animol::widget_main>
{
  // 'a' means serialize entire state; 's' means only style
  char presentation = 'a';

  constexpr auto parse(format_parse_context& ctx)
  {
    auto it  = ctx.begin();
    auto end = ctx.end();

    if (it != end && (*it == 'a' || *it == 's'))
      presentation = *it++;

    if (it != end && *it != '}')
      throw format_error("invalid format when formatting animol::widget_main");

    return it;
  }

  template<typename FormatContext>
  auto format(const animol::widget_main& w, FormatContext& ctx) const
  {
    log_debug(FMT_COMPILE("w.layers_.size(): {}"), w.layers_.size());
    log_debug(FMT_COMPILE("w.layers_[0]->is_hidden(): {}"), w.layers_[0]->is_hidden());
    log_debug(FMT_COMPILE("w.layers_[0]->name(): {}"), w.layers_[0]->name());

    if (presentation == 's')
      return format_to(ctx.out(), FMT_COMPILE(R"({{ "description":"{}", "projection":"{}", "view":{}, "layers":{{ "display":"{}", "type":"{}" }} }})"),
        w.description_, magic_enum::enum_name(w.ui_->projection_.get_view()), *w.widget_object_, "show", w.layers_[0]->is_hidden()?w.layers_[1]->name():w.layers_[0]->name());
    else
      return format_to(ctx.out(), FMT_COMPILE(R"({{ "protein_db":"{}", "protein_id":"{}", "description":"{}", "projection":"{}", "view":{}, "current_frame":{}, "playing":{}, "direction":"{}", "layers":{{ "display":"{}", "type":"{}" }} }})"),
        w.url_, w.item_, w.description_, magic_enum::enum_name(w.ui_->projection_.get_view()), *w.widget_object_, w.current_entry_, w.playing_, magic_enum::enum_name(w.frame_direction_), "show", w.layers_[0]->is_hidden()?w.layers_[1]->name():w.layers_[0]->name());

  }
};

