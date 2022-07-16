#pragma once

#include <vector>
#include <string>
#include <span>
#include <string_view>
#include <charconv>

#include "fast_float/fast_float.h"


/*
    utility class to help parse string data.
*/

namespace animol {

class string_data
{

public:


  string_data(std::span<const char> data) noexcept :
    data_(data)
  {
    to_start();

    end_ = pos_ + data.size();
  }


  inline void to_start() noexcept
  {
    pos_      = data_.data();
    prev_pos_ = pos_;
  }


  std::string_view getline() noexcept
  {
    prev_pos_ = pos_;

    while (pos_ < end_ && *pos_ != '\n')
      ++pos_;

    std::string_view sv(prev_pos_, pos_ - prev_pos_);

    if (pos_ < end_)
      ++pos_;

    return sv;
  }

  bool end() const noexcept
  {
    return pos_ >= end_;
  }


  static void split(std::vector<std::string_view>& r, const std::string_view line) noexcept
  {
    r.clear();

    std::int64_t i = 0;

    const auto end = std::ssize(line);

    while (true)
    {
      while (line[i] == ' ' && i < end)
        ++i;

      if (i >= end)
        return;

      auto start = i++;

      while (line[i] != ' ' && i < end)
        ++i;

      r.emplace_back(&line[start], i - start);

      ++i;
    }
  }


  static std::string_view strip_spaces(std::string_view s) noexcept
  {
    while (!s.empty() && s.front() == ' ')
      s = s.substr(1);

    while (!s.empty() && s.back() == ' ')
      s = s.substr(0, s.size() - 1);

    return s;
  }


  static bool parse_float(float& f, std::string_view s) noexcept
  {
    s = strip_spaces(s);

    if (s.empty())
      return false;

    if (auto [p, ec] = fast_float::from_chars(&s[0], &s[s.size()], f); ec != std::errc() || p != &s[s.size()])
      return false;
    else
      return true;
  }



private:


  std::span<const char> data_;

  const char* pos_;
  const char* prev_pos_;
  const char* end_;
};

} // namespace animol
