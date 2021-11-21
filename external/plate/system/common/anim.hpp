#pragma once

#include <memory>

#include "../log.hpp"


namespace plate {

class anim
{

public:


  enum Style { SoftSoft, SoftHard, HardSoft, HoldSoftSoft };


  struct targets
  {
    int* v;
    int from;
    int to;
  };

  int add(std::vector<targets>&& t, int style, float total_time, std::function<void ()> on_change_cb = nullptr,
                                                                  std::function<void ()> at_end_cb = nullptr)
  {
    auto r = ++id_;
    anims_.emplace_back(entry{r, std::move(t), style, 0, total_time,
                                    std::move(on_change_cb), std::move(at_end_cb)});

    return r;
  };


  int add(std::vector<targets>& t, int style, float total_time, std::function<void ()> on_change_cb = nullptr,
                                                                  std::function<void ()> at_end_cb = nullptr)
  {
    auto r = ++id_;
    anims_.emplace_back(entry{r, t, style, 0, total_time, std::move(on_change_cb), std::move(at_end_cb)});

    return r;
  };


  inline void erase(int id) noexcept
  {
    for (auto it = anims_.begin(); it != anims_.end(); ++it)
      if (it->id == id)
      {
        anims_.erase(it);
        return;
      }
  };


  bool process(float time_delta, bool frame_time_correct) noexcept
	{
    if (!frame_time_correct && !anims_.empty()) // wait until frame_time is correct
      return true;

    bool something_to_do = false;

    for (auto it = anims_.begin(); it != anims_.end();)
    {
      auto frac = get_fractional(*it, time_delta);

      for (auto& target : it->targets)
        *target.v = target.from + (frac * (target.to - target.from));

      if (it->on_change_cb)
        it->on_change_cb();

      if (frac >= 1)
      {
        if (it->at_end_cb)
          it->at_end_cb();

        it = anims_.erase(it);
      }
      else
      {
        ++it;
        something_to_do = true;
      }
    }

    return something_to_do;
  }


private:


  struct entry
  {
    int id;
    std::vector<targets> targets;
    int style;
    float run_time;
    float total_time;
    
    std::function<void ()> on_change_cb;
    std::function<void ()> at_end_cb;
  };


  float get_fractional(entry& e, const float& time_delta) noexcept
  {
    e.run_time += time_delta;

    auto f = e.run_time / e.total_time;

    if (f >= 1.0)
      return 1.0;

    switch (e.style)
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
          f = (e.run_time - (0.33 * e.total_time)) / (0.67 * e.total_time); // aligned to soft soft
          if (f < 0.5)
            return (float)(sin(f * M_PI - (M_PI/2.0)) + 1) / 2.0;
          else
            return 0.5 + (float)sin(f * M_PI - (M_PI/2.0)) / 2.0;
        }
      }

    return f;
  };


  std::vector<entry> anims_;

  int id_{0};

}; // anim

} // namespace plate
