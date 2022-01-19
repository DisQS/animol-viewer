#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "widgets/widget_object.hpp"
#include "widgets/anim_alpha.hpp"

#include "widget_layer.hpp"


namespace pdbmovie {

template<class M>
class widget_layer_cartoon : public widget_layer<M>
{

public:

  struct compact_header
  {
    int num_triangles;
    int num_strips;
  };


  struct vert_with_color
  {
    std::array<short,3>        position;
    std::array<short,3>        normal;
    std::array<std::uint8_t,4> color;
  };


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords, 
            const std::shared_ptr<plate::ui_event_destination>& parent, M* main) noexcept override
  {
    plate::ui_event_destination::init(_ui, coords, plate::ui_event_destination::Prop::Display, parent);

    main_   = main;

    start();
  }


  int get_loaded_count() const noexcept override
  {
    return loaded_count_;
  }


  inline bool has_frame(int frame_id) noexcept override
  {
    return store_[frame_id].vertex.is_ready() || store_[frame_id].vertex_strip.is_ready();
  }


  bool set_frame(int frame_id) noexcept override
  {
    if (!has_frame(frame_id))
      return false;

    widget_object_->set_model(&(store_[frame_id].vertex));
    widget_object_->add_model(&(store_[frame_id].vertex_strip));

    return true;
  }


  std::string_view name() const noexcept override
  {
    return "layer_cartoon";
  }


private:


  void start() noexcept
  {
    clear();

    if (main_->get_total_frames() <= 0)
      return;

    store_.resize(main_->get_total_frames());

    // setup the load order- from initial+1 up, and then from initial-1 down..

    for (int i = main_->get_master_frame_id() + 1; i < main_->get_total_frames(); ++i)
      to_load_.push(i);

    for (int i = main_->get_master_frame_id() - 1; i >= 0; --i)
      to_load_.push(i);

    generate_script();
  }


  void generate_script() noexcept
  {
    if (main_->is_remote())
    {
      // request the worker to download the pdb file and generate the script

      auto per_frame_files = main_->get_frame_files();

      std::string u = main_->get_url() + main_->get_item() + "/" + per_frame_files[main_->get_master_frame_id()];

      main_->worker_->call("script", u, [this, wself{this->weak_from_this()} ] (std::span<char> d)
      {
        if (auto w = wself.lock())
        {
          script_ = std::string(d.data(), d.size());

          get_first_frame();
        }
      });
    }
    else // is local
    {
      // load in file and then request the worker to generate the script

      auto per_frame_files = main_->get_frame_files();

      log_debug(FMT_COMPILE("about to load local: {} {}"), main_->get_item(), per_frame_files[main_->get_master_frame_id()]);

      plate::ui_event::directory_load_file(main_->get_item(), per_frame_files[main_->get_master_frame_id()],
                                                          [this, wself{this->weak_from_this()}] (std::string&& pdb)
      {
        log_debug("loaded file");
        if (auto w = wself.lock())
        {
          main_->worker_->call("script_with", pdb, [this, wself, pdb] (std::span<char> d)
          {
            log_debug("scripted file");
            if (auto w = wself.lock())
            {
              script_ = std::string(d.data(), d.size());

              get_first_frame(pdb);
            }
          });
        }
      });
    }

    main_->set_loading();
  }


  void get_first_frame(std::string_view pdb = "") noexcept
  {
    // script has been generated, so run the decoder with the main frame,
    //
    // once the main frame has been generated, upload the script to all the workers and start processing the subsequent frames

    if (main_->is_remote())
    {
      auto per_frame_files = main_->get_frame_files();

      std::string u = main_->get_url() +main_->get_item() + "/" + per_frame_files[main_->get_master_frame_id()];

      std::vector<char> data_to_send(4);

      std::uint32_t script_size = script_.size();

      std::memcpy(data_to_send.data(), &script_size, 4);

      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),       u.begin(),       u.end());

      main_->worker_->call("decode_url_color_interleaved", data_to_send,
        [this, wself{this->weak_from_this()} ] (std::span<char> d)
      {
        if (auto w = wself.lock())
        {
          emscripten_webgl_make_context_current(this->ui_->ctx_);

          process_main_frame(d);
        }
      });
    }
    else // local
    {
      std::vector<char> data_to_send(4);
        
      std::uint32_t script_size = script_.size();
        
      std::memcpy(data_to_send.data(), &script_size, 4);
        
      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),     pdb.begin(),     pdb.end());
        
      main_->worker_->call("decode_contents_color_interleaved", data_to_send,
        [this, wself{this->weak_from_this()}] (std::span<char> d)
      {
        if (auto w = wself.lock())
        {
          emscripten_webgl_make_context_current(this->ui_->ctx_);

          process_main_frame(d);
        }
      });
    }
  }


  void process_next() noexcept
  {
    if (to_load_.empty()) // nothing left to ask for
      return;

    auto to_load = to_load_.front();
    to_load_.pop();

    auto per_frame_files = main_->get_frame_files();

    if (main_->is_remote())
    {
      std::string u = main_->get_url() + main_->get_item() + "/" + per_frame_files[to_load];

      std::vector<char> data_to_send(4);

      std::uint32_t script_size = script_.size();

      std::memcpy(data_to_send.data(), &script_size, 4);

      data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
      data_to_send.insert(data_to_send.end(),       u.begin(),       u.end());

      main_->worker_->call("decode_url_color_interleaved", data_to_send,
                                      [this, wself{this->weak_from_this()}, entry = to_load] (std::span<char> d)
      {
        if (auto w = wself.lock())
        {
          emscripten_webgl_make_context_current(this->ui_->ctx_);
          process_decoded(entry, d);
        }
      });
    }
    else // local
    {
      plate::ui_event::directory_load_file(main_->get_item(), per_frame_files[to_load],
                                      [this, wself{this->weak_from_this()}, entry = to_load] (std::string&& pdb)
      {
        if (auto w = wself.lock())
        {
          std::vector<char> data_to_send(4);

          std::uint32_t script_size = script_.size();

          std::memcpy(data_to_send.data(), &script_size, 4);

          data_to_send.insert(data_to_send.end(), script_.begin(), script_.end());
          data_to_send.insert(data_to_send.end(),     pdb.begin(),     pdb.end());

          main_->worker_->call("decode_contents_color_interleaved", data_to_send, [this, wself, entry] (std::span<char> d)
          {
            if (auto w = wself.lock())
            {
              emscripten_webgl_make_context_current(this->ui_->ctx_);
              process_decoded(entry, d);
            }
          });
        }
      });
    }
  }


  void process_decoded(int entry, std::span<char> d) noexcept
  {
    compact_header* h = new (d.data()) compact_header;

    auto p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

    store_[entry].vertex.upload(p, h->num_triangles, GL_TRIANGLES);
    p += h->num_triangles * sizeof(vert_with_color);

    store_[entry].vertex_strip.upload(p, h->num_strips, GL_TRIANGLE_STRIP);

    if (main_->get_current_frame() == entry) // we've caught up with main frame position
    {
      widget_object_->set_model(&(store_[entry].vertex));
      widget_object_->add_model(&(store_[entry].vertex_strip));
    }

    process_frame();
    process_next();
  }


  void process_main_frame(std::span<char> d) noexcept
  {
    // main frame has been converted to geometry

    compact_header* h = new (d.data()) compact_header;

    auto p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

    log_debug(FMT_COMPILE("main frame triangles: {} strips: {}"), h->num_triangles, h->num_strips);

    if (h->num_triangles == 0 && h->num_strips == 0)
    {
      main_->set_error("Failed to process");
      return;
    }

    // upload the vertices

    store_[main_->get_master_frame_id()].vertex.upload(p, h->num_triangles, GL_TRIANGLES);
    p += h->num_triangles * sizeof(vert_with_color);
        
    store_[main_->get_master_frame_id()].vertex_strip.upload(p, h->num_strips, GL_TRIANGLE_STRIP);
    p += h->num_strips * sizeof(vert_with_color);

    // start number_of_worker_threads processing

    for (int i = 0; i < worker::get_max_workers(); ++i)
      process_next();

    widget_object_ = plate::ui_event_destination::make_ui<plate::widget_object<vert_with_color>>(this->ui_, this->coords_,
                                      plate::ui_event_destination::Prop::Display, this->shared_from_this(), main_->get_shared_ubuf());

    if (main_->get_current_frame() ==  main_->get_master_frame_id())
    {
      widget_object_->set_model(&store_[main_->get_master_frame_id()].vertex);
      widget_object_->add_model(&store_[main_->get_master_frame_id()].vertex_strip);
    }

    auto fade_in = plate::ui_event_destination::make_anim<plate::anim_alpha>(this->ui_, widget_object_, plate::ui_anim::Dir::Forward, 0.3f);

    // loop through data to find protein width

    if (main_->get_scale() == 1.0)
    {
      p = reinterpret_cast<std::byte*>(d.data()) + sizeof(compact_header);

      int max = 0;

      auto v = reinterpret_cast<vert_with_color*>(p);

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

      if (max)
        main_->set_scale((std::min(this->my_width(), this->my_height())/2.0) * (32'768.0 / max) * 0.8);
    }

    process_frame();
  }


  void process_frame() noexcept
  {
    ++loaded_count_;

    if (loaded_count_ == main_->get_total_frames())
    {
      log_debug(FMT_COMPILE("all loaded: {}"), loaded_count_);
    }

    main_->set_loading();
  }


  void clear()
  {
    store_.clear();

    std::queue<int> x;
    x.swap(to_load_);

    loaded_count_ = 0;

    script_.clear();

    if (widget_object_)
    {
      widget_object_->disconnect_from_parent();
      widget_object_.reset();
    }
  }


  std::shared_ptr<plate::widget_object<vert_with_color>> widget_object_;

  struct frame
  {
    plate::buffer<vert_with_color> vertex;
    plate::buffer<vert_with_color> vertex_strip;
  };

  std::vector<frame> store_; // each frame's vertex and vertex_strip buffer is stored here

  std::queue<int> to_load_; // the order in which to request frames
  int loaded_count_{0};

  std::string script_; // the molauto script in use

  M* main_{nullptr}; // tha main widget

}; // class widget_layer_cartoon


} // namespace pdbmovie
