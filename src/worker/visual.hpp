#pragma once

#include <vector>
#include <string>
#include <span>
#include <array>

#include "../db/db.hpp"

#include "string_data.hpp"

/*
    generate webgl data to visualise proteins in different ways
*/


namespace animol {


class visualise
{

public:

  static constexpr std::uint32_t ATOMS                    = 1 << 0;
  static constexpr std::uint32_t HETATOMS                 = 1 << 1;
  static constexpr std::uint32_t SHIFT_TO_CENTER          = 1 << 2;
  static constexpr std::uint32_t SHIFT_TO_CENTER_OF_MASS  = 1 << 3;


  struct atom
  {
    std::array<short, 3>   position;
    short                  radius;
    std::array<uint8_t, 3> color;
    uint8_t                reserved; // 255
  };

  static_assert(sizeof(atom) == 12, "atom struct expands to a bad size");


  visualise(std::span<char> pdb_data) noexcept :
    data_(pdb_data)
  {
  }


  void generate_atoms(std::vector<atom>& d, std::uint32_t options) noexcept
  {
    if ((options & SHIFT_TO_CENTER) && !calc_center())
      return;

    if ((options & SHIFT_TO_CENTER_OF_MASS) && !calc_center_of_mass())
      return;

    data_.to_start();

    for (auto line = data_.getline(); !line.empty(); line = data_.getline())
    {
      if ((options & ATOMS) && line.starts_with("ATOM "))
        add_atom(d, line, options);

      if ((options & HETATOMS) && line.starts_with("HETATM"))
        add_atom(d, line, options);
    }
  }


private:


  bool calc_center() noexcept
  {
    std::array<float, 3> min = { std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max(),
                                 std::numeric_limits<float>::max() };

    std::array<float, 3> max = { std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest(),
                                 std::numeric_limits<float>::lowest() };

    data_.to_start();

    for (auto line = data_.getline(); !line.empty(); line = data_.getline())
    {
      if (!line.starts_with("ATOM "))
        continue;

      std::array<float, 3> cur;

      bool ok;

      ok  = string_data::parse_float(cur[0], line.substr(30, 8));
      ok &= string_data::parse_float(cur[1], line.substr(38, 8));
      ok &= string_data::parse_float(cur[2], line.substr(46, 8));

      if (!ok)
      {
        log_debug(FMT_COMPILE("Unable to parse positions in line: {}"), line);
        return false;
      }

      for (int i = 0; i < 3; ++i)
      {
        min[i] = std::min(min[i], cur[i]);
        max[i] = std::max(max[i], cur[i]);
      }
    }

    for (int i = 0; i < 3; ++i)
      center_[i] = (min[i] + max[i]) / 2.0f;

    return true;
  }


  bool calc_center_of_mass() noexcept
  {
    data_.to_start();

    std::array<float, 3> sum_m = { 0, 0, 0 };

    float tot_mass = 0;

    for (auto line = data_.getline(); !line.empty(); line = data_.getline())
    {
      if (!line.starts_with("ATOM "))
        continue;

      std::array<float, 3> cur;

      bool ok;

      ok  = string_data::parse_float(cur[0], line.substr(30, 8));
      ok &= string_data::parse_float(cur[1], line.substr(38, 8));
      ok &= string_data::parse_float(cur[2], line.substr(46, 8));

      if (!ok)
      {
        log_debug(FMT_COMPILE("Unable to parse positions in line: {}"), line);
        return false;
      }

      auto atom_name = string_data::strip_spaces(line.substr(12, 2));

      auto atom_id = db::get_atomic_id(atom_name);

      if (atom_id == 0)
      {
        // pdb files sometimes code the remoteness in the first two columns if the string size is 4 so.. try chopping it..

        atom_id = db::get_atomic_id(atom_name.substr(0,1));

        if (atom_id == 0)
          continue;
      }

      for (int i = 0; i < 3; ++i)
        sum_m[i] += cur[i] * db::elements[atom_id].atomic_mass;

      tot_mass += db::elements[atom_id].atomic_mass;
    }

    for (int i = 0; i < 3; ++i)
      center_[i] = sum_m[i] / tot_mass;

    return true;
  }


  void add_atom(std::vector<atom>& d, const std::string_view& line, const std::uint32_t& options) noexcept
  {
    // extract atom name and position

    auto atom_name = string_data::strip_spaces(line.substr(12, 2));

//    if (atom_name != "C") // hack for testing
//      return;
//
//    auto remoteness = string_data::strip_spaces(line.substr(14, 1));
//
//    if (remoteness != "A") // hack for testing
//      return;

    auto atom_id = db::get_atomic_id(atom_name);

    if (atom_id == 0)
    {
      // pdb files sometimes code the remoteness in the first two columns if the string size is 4 so.. try chopping it..

      atom_id = db::get_atomic_id(atom_name.substr(0,1));

      if (atom_id == 0)
      {
        fmt::print("Unable to find atom properties for: {} in line: {}\n", atom_name, line);
        return;
      }
    }

    const auto& elem = db::elements[atom_id];

    float x, y, z;

    bool ok;

    ok  = string_data::parse_float(x, line.substr(30, 8));
    ok &= string_data::parse_float(y, line.substr(38, 8));
    ok &= string_data::parse_float(z, line.substr(46, 8));

    if (!ok)
    {
      fmt::print("Unable to parse atom position for: {} in line: {}\n", atom_name, line);
      return;
    }

    d.resize(d.size() + 1);

    auto& ad = d.back();

    if (options | SHIFT_TO_CENTER)
      ad.position = { to_short(x - center_[0]), to_short(y - center_[1]), to_short(z - center_[2]) };
    else
      ad.position = { to_short(x), to_short(y), to_short(z) };

    ad.radius   = elem.atomic_radius;
    ad.color    = elem.cpk_color;
    ad.reserved = 255;
  }

  
  inline short to_short(const float& f) const noexcept
  {
    return (32768.0 / 400.0) * f; // ugh fixme
  }


  string_data data_;

  std::array<float, 3> center_;

}; // class visualise

} // namespace animol
