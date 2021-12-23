#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "system/webgl/worker.hpp"

#include "widgets/widget_rounded_box.hpp"
#include "widgets/widget_object.hpp"
#include "widgets/widget_text.hpp"


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

    //else
    //  set_load_button();

    set_load_button();
  }


  void display() noexcept
  {
    if (current_entry_ < 0)
      return;

    current_time_ += ui_->frame_diff_;

    if (current_time_ >= frame_time_)
      set_next_frame();

    ui_->do_draw(); // keep drawing
  }


  void reshape() noexcept
  { 
    coords_ = { { 0, 0 }, { ui_->pixel_width_, ui_->pixel_height_ } };

    set_title();
    set_loading();

    if (widget_select_)
      set_load_button();

    if (widget_object_)
      widget_object_->set_geometry(coords_);
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

    set_title();
    set_loading();

    // load in movie plan file

    auto h = async::request(url_ + item_ + "/movie.plan", "GET", "", [this] (std::uint32_t handle, plate::data_store&& d)
    {
      emscripten_webgl_make_context_current(ui_->ctx_);

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
    set_loading();

    auto old_worker = std::move(worker_); // delete the old one (if any) after we have created a new one

    worker_ = std::make_unique<worker>();

    worker_->set_path("/animol-viewer/version/1/decoder_worker.js");

    if (local_)
    {
      // load in file and then request the worker to generate the script

      ui_event::directory_load_file(item_, pdb_list_[current_entry_], [this, self{shared_from_this()}] (std::string&& pdb)
      {
        if (!worker_)
          return;

        worker_->call("script_with", pdb, [this, self{shared_from_this()}, pdb] (std::span<char> d)
        {
          script_ = std::string(d.data(), d.size());

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
        });
      });
    }
    else
    {
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


  void get_first_frame() noexcept
  {
    // script has been generated, so run the decoder with the main frame,
    //
    // once the main frame has been generated, upload the script to all the workers and start processing the subsequent frames

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

    for (int i = 0; i < worker::get_num_workers(); ++i)
      process_next();

    widget_object_ = ui_event_destination::make_ui<widget_object<vert, cols>>(ui_, coords_,
                                          Prop::Display | Prop::Input | Prop::Swipe | Prop::Priority, shared_from_this());

    widget_object_->set_model(&store_[current_entry_].vertex,       &shared_vertex_colors_);
    widget_object_->add_model(&store_[current_entry_].vertex_strip, &shared_vertex_strip_colors_);

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
      widget_title_ = ui_event_destination::make_ui<widget_text>(ui_, c, Prop::Display, shared_from_this(), title,
                                                        gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);
  }


  void set_loading() noexcept
  {
    std::string loading;

    if (current_entry_ == -1) // haven't loaded plan/directory listing
      loading = "loading: plan";
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

    if (widget_loading_)
    {
      widget_loading_->show();
      widget_loading_->set_geometry(coords_);
      widget_loading_->set_text(loading);
    }
    else
      widget_loading_ = ui_event_destination::make_ui<widget_text>(ui_, coords_, Prop::Display, shared_from_this(), loading,
                                  gpu::align::BOTTOM | gpu::align::OUTSIDE, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);
  }


  void set_load_button()
  {
    // button for loading local files

    if (!(widget_select_ || ui_event::have_file_system_access_support()))
      return;

    gpu::int_box c;

    c.p1.x = my_width() - (150 * ui_->pixel_ratio_);
    c.p1.y = my_height() - (70 * ui_->pixel_ratio_);

    c.p2.x = my_width()  - (10 * ui_->pixel_ratio_);
    c.p2.y = my_height() - (10 * ui_->pixel_ratio_);

    if (widget_select_)
    {
      widget_box_->set_geometry(c);
      widget_select_->set_geometry(c);
    }
    else
    {
      widget_box_ = ui_event_destination::make_ui<widget_rounded_box>(ui_, c, Prop::Display, shared_from_this(),
                                                                             ui_->select_color_, 15 * ui_->pixel_ratio_);

      widget_select_ = ui_event_destination::make_ui<widget_text>(ui_, c, Prop::Display | Prop::Input, widget_box_, "Open local",
                                                       gpu::align::CENTER, ui_->txt_color_, gpu::rotation{0.0f,0.0f,0.0f}, 0.8f);

      widget_select_->set_click_cb([this]
      {
        ui_event::directory_show_picker(".*\\.pdb$", [this] (std::string dir_name, std::vector<std::string>& files) -> void
        {
          if (files.size() > 0)
            start_local(dir_name, files);
        });
      });
    }
  }
  

  std::shared_ptr<plate::widget_text>         widget_title_;
  std::shared_ptr<plate::widget_text>         widget_loading_;

  std::shared_ptr<plate::widget_rounded_box>  widget_box_;
  std::shared_ptr<plate::widget_text>         widget_select_;

  std::shared_ptr<plate::widget_object<vert, cols>> widget_object_;


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

}; // class widget_main


} // namespace pdbmovie
