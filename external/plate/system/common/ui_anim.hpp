#pragma once

#include <memory>

#include "ui_event_destination.hpp"
#include "../log.hpp"


namespace plate {


class ui_anim : public ui_event_destination
{

public:


  enum Style { SoftSoft, SoftHard, HardSoft, HoldSoftSoft, Linear };
  enum Dir   { Forward, Reverse };

  struct targets
  {
    float* v;
    float  from;
    float  to;
  };


  void init(const std::shared_ptr<state>& _ui, std::uint32_t prop,
                                      const std::shared_ptr<ui_event_destination>& parent) noexcept
  {
    ui_event_destination::init(_ui, prop, parent);

    std::shared_ptr<ui_anim> self = static_pointer_cast<ui_anim>(shared_from_this());

    std::weak_ptr<ui_anim> wself = self;

    ui_->anims_.push_back(wself);
  }


  ~ui_anim()
  {
    auto wself = weak_from_this();

    for (auto it = ui_->anims_.begin(); it != ui_->anims_.end(); ++it)
      if (!it->owner_before(wself) && !wself.owner_before(*it))
      {
        ui_->anims_.erase(it);
        return;
      }
    
    log_error("unable to find wself in anims");
  }


  void init_targets()
  {
    for (auto& t : targets_)
      *t.v = t.from;
    
    on_change();
  }

  void set_end_cb(std::function< void ()> cb) noexcept
  {
    end_cb_ = cb;
  }
  

  void process(float time_delta, bool frame_time_correct) noexcept
	{
    if (!frame_time_correct) // wait until frame_time is correct
    {
      on_change(); // push out the initial values
      return;
    }

    auto frac = get_fractional(time_delta);

    for (auto& target : targets_)
      *target.v = target.from + (frac * (target.to - target.from));
    
    on_change();

    if (frac >= 1.0)
    {
      on_end();
      return;
    }

    //on_change();
  }


  void on_end() noexcept
  {
    auto self(shared_from_this());

    if (end_cb_)
      end_cb_();

    if (auto p = parent_.lock())
    {
      p->clear_anim();
    
      parent_.reset();
    }
  }


  virtual void on_change() noexcept
  {
  }


  virtual std::string_view name() const noexcept = 0;


protected:


  float get_fractional(const float& time_delta) noexcept
  {
    run_time_ += time_delta;

    auto f = run_time_ / total_time_;

    if (f >= 1.0)
      return 1.0;

    switch (style_)
    {
      case Style::SoftSoft:

        // slow, fast, slow based on sin(-PI/2) -> sin(PI/2)
  
        if (f < 0.5)
          return (float)(sin(f * M_PI - (M_PI/2.0)) + 1) / 2.0;
        else
          return 0.5 + (float)sin(f * M_PI - (M_PI/2.0)) / 2.0;

      case Style::HoldSoftSoft:

        // hold, then as SoftSoft

        if (f < 0.33) 
          return 0;
        else
        {
          f = (run_time_ - (0.33 * total_time_)) / (0.67 * total_time_); // aligned to soft soft
          if (f < 0.5)
            return (float)(sin(f * M_PI - (M_PI/2.0)) + 1) / 2.0;
          else
            return 0.5 + (float)sin(f * M_PI - (M_PI/2.0)) / 2.0;
        }
      
      case Style::Linear:

        return f;

      default:
        return f;
      }
  }


  std::vector<targets>    targets_;
  std::uint32_t           style_;
  float                   run_time_{0};
  float                   total_time_;

  std::function< void ()> end_cb_;

}; // ui_anim


void process_ui_anims(const std::vector<std::weak_ptr<ui_anim>>& anims) noexcept
{
  for (auto& a : anims)
    if (auto p = a.lock())
      p->process(p->ui_->frame_diff_, p->ui_->frame_time_correct_);
}


} // namespace plate
