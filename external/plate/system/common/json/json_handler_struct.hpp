#pragma once

#include "json_converters.hpp"

/*
    a json handler which decodes into a struct
*/


namespace plate {

  template<class UT>
  class json_handler_struct : public json_handler
  {
    public:


    json_handler_struct(bool fail_not_exists = false) :
      fail_not_exists_(fail_not_exists),
      skip_(this)
    {}


    bool rhs(const std::string_view& s) noexcept
    {
      return rhs_auto(data, s, fail_not_exists_);
    }


    bool rhs(const float64& d) noexcept
    {
      return rhs_auto(data, d, fail_not_exists_);
    }


    bool rhs(const bool& b) noexcept
    {
      return rhs_auto(data, b, fail_not_exists_);
    }


    constexpr uint64 mask_of(const std::string_view& s) const noexcept
    {
      return mask_of_entry(s, UT::lookup_);
    }


    bool has(const uint64& m) const noexcept
    {
      return mask_ & m;
    }


    void set_has(const uint64& m) noexcept
    {
      mask_ |= m;
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
        rhs_auto(data, std::string_view{object_start_, p - object_start_}, fail_not_exists_);

        object_start_ = nullptr;
        return this;
      }

      --depth_;

      if (depth_ < 0)
        return nullptr;

      if (UT::must_ != 0 || UT::may_ != 0)
      {
        if ((UT::must_ & mask_) != UT::must_)
          return nullptr;

        if ((UT::must_ | UT::may_ | mask_) != (UT::must_ | UT::may_))
          return nullptr;
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
        rhs_auto(data, std::string_view{object_start_, p - object_start_}, fail_not_exists_);

        object_start_ = nullptr;
        return this;
      }

      return this;
    }


    // also parse http_request get queries as provided by http_request

    bool parse_http_query(const std::vector<std::pair<std::string_view, std::string_view>>& query) noexcept
    {
      for (auto& q : query)
      {
        active_ = q.first;
        if (q.second != "" && !rhs(q.second))
          return false;
      }

      if (UT::must_ != 0 || UT::may_ != 0)
      {
        if ((UT::must_ & mask_) != UT::must_)
          return false;

        if ((UT::must_ | UT::may_ | mask_) != (UT::must_ | UT::may_))
          return false;
      }

      return true;
    }


    // for generating json output

    template<std::size_t I>
    std::string to_json_pair() const noexcept
    {
      using l_type = std::remove_const_t<std::remove_reference_t<pfr::tuple_element_t<I, UT>>>;

      if constexpr (std::is_arithmetic_v<l_type>)
        return fmt::format(FMT_COMPILE("\"{}\":{}"), UT::lookup_[I], pfr::get<I>(data));
      else
      {
        if constexpr (std::is_same_v<l_type, std::span<const std::byte>>)
          return fmt::format(FMT_COMPILE("\"{}\":\"binary_span\""), UT::lookup_[I]);
        else
          return fmt::format(FMT_COMPILE("\"{}\":\"{}\""), UT::lookup_[I], pfr::get<I>(data));
      }
    }


    template<std::size_t J>
    std::string to_json_e(const bool& all) const noexcept
    {
      if constexpr (UT::lookup_.size() > J + 1)
      {
        if (all || (mask_ & (1ull << J)))
          return to_json_pair<J>() + ", " + to_json_e<J+1>(all);
        else
          return to_json_e<J+1>(all);
      }
      else
      {
        if (all || (mask_ & (1ull << J)))
          return to_json_pair<J>() + ", ";
        else
          return "";
      }
    }

    // print out as json, all => all fields, false => just fields supplied

    std::string to_json(bool all = true) const noexcept
    {
      std::string s = "{ ";

      s += to_json_e<0>(all);

      if (s.size() > 2) // replace last ", " with " }"
      {
        s[s.size() - 2] = ' ';
        s[s.size() - 1] = '}';
      }
      else
        s += "}";

      return s;
    }


    UT data{};

    bool fail_not_exists_;        // if the json contains something we do not have, should we fail?

    json_handler_skip skip_;
    const char* object_start_{nullptr};
    std::string_view object_active_;

  }; // class json_handler_struct


} // namespace plate
