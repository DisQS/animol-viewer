#pragma once

#include "json_converters.hpp"

/*
    json parse handler interface

    and a helper for skipping parsing: json_handler_skip
*/


namespace plate {

  // we can use a bitset to mark which structure entries have already been parsed


  // returns the bit set which specifies the index of the value: s in container: T

  template<class T>
  constexpr uint64 mask_of_entry(const std::string_view& s, const T& t) noexcept
  {
    auto found = std::find(t.begin(), t.end(), s);

    if (found != t.end())
      return 1ull << std::distance(t.begin(), found);

    return 0;
  }


  // returns the bits set which specifies the indexes of the values: s in container: T

  template<class S, class T>
  constexpr uint64 mask_of(const S& s, const T& t) noexcept
  {
    uint64 m = 0;

    for (const auto& e : s)
      if (!e.empty())
      {
        auto found = std::find(t.begin(), t.end(), e);

        if (found != t.end())
          m |= 1 << std::distance(t.begin(), found);
      }

    return m;
  }

  
  // our json_handler interface.
  //
  // this interface is used by the json parser to signal json elements

  class json_handler
  {
  public:

    json_handler()
    {}
    
    virtual bool lhs(const std::string_view& s) noexcept { active_ = s; return true; }

    virtual bool rhs( [[maybe_unused]] const std::string_view& s) noexcept { return true; }
    virtual bool rhs( [[maybe_unused]] const bool& b)             noexcept { return true; }
    virtual bool rhs()                                            noexcept { return true; } // null
    virtual bool rhs( [[maybe_unused]] const float64& d)          noexcept { return true; }

    virtual json_handler* start_object([[maybe_unused]] const char* p) noexcept { return this; }
    virtual json_handler*   end_object([[maybe_unused]] const char* p) noexcept { return this; }

    virtual json_handler* start_array([[maybe_unused]] const char* p) noexcept { return this; }
    virtual json_handler*   end_array([[maybe_unused]] const char* p) noexcept { return this; }

    std::string_view active_;

    std::vector<std::vector<std::byte>> temp_; // store decoded stuff here, then spans are views on this

    uint64 mask_{0};   // mask indicating which values have been parsed

    int    depth_{0};  // depth of json tree, increments for an object or array

    bool ok{false}; // set by json to true on successful parse


    template<class UT, class JT>
    bool json_process(UT& t, std::size_t id, const JT& jv) noexcept
    {
      static_assert(pfr::tuple_size<UT>::value <= 50, "Add more case statements to json_process");

      bool ok = false;

      switch (id)
      {
        case  0: if constexpr ( 0 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 0>(t), jv); break;
        case  1: if constexpr ( 1 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 1>(t), jv); break;
        case  2: if constexpr ( 2 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 2>(t), jv); break;
        case  3: if constexpr ( 3 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 3>(t), jv); break;
        case  4: if constexpr ( 4 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 4>(t), jv); break;
        case  5: if constexpr ( 5 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 5>(t), jv); break;
        case  6: if constexpr ( 6 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 6>(t), jv); break;
        case  7: if constexpr ( 7 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 7>(t), jv); break;
        case  8: if constexpr ( 8 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 8>(t), jv); break;
        case  9: if constexpr ( 9 < pfr::tuple_size<UT>::value) ok = convert(pfr::get< 9>(t), jv); break;
        case 10: if constexpr (10 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<10>(t), jv); break;
        case 11: if constexpr (11 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<11>(t), jv); break;
        case 12: if constexpr (12 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<12>(t), jv); break;
        case 13: if constexpr (13 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<13>(t), jv); break;
        case 14: if constexpr (14 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<14>(t), jv); break;
        case 15: if constexpr (15 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<15>(t), jv); break;
        case 16: if constexpr (16 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<16>(t), jv); break;
        case 17: if constexpr (17 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<17>(t), jv); break;
        case 18: if constexpr (18 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<18>(t), jv); break;
        case 19: if constexpr (19 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<19>(t), jv); break;
        case 20: if constexpr (20 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<20>(t), jv); break;
        case 21: if constexpr (21 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<21>(t), jv); break;
        case 22: if constexpr (22 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<22>(t), jv); break;
        case 23: if constexpr (23 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<23>(t), jv); break;
        case 24: if constexpr (24 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<24>(t), jv); break;
        case 25: if constexpr (25 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<25>(t), jv); break;
        case 26: if constexpr (26 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<26>(t), jv); break;
        case 27: if constexpr (27 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<27>(t), jv); break;
        case 28: if constexpr (28 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<28>(t), jv); break;
        case 29: if constexpr (29 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<29>(t), jv); break;
        case 30: if constexpr (30 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<30>(t), jv); break;
        case 31: if constexpr (31 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<31>(t), jv); break;
        case 32: if constexpr (32 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<32>(t), jv); break;
        case 33: if constexpr (33 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<33>(t), jv); break;
        case 34: if constexpr (34 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<34>(t), jv); break;
        case 35: if constexpr (35 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<35>(t), jv); break;
        case 36: if constexpr (36 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<36>(t), jv); break;
        case 37: if constexpr (37 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<37>(t), jv); break;
        case 38: if constexpr (38 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<38>(t), jv); break;
        case 39: if constexpr (39 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<39>(t), jv); break;
        case 40: if constexpr (40 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<40>(t), jv); break;
        case 41: if constexpr (41 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<41>(t), jv); break;
        case 42: if constexpr (42 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<42>(t), jv); break;
        case 43: if constexpr (43 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<43>(t), jv); break;
        case 44: if constexpr (44 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<44>(t), jv); break;
        case 45: if constexpr (45 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<45>(t), jv); break;
        case 46: if constexpr (46 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<46>(t), jv); break;
        case 47: if constexpr (47 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<47>(t), jv); break;
        case 48: if constexpr (48 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<48>(t), jv); break;
        case 49: if constexpr (49 < pfr::tuple_size<UT>::value) ok = convert(pfr::get<49>(t), jv); break;
      }

      if (ok) // parsed ok so mark it as done
        mask_ |= (1ull << id);

      return ok;
    }


    template<class UT, class JT>
    bool rhs_auto(UT& uv, const JT& jv, const bool fail_not_exists) noexcept
    {
       if (const auto found = std::find(UT::lookup_.begin(), UT::lookup_.end(), active_); found != UT::lookup_.end())
         return json_process(uv, std::distance(UT::lookup_.begin(), found), jv);

      return !fail_not_exists;
    }

  }; // class json_handler


  // utility class for skipping over an array or object

  class json_handler_skip : public json_handler
  {
    public:

      json_handler_skip(json_handler* parent) noexcept
      {
        parent_ = parent;
      }

      json_handler* start_object([[maybe_unused]] const char* p) noexcept
      {
        ++depth_;

        return this;
      }

      json_handler* end_object(const char* p) noexcept
      {
        --depth_;

        if (depth_ >= 0)
          return this;
        else
        {
          depth_ = 0;

          // let the parent know we are at the end

          parent_->end_object(p);

          return parent_;
        }
      }

      json_handler* start_array([[maybe_unused]] const char* p) noexcept
      {
        ++depth_;

        return this;
      }

      json_handler* end_array(const char* p) noexcept
      {
        --depth_;

        if (depth_ >= 0)
          return this;
        else
        {
          depth_ = 0;

          // let the parent know we are at the end

          parent_->end_array(p);

          return parent_;
        }
      }

    private:

      json_handler* parent_;
  };

} // namespace plate
