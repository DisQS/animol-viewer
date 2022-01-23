#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"

#include "widgets/widget_spheres.hpp"
#include "widgets/anim_alpha.hpp"

#include "widget_layer.hpp"


namespace pdbmovie {

template<class M>
class widget_layer_spacefill : public widget_layer<M>
{

public:


  ~widget_layer_spacefill()
  {
    clear();
  }


  void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords, 
            const std::shared_ptr<plate::ui_event_destination>& parent, M* main) noexcept override
  {
    plate::ui_event_destination::init(_ui, coords, plate::ui_event_destination::Prop::Display, parent);

    main_ = main;

    start();
  }


  int get_loaded_count() const noexcept override
  {
    return loaded_count_;
  }


  inline bool has_frame(int frame_id) noexcept override
  {
    return store_[frame_id].ibuf.is_ready();
  }


  bool set_frame(int frame_id) noexcept override
  {
    if (!has_frame(frame_id))
      return false;

    widget_spheres_->set_instance_ptr(&(store_[frame_id].ibuf), &(store_[frame_id].vao));

    return true;
  }


  std::string_view name() const noexcept override
  {
    return "layer_spacefill";
  }


private:


  void start() noexcept
  {
    clear();

    if (main_->get_total_frames() <= 0)
      return;

    widget_spheres_ = plate::ui_event_destination::make_ui<plate::widget_spheres>(this->ui_, this->coords_,
                                                              this->shared_from_this(), main_->get_shared_ubuf());

    store_.resize(main_->get_total_frames());

    // setup the load order- from current_frame

    for (int i = main_->get_current_frame(); i < main_->get_total_frames(); ++i)
      to_load_.push(i);

    for (int i = main_->get_current_frame() - 1; i >= 0; --i)
      to_load_.push(i);

    for (int i = 0; i < worker::get_max_workers(); ++i)
      process_next();
  }


  void process_next() noexcept
  {
    if (to_load_.empty()) // nothing left to ask for
      return;

    auto to_load = to_load_.front();
    to_load_.pop();

    auto per_frame_files = main_->get_frame_files();

    std::string u;

    if (main_->is_remote())
      u = main_->get_url() + main_->get_item() + "/" + per_frame_files[to_load];
    else
      u = per_frame_files[to_load];

    main_->worker_->call("visualise_atoms_url", u, [this, wself{this->weak_from_this()}, entry = to_load] (std::span<char> d)
    {
      if (auto w = wself.lock())
      {
        emscripten_webgl_make_context_current(this->ui_->ctx_);

        std::span<plate::widget_spheres::inst> s(reinterpret_cast<plate::widget_spheres::inst*>(d.data()), d.size() /
                                                                                         sizeof(plate::widget_spheres::inst));

        process_frame(entry, s);
      }
    });
  }


  void process_frame(int entry, std::span<plate::widget_spheres::inst> s) noexcept
  {
    if (s.empty())
    {
      main_->set_error("Failed to process");
      return;
    }

    if (!main_->scale_has_been_set())
    {
      int max = 0;

      for (auto& s_entry : s)
      { 
        if (abs(s_entry.offset[0]) > max) max = abs(s_entry.offset[0]);
        if (abs(s_entry.offset[1]) > max) max = abs(s_entry.offset[1]);
        if (abs(s_entry.offset[2]) > max) max = abs(s_entry.offset[2]);
      }

      log_debug(FMT_COMPILE("received: {} atoms max width: {}"), s.size(), max);

      if (max)
        main_->set_scale((std::min(this->my_width(), this->my_height())/2.0) * (32'768.0 / max) * 0.8);
    }

    store_[entry].ibuf.upload(s.data(), s.size());

    if (main_->get_current_frame() == entry) // we've caught up with main frame position
      widget_spheres_->set_instance_ptr(&(store_[entry].ibuf), &(store_[entry].vao));
    
    ++loaded_count_;

    if (loaded_count_ == main_->get_total_frames())
      log_debug(FMT_COMPILE("all loaded: {} atoms: {}"), loaded_count_, s.size());

    main_->update_loading();

    process_next();
  }


  void clear()
  {
    std::queue<int> x;
    x.swap(to_load_);

    loaded_count_ = 0;

    if (widget_spheres_)
    {
      widget_spheres_->disconnect_from_parent();
      widget_spheres_.reset();
    }

    for (auto& s : store_)
      if (s.vao)
        glDeleteVertexArrays(1, &s.vao);

    store_.clear();
  }


  std::shared_ptr<plate::widget_spheres> widget_spheres_;

  struct frame
  {
    plate::buffer<plate::widget_spheres::inst> ibuf;
    std::uint32_t                              vao{0};
  };

  std::vector<frame> store_; // each frame's vertex and vertex_strip buffer is stored here

  std::queue<int> to_load_; // the order in which to request frames
  int loaded_count_{0};

  M* main_{nullptr}; // tha main widget

}; // class widget_layer_spacefill


} // namespace pdbmovie
