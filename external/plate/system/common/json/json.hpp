#pragma once

#include "json_handler.hpp"
#include "json_handler_struct.hpp"
#include "json_handler_vector.hpp"
#include "json_handler_funcs.hpp"


/*
    A json parser, used in conjuntion with a json_handler.
*/


namespace plate {

class json
{

public:

  json() noexcept
  {
    msg_      = nullptr;
    msg_size_ = 0;
    cur_      = msg_;
    msg_end_  = cur_;
  }


  json(std::string_view s) noexcept
  {
    msg_      = s.data();
    msg_size_ = s.size();
    cur_      = msg_;
    msg_end_  = msg_ + msg_size_;
  }


  json(char* msg, uint64 msg_size) noexcept :
    msg_(msg),
    msg_size_(msg_size),
    cur_(msg)
  {
    msg_end_ = msg_ + msg_size_;
  }


  json(std::span<std::byte> d) noexcept :
    msg_(reinterpret_cast<char*>(d.data())),
    msg_size_(d.size()),
    cur_(msg_)
  {
    msg_end_ = msg_ + msg_size_;
  }


  bool parse(json_handler* j) noexcept
  {
    j->ok     = false;
    j->depth_ = 0;
    j->mask_  = 0;

    if (do_parse(j))
      j->ok = true;

    return j->ok;
  }


  bool parse(char* msg, uint64 msg_size, json_handler* j) noexcept
  {
    msg_ = msg;
    msg_size_ = msg_size;
    cur_ = msg;
    msg_end_ = msg_ + msg_size_;

    return parse(j);
  }


  uint32 get_position() noexcept
  {
    return cur_ - msg_;
  }


private:


  bool do_parse(json_handler* j) noexcept
  {
    j_ = j;
    cur_ = msg_;

    parse_whitespace();

    if (cur_ >= msg_end_)
      return false;

    if (*cur_ == '{')
    {
      auto res = j_->start_object(cur_);

      if (!res)
        return false;

      j_ = res;

     return parse_object();
    }

    if (*cur_ == '[')
    {
      auto res = j_->start_array(cur_);

      if (!res)
        return false;

      j_ = res;

      return parse_array();
    }

    return false;
  }


  inline bool parse_whitespace() noexcept
  {
    while (cur_ < msg_end_)
    {
      if (*cur_ == ' ' || *cur_ == '\r' || *cur_ == '\n' || *cur_ == '\t')
      {
        ++cur_;
        continue;
      }

      return true;
    }

    return true;
  }


  bool parse_hex() noexcept
  {
    if (cur_ == msg_end_)
      return false;

    if ((*cur_ >= '0' && *cur_ <= '9') || (*cur_ >= 'a' && *cur_ <= 'f') || (*cur_ >= 'A' && *cur_ <= 'F'))
      return true;

    return false;
  }


  std::optional<float64> parse_number() noexcept
  {
    // find the end of the stringified number and pass to fast conversion routine

    auto start = cur_;

    while (cur_ != msg_end_)
    {
      if ((*cur_ >= '0' && *cur_ <= '9') || *cur_ == '-' || *cur_ == '.' || *cur_ == '+' || *cur_ == 'e' || *cur_ == 'E')
      {
        cur_++;
        continue;
      }

      // found the end

      if (cur_ == start)
        return std::nullopt;

      float64 d;

      if (auto [p, ec] = fast_float::from_chars(reinterpret_cast<const char*>(start),
                reinterpret_cast<const char*>(start) + (cur_ - start), d); ec != std::errc() || p != cur_)
        return std::nullopt;

      return { d };
    }

    return std::nullopt;
  }


  std::optional<std::string_view> parse_number_as_string() noexcept
  {
    auto start = cur_;

    while (cur_ != msg_end_)
    {
      if ((*cur_ >= '0' && *cur_ <= '9') || *cur_ == '-' || *cur_ == '.' || *cur_ == '+' || *cur_ == 'e' || *cur_ == 'E')
      {
        cur_++;
        continue;
      }

      // found the end

      if (cur_ == start)
        return std::nullopt;

      return { std::string_view(start, cur_ - start) };
    }

    return std::nullopt;
  }


  std::optional<std::string_view> parse_string() noexcept
  {
    if (cur_ == msg_end_ || *cur_ != '"')
      return std::nullopt;

    cur_++;

    auto start = cur_;
    
    while (cur_ != msg_end_)
    {
      if (*cur_ == '\\')
      {
        cur_++; // read in the control character
        if (cur_ == msg_end_)
          return std::nullopt;

        if (*cur_ != '"' && *cur_ != '\\' && *cur_ != '/' && *cur_ != 'b' && *cur_ != 'f' && *cur_ != 'n' && *cur_ != 'r' && *cur_ != 't' && *cur_ != 'u')
          return std::nullopt;

        if (*cur_ == 'u') // skips 4 more hex characters
        {
          for (int i = 0; i < 4; i++)
          {
            cur_++;
            if (!parse_hex())
              return std::nullopt;
          }
        }
      }
      else
      {
        if (*cur_ == '"')
        {
          cur_++;
          return { std::string_view(start, cur_ - start - 1) };
        }
      }
      cur_++;
    }

    return std::nullopt;
  }


  bool parse_true() noexcept
  {
    constexpr const std::string_view sv = "true";
    constexpr const auto sv_size = sv.size();

    if (cur_ + sv_size >= msg_end_)
      return false;

    if (memcmp(sv.data(), cur_, sv_size) == 0)
    {
      cur_ += sv_size;

      return true;
    }

    return false;
  }


  bool parse_false() noexcept
  {
    constexpr const std::string_view sv = "false";
    constexpr const auto sv_size = sv.size();

    if (cur_ + sv_size >= msg_end_)
      return false;

    if (memcmp(sv.data(), cur_, sv_size) == 0)
    {
      cur_ += sv_size;

      return true;
    }

    return false;
  }


  bool parse_null() noexcept
  {
    constexpr std::string_view sv = "null";
    constexpr auto sv_size = sv.size();

    if (cur_ + sv_size >= msg_end_)
      return false;

    if (memcmp(sv.data(), cur_, sv_size) == 0)
    {
      cur_ += sv_size;

      return true;
    }

    return false;
  }


  bool parse_value() noexcept
  {
    parse_whitespace();

    if (*cur_ == '"')
    {
      auto sv = parse_string();

      if (sv)
        return j_->rhs(*sv);

      return false;
    }
    else
    {
      if (*cur_ == 't')
      {
        return parse_true() && j_->rhs(true);
      }
      else
      {
        if (*cur_ == 'f')
        {
          return parse_false() && j_->rhs(false);
        }
        else
        {
          if (*cur_ == 'n')
          {
            return parse_null() && j_->rhs();
          }
          else
          {
            if ((*cur_ >= '0' && *cur_ <= '9') || *cur_ == '-')
            {
              //auto d = parse_number();
              //if (d) return j_->rhs(*d);
              auto sv = parse_number_as_string();
              if (sv)
                return j_->rhs(*sv);

              return false;
            }
            else
            {
              if (*cur_ == '{')
              {
                auto res = j_->start_object(cur_);

                if (!res)
                  return false;

                j_ = res;

                return parse_object();
              }
              else
              {
                if (*cur_ == '[')
                {
                  auto res = j_->start_array(cur_);

                  if (!res)
                    return false;

                  j_ = res;

                  return parse_array();
                }
                else
                {
                  return false;
                }
              }
            }
          }
        }
      }
    }

    // never gets here
  }


  bool parse_array() noexcept
  {
    if (*cur_ != '[')
      return false;

    cur_++;

    parse_whitespace();

    if (*cur_ == ']') // an empty array
    {
      cur_++;

      auto res = j_->end_array(cur_);

      if (!res)
        return false;

      j_ = res;

      return true;
    }

    do
    {
      if (!parse_value())
        return false;

      parse_whitespace();

      if (*cur_ == ',')
      {
        cur_++;
        continue;
      }

      if (*cur_ == ']') // end of array
      {
        cur_++;

        auto res = j_->end_array(cur_);

        if (!res)
          return false;

        j_ = res;

        return true;
      }

      return false; // not a , or ]

    } while (true);
  }


  bool parse_object() noexcept
  {
    if (!cur_)
      return false;

    if (*cur_ != '{')
      return false;

    cur_++;

    parse_whitespace();

    while (cur_ != msg_end_)
    {
      // a lhs

      auto sv = parse_string();
      if (!sv || !j_->lhs(*sv))
        return false;

      parse_whitespace();

      if (cur_ == msg_end_ || *cur_ != ':')
        return false;

      cur_++;

      if (!parse_value())
        return false;

      parse_whitespace();

      if (cur_ == msg_end_)
        return false;

      if (*cur_ == '}')
      {
        cur_++;

        auto res = j_->end_object(cur_);

        if (!res)
          return false;

        j_ = res;

        parse_whitespace();

        //if (cur_ == msg_end_)
        //  return false;

        return true;
      }

      if (*cur_ != ',')
        return false;

      cur_++;

      parse_whitespace();
    }

    return false;
  }


  const char* msg_     = nullptr;
  uint64 msg_size_     = 0;
  const char* cur_     = nullptr;
  const char* msg_end_ = nullptr;

  json_handler* j_     = nullptr;

}; // class json


// helpers to simplify usage

template<class UT>
auto json_parse_struct(std::string_view s, bool fail_not_exists = false) noexcept
{
  json_handler_struct<UT> h(fail_not_exists);

  json j(s);

  j.parse(&h);

  return std::move(h);
}


template<class UT>
auto json_parse_vector(std::string_view s, bool fail_not_exists = false) noexcept
{
  json_handler_vector<UT> h(fail_not_exists);

  json j(s);

  j.parse(&h);

  return std::move(h);
}


auto json_parse_funcs(std::string_view s, std::span<json_handler_funcs::lookup> cbs, bool fail_not_exists = false) noexcept
{
  json_handler_funcs h(cbs, fail_not_exists);

  json j(s);

  j.parse(&h);

  return std::move(h);
}


} // namespace plate
