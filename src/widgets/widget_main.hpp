#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "system/webgl/worker.hpp"

#include "widgets/widget_rounded_box.hpp"
#include "widgets/widget_object.hpp"
#include "widgets/widget_object_instanced.hpp"
#include "widgets/widget_text.hpp"

#include "widget_control.hpp"


namespace pdbmovie {

using namespace plate;

template<bool SHARED_COLORS>
class widget_main : public plate::ui_event_destination
{

  struct compact_header
  {
    int num_triangles;
    int num_strips;
  };

  // can operate on data with interleaved colors, or not.

  struct vert_with_color
  {
    std::array<short,3>        position;
    std::array<short,3>        normal;
    std::array<std::uint8_t,4> color;
  };

  struct vert_without_color
  {
    std::array<short,3>        position;
    std::array<short,3>        normal;
  };

  struct cols_sep
  {
    std::array<std::uint8_t,4> color;
  };


  typedef typename std::conditional<SHARED_COLORS, vert_without_color, vert_with_color>::type  vert;
  typedef typename std::conditional<SHARED_COLORS, cols_sep,           bool>::type             cols;


public:


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords, std::string url, std::string code, std::string description) noexcept
  {
    ui_event_destination::init(_ui, coords, Prop::Display | Prop::Reshape);
    log_debug(FMT_COMPILE("url: '{}'  code:'{}'"), url, code);

    if (url != "" && code != "")
      start_remote(url, code, description);
    else
      create_control();

    send_keyboard();
  }


  void display() noexcept
  {
    if (current_entry_ < 0)
      return;

    if (widget_object_instanced_)
      widget_object_instanced_->set_scale_and_direction(widget_object_->get_scale(), widget_object_->get_direction());

    if (!playing_ || pdb_list_.size() <= 1)
      return;

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
    set_loading();

    if (widget_object_)
    {
      widget_object_->set_geometry(coords_);
      widget_object_->zoom(z);

      if (widget_object_instanced_)
      {
        widget_object_instanced_->set_geometry(coords_);
        widget_object_instanced_->set_scale_and_direction(widget_object_->get_scale(), widget_object_->get_direction());
      }
    }

    if (auto w = widget_control_.lock(); w)
    {
      gpu::int_box cc = { coords_.p1, { coords_.p2.x, static_cast<int>(coords_.p1.y + (control_height_ * ui_->pixel_ratio_)) } };
      w->set_geometry(cc);
    }
  }


  void stop() noexcept
  {
    worker_.reset();
  }

  ~widget_main() noexcept
  {
    if (ui_)
      emscripten_webgl_make_context_current(ui_->ctx_);

    log_debug(FMT_COMPILE("widget for {} deleted"), item_);
  }


  inline bool is_playing() const noexcept
  {
    return playing_;
  }


  inline void set_playing(bool playing) noexcept
  {
    playing_ = playing;
  }


  std::pair<float, int> get_frame() const noexcept
  {
    float partial_frame = std::min(1.0f, (current_time_ / frame_time_));

    if (frame_direction_ == 1 && current_entry_ < pdb_list_.size() - 1)
      return { current_entry_ + partial_frame, pdb_list_.size() };
    else
      return { current_entry_ - partial_frame, pdb_list_.size() };
  }


  inline bool has_frame() const noexcept
  {
    return widget_object_.get() != nullptr;
  }


  bool set_frame(int frame) noexcept
  {
    if (store_.empty())
      return false;

    if (frame < 0)
      frame = 0;

    if (frame >= store_.size())
      frame = store_.size() - 1;

    if (store_[frame].vertex_strip.is_ready())
    {
      current_time_ = 0;
      current_entry_ = frame;
  
      widget_object_->set_model(&(store_[current_entry_].vertex),       &shared_vertex_colors_);
      widget_object_->add_model(&(store_[current_entry_].vertex_strip), &shared_vertex_strip_colors_);

      return true;
    }

    return false;
  }


  void show_load() noexcept
  {
    ui_event::open_file_show_picker([this] (std::vector<std::string>& files)
    {
      emscripten_webgl_make_context_current(ui_->ctx_);

      log_debug(FMT_COMPILE("selected: ({}) {}"), files.size(), fmt::join(files, " "));

      if (files.size() > 0)
        start_local("", files);
    });

//    ui_event::directory_show_picker(".*\\.pdb$", [this] (std::string dir_name, std::vector<std::string>& files)
//    {
//      emscripten_webgl_make_context_current(ui_->ctx_);
//
//      if (files.size() > 0)
//        start_local(dir_name, files);
//    });
  }


  void key_event(ui_event::KeyEvent event, std::string_view utf8, std::string_view code_utf8, ui_event::KeyMod mod)
  {
    if (event == ui_event::KeyEventDown)
    {
      create_control();

      if (code_utf8 == "Space")
      {
        set_playing(!playing_);

        if (auto w = widget_control_.lock())
          w->update_status();

        return;
      }

      if (code_utf8 == "ArrowLeft") // go back one second
      {
        set_frame(current_entry_ - (1.0 / frame_time_));
        return;
      }

      if (code_utf8 == "ArrowRight") // go forwards one second
      {
        set_frame(current_entry_ + (1.0 / frame_time_));
        return;
      }

      if (code_utf8 == "ArrowUp") // go forwards one frame
      {
        set_frame(current_entry_ + 1);
        return;
      }

      if (code_utf8 == "ArrowDown") // go back one frame
      {
        set_frame(current_entry_ - 1);
        return;
      }

      if (code_utf8 == "KeyP") // testing own visualisation
      {
        create_test_instanced();
        return;
      }

      log_debug(FMT_COMPILE("ignored key: {}"), code_utf8);
    }
  }


  std::string_view name() const noexcept
  {
    return "#main";
  }


private:


  void start_remote(std::string url, std::string code, std::string description) noexcept
  {
    clear();

    local_       = false;
    item_        = code;
    url_         = url;
    description_ = description;

    if (auto w = widget_control_.lock())
      w->update_status();

    if (item_.ends_with(".pdb")) // no nedd for a movie.plan
    {
      pdb_list_.push_back(item_);
      item_ = item_.substr(0, item_.size() - 4);

      current_entry_ = 0;
      store_.resize(1);

      set_title();

      log_debug("remote has: 1 frame");

      generate_script();

      return;
    }

    // load in movie plan file

    set_title();
    set_loading();

    auto h = async::request(url_ + item_ + "/movie.plan", "GET", "", [this] (std::uint32_t handle, plate::data_store&& d)
    {
      emscripten_webgl_make_context_current(ui_->ctx_);

      if (d.empty())
      {
        log_debug("movie.plan is empty");
        return;
      }

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
          current_entry_ = pdb_list_.size() - 1;
          continue;
        }

        pdb_list_.push_back(std::string(sv.substr(start, end - start)));
      }

      auto count = pdb_list_.size();

      // if an initial structure wasn't specified, assume it's the middle one

      if (current_entry_ == -1)
        current_entry_ = count / 2 - 1;

      // setup the load order- from initial+1 up, and then from initial-1 down..

      for (int i = current_entry_ + 1; i < count; ++i)
        to_load_.push(i);

      for (int i = current_entry_ - 1; i >= 0; --i)
        to_load_.push(i);

      store_.resize(count);

      log_debug(FMT_COMPILE("remote has: {} frames, main: {}"), count, current_entry_);

      generate_script();
    },
    {}, {}); // fixme, todo - handle failed to load
  }


  void start_local(const std::string& dir_name, std::vector<std::string>& files) noexcept
  {
    clear();

    local_       = true;
    item_        = dir_name;
    description_ = "";
    pdb_list_    = std::move(files);

    log_debug(FMT_COMPILE("starting local: {} frames: {}"), item_, pdb_list_.size());

    set_title();

    // start loading from the first entry

    current_entry_ = 0;

    // setup the load order- from initial+1 up

    for (int i = current_entry_ + 1; i < pdb_list_.size(); ++i)
      to_load_.push(i);

    store_.resize(pdb_list_.size());

    generate_script();
  }

  
  void generate_script() noexcept
  {
    ui_->do_draw();
    set_loading();

    if (auto w = widget_control_.lock())
      w->update_status();

    auto old_worker = std::move(worker_); // delete the old one (if any) after we have created a new one

    worker_ = std::make_unique<worker>();

    worker_->set_path("/animol-viewer/version/1/decoder_worker.js");

    // display_style_ = 1; // hack

    if (local_)
    {
      // load in file and then request the worker to generate the script

      ui_event::directory_load_file(item_, pdb_list_[current_entry_], [this, self{shared_from_this()}] (std::string&& pdb)
      {
        if (!worker_)
          return;

        if (display_style_ == 1) // ball-and-stick
        {
          script_ = "title \"na\"\nplot\nread mol \"/i.pdb\";\ntransform atom * by centre position atom *;\nset colourparts on;\nset segments 8;\nball-and-stick atom *;\nend_plot\n";
          get_first_frame(pdb);
          return;
        }
        if (display_style_ == 2) // cpk
        {
          script_ = "title \"na\"\nplot\nread mol \"/i.pdb\";\ntransform atom * by centre position atom *;\nset colourparts on;\nset segments 8;\ncpk atom *;\nend_plot\n";
          get_first_frame(pdb);
          return;
        }

        // display_style_ == 0 // cartoon

        worker_->call("script_with", pdb, [this, self{shared_from_this()}, pdb] (std::span<char> d)
        {
          script_ = std::string(d.data(), d.size());

          get_first_frame(pdb);
        });
      });
    }
    else
    {
      if (display_style_ == 1) // ball-and-stick
      {
        script_ = "title \"na\"\nplot\nread mol \"/i.pdb\";\ntransform atom * by centre position atom *;\nset colourparts on;\nset segments 8;\nball-and-stick atom *;\nend_plot\n";
        get_first_frame();
        return;
      }
      if (display_style_ == 2) // cpk
      {
        script_ = "title \"na\"\nplot\nread mol \"/i.pdb\";\ntransform atom * by centre position atom *;\nset colourparts on;\nset segments 8;\ncpk atom *;\nend_plot\n";
        get_first_frame();
        return;
      }

      // request the worker to download the pdb file and generate the script

      std::string u = url_ + item_ + "/" + pdb_list_[current_entry_];

      if (!worker_)
        return;

      worker_->call("script", u, [this, self{shared_from_this()}] (std::span<char> d)
      {
        script_ = std::string(d.data(), d.size());
        get_first_frame();
      });
    }
  }


  void get_first_frame(std::string pdb = "") noexcept
  {
    // script has been generated, so run the decoder with the main frame,
    //
    // once the main frame has been generated, upload the script to all the workers and start processing the subsequent frames


    if (local_)
    {
      std::vector<char> data_to_send(4);
        
      std::uint32_t script_size = script_.size();
        
      std::memcpy(data_to_send.data(), &script_size, 4);
        
      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),     pdb.begin(),     pdb.end());
        
      if (!worker_)
        return;
  
      worker_->call(SHARED_COLORS ? "decode_contents_color_non_interleaved" : "decode_contents_color_interleaved", data_to_send,
        [this, self{shared_from_this()}] (std::span<char> d)
      {
        emscripten_webgl_make_context_current(ui_->ctx_);

        process_main_frame(d);
      });
    }
    else // remote
    {
      std::string u = url_ + item_ + "/" + pdb_list_[current_entry_];

      std::vector<char> data_to_send(4);

      std::uint32_t script_size = script_.size();

      std::memcpy(data_to_send.data(), &script_size, 4);

      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),       u.begin(),       u.end());

      if (!worker_)
        return;

      worker_->call(SHARED_COLORS ? "decode_url_color_non_interleaved" : "decode_url_color_interleaved", data_to_send,
        [this, self{shared_from_this()}] (std::span<char> d)
      {
        emscripten_webgl_make_context_current(ui_->ctx_);

        process_main_frame(d);
      });
    }
  }


  void process_next() noexcept
  {
    if (to_load_.empty()) // nothing left to ask for
      return;

    if (!worker_)
      return;

    auto to_load = to_load_.front();
    to_load_.pop();

    if (local_)
    {
      ui_event::directory_load_file(item_, pdb_list_[to_load], [this, entry = to_load] (std::string&& pdb)
      {
        std::vector<char> data_to_send(4);

        std::uint32_t script_size = script_.size();

        std::memcpy(data_to_send.data(), &script_size, 4);

        data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
        data_to_send.insert(data_to_send.end(),     pdb.begin(),     pdb.end());

        if (!worker_)
          return;

        worker_->call(SHARED_COLORS ? "decode_contents_no_color" : "decode_contents_color_interleaved", data_to_send,
          [this, self{shared_from_this()}, entry] (std::span<char> d)
        {
          emscripten_webgl_make_context_current(ui_->ctx_);
          process_decoded(entry, d);
        });
      });
    }
    else
    {
      std::string u = url_ + item_ + "/" + pdb_list_[to_load];

      std::vector<char> data_to_send(4);

      std::uint32_t script_size = script_.size();

      std::memcpy(data_to_send.data(), &script_size, 4);

      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),       u.begin(),       u.end());

      worker_->call(SHARED_COLORS ? "decode_url_no_color" : "decode_url_color_interleaved", data_to_send,
        [this, self{shared_from_this()}, entry = to_load] (std::span<char> d)
      { 
        emscripten_webgl_make_context_current(ui_->ctx_);
        process_decoded(entry, d);
      });
    }
  }


  void process_decoded(int entry, std::span<char> d) noexcept
  {
    compact_header* h = new (d.data()) compact_header;

    auto p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

    //log_debug(FMT_COMPILE("frame: {} triangles: {} strips: {}"), entry, h->num_triangles, h->num_strips);

    store_[entry].vertex.upload(p, h->num_triangles, GL_TRIANGLES);
    p += h->num_triangles * sizeof(vert);

    store_[entry].vertex_strip.upload(p, h->num_strips, GL_TRIANGLE_STRIP);

    process_frame(entry);
    process_next();
  }


  void process_main_frame(std::span<char> d) noexcept
  {
    // main frame has been converted to geometry

    compact_header* h = new (d.data()) compact_header;

    auto p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

    log_debug(FMT_COMPILE("main frame triangles: {} strips: {}"), h->num_triangles, h->num_strips);

    // upload the vertices

    store_[current_entry_].vertex.upload(p, h->num_triangles, GL_TRIANGLES);
    p += h->num_triangles * sizeof(vert);
        
    store_[current_entry_].vertex_strip.upload(p, h->num_strips, GL_TRIANGLE_STRIP);
    p += h->num_strips * sizeof(vert);

    if constexpr (SHARED_COLORS) // upload the colors
    {
      shared_vertex_colors_.upload(p, h->num_triangles);
      p += h->num_triangles * sizeof(cols);

      shared_vertex_strip_colors_.upload(p, h->num_strips);
    }

    // start number_of_worker_threads processing

    for (int i = 0; i < worker::get_max_workers(); ++i)
      process_next();

    widget_object_ = ui_event_destination::make_ui<widget_object<vert, cols>>(ui_, coords_,
                                          Prop::Display | Prop::Input | Prop::Swipe | Prop::Priority, shared_from_this());

    widget_object_->set_model(&store_[current_entry_].vertex,       &shared_vertex_colors_);
    widget_object_->add_model(&store_[current_entry_].vertex_strip, &shared_vertex_strip_colors_);

    widget_object_->set_user_interacted_cb([this] ()
    {
      create_control();
    });

    auto fade_in = plate::ui_event_destination::make_anim<plate::anim_alpha>(ui_, widget_object_, plate::ui_anim::Dir::Forward, 0.3f);

    // loop through data to find protein width

    p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

    int max = 0;

    auto v = reinterpret_cast<vert*>(p);

    for (int i = 0; i < h->num_triangles + h->num_strips; ++i, ++v)
    {
      if (abs(v->position[0]) > max) max = abs(v->position[0]);
      if (abs(v->position[1]) > max) max = abs(v->position[1]);
      if (abs(v->position[2]) > max) max = abs(v->position[2]);
    }

    // values are int16 and so range from +/- 32'768
    //
    // to ensure it fits in the 'screen', calculate the minimum screen dimension, halve it to get the width and then scale
    // according to the max value found
    //
    // scale it a bit smaller to cope with movement from the central main frame we're looking at FIXME - calc dynamically from other
    // frames?

    log_debug(FMT_COMPILE("max width: {}"), max);

    widget_object_->set_scale((std::min(my_width(), my_height())/2.0) * (32'768.0 / max) * 0.8);

    process_frame(current_entry_);
  }


  void process_frame(int entry) noexcept
  {
    ++loaded_count_;

    if (loaded_count_ == pdb_list_.size())
    {
      log_debug(FMT_COMPILE("all loaded: {}"), loaded_count_);
    }

    set_loading();

    ui_->do_draw();
  }


  void set_next_frame() noexcept
  {
    auto next_entry = current_entry_ + frame_direction_;
    auto next_frame_direction = frame_direction_;

    if (next_entry < 0)
    {
      next_entry = 0;
      next_frame_direction = 1;
    }
    else
    {
      if (next_entry == pdb_list_.size())
      {
        next_entry = pdb_list_.size() - 1;
        next_frame_direction = -1;
      }
    }

    if (store_[next_entry].vertex_strip.is_ready())
    {
      current_time_ = 0;
      current_entry_ = next_entry;
      frame_direction_ = next_frame_direction;

      widget_object_->set_model(&(store_[current_entry_].vertex),       &shared_vertex_colors_);
      widget_object_->add_model(&(store_[current_entry_].vertex_strip), &shared_vertex_strip_colors_);
    }
  }


  void clear()
  {
    store_.clear();

    std::queue<int> x;
    x.swap(to_load_);

    loaded_count_ = 0;

    current_entry_ = -1;

    item_ = "";

    pdb_list_.clear();

    url_  = "";

    script_.clear();

    if (widget_object_instanced_)
    {
      widget_object_instanced_->disconnect_from_parent();
      widget_object_instanced_.reset();
    }

    if (widget_object_)
    {
      widget_object_->disconnect_from_parent();
      widget_object_.reset();
    }
  }


  void set_title() noexcept
  {
    std::string title{};

    if (description_ == "")
      title = local_ ? "local: " + item_ : item_;
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


  void set_loading() noexcept
  {
    std::string loading;

    if (current_entry_ == -1) // haven't loaded plan/directory listing
    {
      if (item_ == "")
        loading = "";
      else
        loading = "loading: plan";
    }
    else
    {
      if (loaded_count_ == pdb_list_.size()) // all loaded
      {
        if (widget_loading_)
          widget_loading_->hide();

        worker_.reset(); // no more work to do

        return;
      }

      loading = fmt::format(FMT_COMPILE("loaded: {}/{}"), loaded_count_, pdb_list_.size());
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


  void create_control() noexcept
  {
    if (auto w = widget_control_.lock(); w)
    {
      w->user_interacted();
    }
    else
    {
      gpu::int_box cc = { coords_.p1, { coords_.p2.x, static_cast<int>(coords_.p1.y + (control_height_ * ui_->pixel_ratio_)) } };
      widget_control_ = ui_event_destination::make_ui<widget_control<widget_main>>(ui_, cc, shared_from_this(), this);
    }
  }


  // temporary struct used in generate_sphere to create widget_object_instanced::verts
  struct double_vert
  {
    std::array<double, 3> p; //position

    static inline double radius{1.0};

    operator widget_object_instanced::vert() const noexcept
    {
      const double normalf = 32767.0 / std::sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
      const double posf = radius / std::sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);

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


  /*consteval*/ std::vector<widget_object_instanced::vert> generate_sphere(const double radius) noexcept
  {
    double_vert::set_radius(radius);

    const/*expr*/ double phi    = ((1.0 + std::sqrt(5.0))/2.0);
    const/*expr*/ double factor = std::sqrt(phi + 2.0);

    // normals
    const/*expr*/ double pa = 32767.0 * 1.0 / factor;
    const/*expr*/ double pb = 32767.0 * phi / factor;
    const/*expr*/ double ma = -pa;
    const/*expr*/ double mb = -pb;

    // positions
    const/*expr*/ double pA = radius * 1.0 / factor;
    const/*expr*/ double pB = radius * phi / factor;
    const/*expr*/ double mA = -pA;
    const/*expr*/ double mB = -pB;

    /*const/expr/ std::vector<double_vert> p = {{
      {{ 0, pA, pB}, { 0, pa, pb}}, {{ 0, pA, mB}, { 0, pa, mb}}, {{ 0, mA, pB}, { 0, ma, pb}}, {{ 0, mA, mB}, { 0, ma, mb}},
      {{pA, pB,  0}, {pa, pb,  0}}, {{pA, mB,  0}, {pa, mb,  0}}, {{mA, pB,  0}, {ma, pb,  0}}, {{mA, mB,  0} ,{ma, mb,  0}},
      {{pB,  0, pA}, {pb,  0, pa}}, {{mB,  0, pA}, {mb,  0, pa}}, {{pB,  0, mA}, {pb,  0, ma}}, {{mB,  0, mA}, {mb,  0, ma}}
    }};*/

    const/*expr*/ std::vector<double_vert> p = {{
      {{ 0, pA, pB}}, {{ 0, pA, mB}}, {{ 0, mA, pB}}, {{ 0, mA, mB}},
      {{pA, pB,  0}}, {{pA, mB,  0}}, {{mA, pB,  0}}, {{mA, mB,  0}},
      {{pB,  0, pA}}, {{mB,  0, pA}}, {{pB,  0, mA}}, {{mB,  0, mA}}
    }};

    /*std::vector<widget_object_instanced::vert> triangles = {{
      {p[0]}, {p[1]}, {p[2]}
    }};*/

    const/*expr*/ std::vector<std::uint32_t> indexes = {{
      0, 4, 6,  1, 4, 6,                     // top 2
      0, 4, 8,  1, 4,10,  0, 6, 9,  1, 6,11, // next 4

      4, 8,10,  5, 8,10,  6, 9,11,  7, 9,11, // 2 vertical diamonds

      0, 2, 8,  0, 2, 9,  1, 3,10,  1, 3,11, // 2 horizontal diamonds
      
      2, 5, 8,  3, 5,10,  2, 7, 9,  3, 7,11, // 4 connected to bottom 2
      2, 5, 7,  3, 5, 7                      // bottom 2 
    }};

    std::vector<widget_object_instanced::vert> triangles;
    
    triangles.reserve(5 * p.size());

    //for (const auto i : indexes)
    //  triangles.emplace_back(static_cast<widget_object_instanced::vert>(p[i]));

    for (std::uint32_t i = 0; i < indexes.size(); i+=3)
    {
      auto p0 = p[indexes[i]];
      auto p1 = p[indexes[i+1]];
      auto p2 = p[indexes[i+2]];

      // number of triangles in sphere is 20*split^2
      std::uint32_t split = 3;

      for (std::uint32_t i = 0; i < split; i++)
      {
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i  )      + p2*i     )/split));
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1) + p1 + p2*i     )/split));
        triangles.emplace_back(static_cast<widget_object_instanced::vert>(( p0*(split-i-1)      + p2*(i+1) )/split));
        
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

      /*triangles.emplace_back(static_cast<widget_object_instanced::vert>(p0));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p1)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p2)/2));

      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p2)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p1)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p1+p2)/2));

      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p1)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>(p1));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p1+p2)/2));

      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p0+p2)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>((p1+p2)/2));
      triangles.emplace_back(static_cast<widget_object_instanced::vert>(p2));*/

      //triangles.emplace_back(static_cast<widget_object_instanced::vert>(p0));
      //triangles.emplace_back(static_cast<widget_object_instanced::vert>(p1));
      //triangles.emplace_back(static_cast<widget_object_instanced::vert>(p2));
    }


    return triangles;
  }


  void create_test_instanced() noexcept
  {
    if (pdb_list_.empty() || widget_object_instanced_)
      return;

    widget_object_instanced_ = ui_event_destination::make_ui<widget_object_instanced>(ui_, coords_, widget_object_);

    auto v = generate_sphere(50.0);
    /*std::vector<widget_object_instanced::vert> v = {{
      {{ -50, -50, 0 }, { 0, 1, 0 }},
      {{ -50,  50, 0 }, { 0, 1, 0 }},
      {{  50, -50, 0 }, { 32767, 0, 0 }},
      {{   0,   0, 71}, { 0, 1, 0 }},
      {{ -50,  50, 0 }, { 0, 1, 0 }},
      {{  50, -50, 0 }, { 0, 1, 0 }}
    }};*/


    widget_object_instanced_->set_vertex(v, GL_TRIANGLES);

/*
    std::array<widget_object_instanced::inst, 4> i = {{
      {{    0, 0, 0 }, { 100 }, { 255,   0,   0, 255 }},
      {{ 1000, 0, 0 }, { 110 }, {   0, 255,   0, 255 }},
      {{ 2000, 0, 0 }, { 120 }, {   0,   0, 255, 255 }},
      {{ 3000, 0, 0 }, { 130 }, { 128, 128, 128, 255 }}
    }};

    widget_object_instanced_->set_instance(i);
*/

    widget_object_instanced_->set_scale_and_direction(widget_object_->get_scale(), widget_object_->get_direction());

    if (local_)
    {
      ui_event::directory_load_file(item_, pdb_list_[0], [this, self{shared_from_this()}] (std::string&& pdb)
      {
        worker_->call("visualise_atoms", pdb, [this, self{shared_from_this()}] (std::span<char> d)
        {
          emscripten_webgl_make_context_current(ui_->ctx_);

          std::span<widget_object_instanced::inst> s(reinterpret_cast<widget_object_instanced::inst*>(d.data()),
                                                    d.size() / sizeof(widget_object_instanced::inst));

          log_debug(FMT_COMPILE("received: {} atoms"), s.size());

          widget_object_instanced_->set_instance(s);
        });
      });
    }
    else // remote
    {
      std::string u = url_ + item_ + "/" + pdb_list_[0];

      worker_->call("visualise_atoms_url", u, [this, self{shared_from_this()}] (std::span<char> d)
      {
        emscripten_webgl_make_context_current(ui_->ctx_);

        std::span<widget_object_instanced::inst> s(reinterpret_cast<widget_object_instanced::inst*>(d.data()),
                                                   d.size() / sizeof(widget_object_instanced::inst));

        log_debug(FMT_COMPILE("received: {} atoms"), s.size());

        widget_object_instanced_->set_instance(s);
      });
    }
  }
  

  std::shared_ptr<plate::widget_text>         widget_title_;
  std::shared_ptr<plate::widget_text>         widget_loading_;

  std::shared_ptr<plate::widget_object<vert, cols>> widget_object_;
  std::shared_ptr<plate::widget_object_instanced>   widget_object_instanced_;

  std::weak_ptr<widget_control<widget_main>> widget_control_;

  bool playing_{true};

  int display_style_{0};

  struct frame
  {
    buffer<vert> vertex;
    buffer<vert> vertex_strip;
  };

  std::vector<frame> store_; // each frame's vertex and vertex_strip buffer is stored here

  std::queue<int> to_load_; // the order in which to request frames
  int loaded_count_{0};

  int current_entry_{-1};

  int frame_direction_{1};
  float current_time_{0};
  float frame_time_{1.0/30.0}; // 30 fps

  bool local_{false}; // whether we are loading local files or remote

  std::string description_{}; // description of animation

  std::string item_;                  // item to load (either a protein code if remote, or a local directory name)
  std::vector<std::string> pdb_list_; // list of pdb files to display

  std::string url_;    // location of remote pdb files

  std::string script_; // the molauto script in use

  buffer<cols> shared_vertex_colors_;        // for keeping the colors seperate from the vertices
  buffer<cols> shared_vertex_strip_colors_;

  std::unique_ptr<worker> worker_; // a multi-threaded worker for performaing background tasks

  constexpr static int control_height_ = 55;

}; // class widget_main


} // namespace pdbmovie
