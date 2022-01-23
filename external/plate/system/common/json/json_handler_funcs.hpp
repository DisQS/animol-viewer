#pragma once

#include "json_handler.hpp"

/*
    json parse handler with a callback per type.
*/


namespace plate {

  class json_handler_funcs : public json_handler
  {
    public:

    struct lookup
    {
      std::string_view                        name;
      bool                                    must;
      std::function< bool (std::string_view)> f;

      bool operator== (const lookup& other) const noexcept
      {
        return name == other.name;
      }

      bool operator== (const std::string_view& other) const noexcept
      {
        return name == other;
      }
    };


    json_handler_funcs(std::span<lookup> l, bool fail_not_exists = false) :
      fail_not_exists_(fail_not_exists),
      skip_(this),
      lookup_(l)
    {}


    bool apply(std::string_view s) noexcept
    {
      if (const auto found = std::find(lookup_.begin(), lookup_.end(), active_); found != lookup_.end())
      {
        auto ok = found->f(s);

        if (ok)
        {
          mask_ |= 1 << std::distance(lookup_.begin(), found);
          return true;
        }
      }

      return !fail_not_exists_;
    }


    bool rhs(const std::string_view& s) noexcept
    {
      return apply(s);
    }


    bool rhs(const bool& s) noexcept
    {
      return apply(s ? "true" : "false");
    }


    json_handler* start_object(const char* p) noexcept
    {
      if (depth_ == 0) // depth 1 is normally where the object starts
      {
        ++depth_;
        return this;
      }
      // handle as a string_view so skip

      object_start_ = p;
      object_active_ = active_;

      return &skip_;
    }


    json_handler* end_object(const char* p) noexcept
    {
      if (object_start_)
      {
        active_ = object_active_;
        apply(std::string_view{object_start_, static_cast<std::size_t>(p - object_start_)});

        object_start_ = nullptr;
        return this;
      }

      --depth_;

      if (depth_ < 0)
        return nullptr;

      // check to make sure all must entries have been included
      int i = 0;
      for (auto& e : lookup_)
      {
        if (e.must && !((1ull << i) & mask_))
          return nullptr;

        ++i;
      }

      return this;
    }


    json_handler* start_array(const char* p) noexcept
    {
      // handle as a string_view so skip

      object_start_ = p;
      object_active_ = active_;

      return &skip_;
    }


    json_handler* end_array(const char* p) noexcept
    {
      if (object_start_)
      {
        active_ = object_active_;
        apply(std::string_view{object_start_, static_cast<std::size_t>(p - object_start_)});

        object_start_ = nullptr;
        return this;
      }

      return this;
    }


    bool fail_not_exists_;        // if the json contains something we do not have, should we fail?

    json_handler_skip skip_;
    const char* object_start_{nullptr};
    std::string_view object_active_;

    std::span<lookup> lookup_;

  }; // class json_handler_funcs


} // namespace plate
