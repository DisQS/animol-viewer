#pragma once

#include "json_handler.hpp"


namespace plate {


  template<class UT>
  class json_handler_vector : public json_handler
  {
    public:


    json_handler_vector(bool fail_not_exists = false) :
      fail_not_exists_(fail_not_exists),
      skip_(this)
    {
    }


    bool rhs(const std::string_view& s) noexcept
    {
      return rhs_auto(data_entry, s, fail_not_exists_);
    }


    bool rhs(const float64& d) noexcept
    {
      return rhs_auto(data_entry, d, fail_not_exists_);
    }


    bool rhs(const bool& b) noexcept
    {
      return rhs_auto(data_entry, b, fail_not_exists_);
    }

    bool has(std::string_view s) const noexcept
    {
      if (const auto found = std::find(UT::lookup_.begin(), UT::lookup_.end(), s); found != UT::lookup_.end())
        return (mask_ & (1ull << std::distance(UT::lookup_.begin(), found))) != 0;

      return false;
    }


    void set_has(std::string_view s) noexcept
    {
      if (const auto found = std::find(UT::lookup_.begin(), UT::lookup_.end(), s); found != UT::lookup_.end())
        mask_ |= 1ull << std::distance(UT::lookup_.begin(), found);
    }


    json_handler* start_object(const char* p) noexcept
    {
      if (in_object_)
      {
        // handle as a string_view

        object_start = p;
        object_active = active_;

        return &skip_;
      }

      in_object_ = true;
      mask_ = 0;

      return this;
    }


    json_handler* end_object(const char* p) noexcept
    {
      if (object_start)
      {
        active_ = object_active;
        rhs_auto(data_entry, std::string_view{object_start, p - object_start}, fail_not_exists_);

        object_start = nullptr;

        return this;
      }

      if (in_object_)
      {
        data.push_back(data_entry);
        in_object_ = false;

        if (UT::must_ != 0 || UT::may_ != 0)
        {
          if ((UT::must_ & mask_) != UT::must_)
            return nullptr;

          if ((UT::must_ | UT::may_ | mask_) != (UT::must_ | UT::may_))
            return nullptr;
        }

        return this;
      }

      return nullptr;
    }


    json_handler* start_array(const char* p) noexcept
    {
      if (!in_array_)
      {
        in_array_ = true;

        return this;
      }

      // handle as a string_view so skip

      object_start = p;
      object_active = active_;

      return &skip_;
    }


    json_handler* end_array(const char* p) noexcept
    {
      if (object_start)
      {
        active_ = object_active;
        rhs_auto(data_entry, std::string_view{object_start, p - object_start}, fail_not_exists_);

        object_start = nullptr;
        return this;
      }

      return this;
    }


    std::vector<UT> data;
    UT              data_entry;

    bool        fail_not_exists_;   // if the json contains something we do not have, should we fail?

    bool        in_array_{false};
    bool        in_object_{false};

    json_handler_skip skip_;
    const char* object_start{nullptr};
    std::string_view object_active;
  };


} // namespace plate
