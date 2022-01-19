#pragma once

#include <array>
#include <type_traits>

#include "plate.hpp"


namespace pdbmovie {


template<class M>
class widget_layer : public plate::ui_event_destination
{

public:


  virtual void init(const std::shared_ptr<plate::state>& _ui, plate::gpu::int_box& coords, 
            const std::shared_ptr<plate::ui_event_destination>& parent, M* main) noexcept;


  virtual int get_loaded_count() const noexcept;


  virtual bool has_frame(int frame_id) noexcept;

  virtual bool set_frame(int frame_id) noexcept;


}; // class widget_layer


} // namespace pdbmovie
