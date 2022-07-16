#include "system/webgl/log.hpp"
#include "system/common/json/json.hpp"

// https://github.com/priimak/scala-data/blob/master/NAMD-DCD-File-Format.md

#include <string_view>
#include <string>
#include <span>
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>

#include "string_data.hpp"


namespace animol {

class dcd2pdb
{

public:

  // structure storing info useful for extracting dcd data from a dcd file

  struct info
  {
    int number_frames;
    int number_atoms;
    std::uint64_t start_offset;

    std::uint64_t get_frame_offset(int frame_id) const noexcept
    {
      return start_offset + (frame_id * get_frame_size());
    }

    std::uint64_t get_frame_size() const noexcept
    {
      // Each coordinate (x, y and z) has:
      // 4 bytes for start_length
      // 4 bytes for end_length
      // 4 bytes per atom for the float cordinate

      return (4 + 4 + (4 * number_atoms)) * 3;
    }
  };


  // interface structure to allow passing key details between processes using json

  struct interface
  {
    std::string_view psf_url;
    std::string_view dcd_url;

    int           frame;
    int           number_atoms;
    std::uint64_t dcd_data_offset;

    static constexpr std::array<std::string_view, 5> lookup_ =
    {{
      "psf_url", "dcd_url", "frame", "number_atoms", "dcd_data_offset"
    }};

    static constexpr std::uint64_t must_ = plate::mask_of(std::array<std::string_view, 5>
    {{
      "psf_url", "dcd_url", "frame", "number_atoms", "dcd_data_offset"
    }}, lookup_);

    static constexpr std::uint64_t may_  = std::numeric_limits<std::uint64_t>::max();
  };


  static constexpr double time_multiplier = 48.88821;

  struct dcd_header
  {
    std::int32_t len;
    std::byte    cord[4];
    std::int32_t nset;          // number of sets of dynamic atoms
    std::int32_t istart;
    std::int32_t nsavc;
    std::int32_t unused1[5];
    std::int32_t fixed_atoms;
    float        dt;
    std::int32_t unused2[10];
    std::int32_t len_confirm;

    inline static std::string_view fcord = "CORD";

    bool is_valid() const noexcept
    {
      if (len != 84 || len != len_confirm || std::memcmp(cord, fcord.data(), 4) != 0 || fixed_atoms != 0)
        return false;
      else
        return true;
    }

    std::string to_string() const noexcept
    {
      return fmt::format(FMT_COMPILE("len: {}\ncord: {}{}{}{}\nnset: {}\nistart: {}\nnsavc: {}\nfixed_atoms: {}\ndt: {} ({})\nlen_confirm: {}\n"),
        len, cord[0], cord[1], cord[2], cord[3], nset, istart, nsavc, fixed_atoms, dt, static_cast<double>(dt) * time_multiplier, len_confirm);
    }
  };

  static_assert(sizeof(dcd_header) == 92);


  dcd2pdb()
  {
  }


  bool generate_template(std::span<const std::byte> psf_data, bool keepCAs, bool keepNonCAs) noexcept
  {
    return generate_template(std::span(reinterpret_cast<const char*>(psf_data.data()), psf_data.size()), keepCAs, keepNonCAs);
  }


  bool generate_template(std::span<const char> psf_data, bool keepCAs, bool keepNonCAs) noexcept
  {
    keepCAs_    = keepCAs;
    keepNonCAs_ = keepNonCAs;

    pdb_data_.clear();
    indexes_.clear();

    std::unordered_map<std::string_view, char> chain_names;

    number_atoms_ = 0;

    string_data psf(psf_data);

    for (std::string_view line = psf.getline(); !(line.empty() && psf.end()); line = psf.getline() )
    {
      if (line.find("!NATOM") == std::string::npos)
        continue;

      auto start = line.find_first_not_of(" ");

      if (start == std::string::npos)
      {
        log_debug("no non-space character found on !NATOM line");
        return false;
      }

      auto end = line.find(" ", start);

      if (end == std::string::npos)
      {
        log_debug("no space character found after non-space character on !NATOM line");
        return false;
      }

      if (auto [p, ec] = std::from_chars(&line[start], &line[end], number_atoms_); ec != std::errc() || p != &line[end])
      {
        log_debug("from_chars error");
        return false;
      }

      break;
    }

    if (!number_atoms_)
    {
      log_debug("!NATOM not found in psf file");
      return false;
    }

    pdb_data_.reserve(number_atoms_ * 55);

    // parse main data out of psf

    std::int32_t counter = 0;

    char next_chain = 'A';

    for (std::string_view line = psf.getline(); !(line.empty() && psf.end()); line = psf.getline() )
    {
      if (counter == number_atoms_)
        break;

      ++counter;

                                       // 0    1      2       3        4
      std::vector<std::string_view> r; // num, chain, resnum, residue, atom

      string_data::split(r, line);

      if (r.size() < 5)
      {
        log_debug(FMT_COMPILE("Bad line: {}"), line);
        return false;
      }

      const bool isCA = (r[4] == "CA");

      if ( ! (   (keepCAs  &&  isCA)
            || (keepNonCAs && !isCA) ) )
        continue;

      if (r[3] == "HSD")
        r[3] = "HIS";

      auto it = chain_names.find(r[1]);

      if (it == chain_names.end())
      {
        if (next_chain == 'Z')
        {
          log_debug("too many chains for alphabet:");
          return false;
        }

        bool ok;
        std::tie(it, ok) = chain_names.insert({ r[1], next_chain++ });
      }

      //pdb_data_ += fmt::format(FMT_COMPILE("ATOM        {: ^4} {: <3} {:1}{: >4}                            \n"), r[4], r[3], it->second, r[2]);

      if (r[4].size() == 4) // left shifted as atom has 4 characters
        pdb_data_ += fmt::format(FMT_COMPILE("ATOM        {: <4} {: <3} {:1}{: >4}                            \n"), r[4], r[3], it->second, r[2]);
      else
        pdb_data_ += fmt::format(FMT_COMPILE("ATOM         {: <3} {: <3} {:1}{: >4}                            \n"), r[4], r[3], it->second, r[2]);

      indexes_.emplace_back(counter - 1);
    }

    if (counter != number_atoms_)
    {
      log_debug(FMT_COMPILE("too few lines in psf. should be {}. got {}"), number_atoms_, counter);
      return false;
    }

    return true;
  }


  bool populate_template(std::span<const std::byte> dcd_data) // dcd_data is the exact block of data wanted
  {
    auto pos = dcd_data.data();

    if (dcd_data.size() < (4 + 4 + (4 * number_atoms_)) * 3)
    {
      log_debug(FMT_COMPILE("bad dcd_data size: {} wanted: {}"), dcd_data.size(), (4 + 4 + (4 * number_atoms_)) * 3);
      return false;
    }

    std::array<const float*, 3> p; // x,y and z data pointers

    for (int j = 0; j < 3; ++j)
    {
      std::int32_t len_start;

      std::memcpy(&len_start, pos, 4);
      pos += 4;

      if (len_start != 4 * number_atoms_)
      {
        log_debug(FMT_COMPILE("entry: {} has bad len_start: {} should be: {}"), j, len_start, 4 * number_atoms_);
        return false;
      }

      p[j] = reinterpret_cast<const float*>(pos);
      pos += len_start;

      std::int32_t len_end;

      std::memcpy(&len_end, pos, 4);
      pos += 4;

      if (len_end != 4 * number_atoms_)
      {
        log_debug(FMT_COMPILE("entry: {} has bad len_end: {} should be: {}"), j, len_end, 4 * number_atoms_);
        return false;
      }
    }

    for (std::int32_t i = 0; i < static_cast<std::int32_t>(indexes_.size()); ++i)
      fmt::format_to(&pdb_data_[i*55+31], FMT_COMPILE("{: >7.3f} {: >7.3f} {: >7.3f}\n"), p[0][indexes_[i]], p[1][indexes_[i]], p[2][indexes_[i]]);

    return true;
  }



  static std::optional<info> get_dcd_data_info(std::span<const std::byte> dcd_data) noexcept // this could just be a 'small' amount of the actual dcd file
  {
    info i;

    if (dcd_data.size() < 92)
    {
      log_debug(FMT_COMPILE("bad dcd_data size: {}"), dcd_data.size());
      return std::nullopt;
    }

    auto addr = dcd_data.data();
    auto pos = addr;

    auto h = reinterpret_cast<const dcd_header*>(pos);

    if (!h->is_valid())
    {
      log_debug(FMT_COMPILE("bad dcd header: {}"), h->to_string());
      return std::nullopt;
    }

    i.number_frames = h->nset;

    // read in titles block

    pos += sizeof(dcd_header);

    if (dcd_data.size() <= (pos - addr) + 12)
    {
      log_debug(FMT_COMPILE("bad size: {}"), dcd_data.size());
      return std::nullopt;
    }

    std::int32_t title_blk_size;
    std::int32_t ntitle;

    std::memcpy(&title_blk_size, pos, 4);   pos += 4;
    std::memcpy(&ntitle,         pos, 4);   pos += 4;

    if (ntitle * 80 + 4 != title_blk_size)
    {
      log_debug(FMT_COMPILE("bad title_blks_size: {} ntitle: {}"), title_blk_size, ntitle);
      return std::nullopt;
    }

    if (dcd_data.size() <= (pos - addr) + 12 + title_blk_size)
    {
      log_debug(FMT_COMPILE("bad size: {}"), dcd_data.size());
      return std::nullopt;
    }

    //std::vector<std::string> titles;

    //for (std::int32_t i = 0; i < ntitle; ++i)
    //{
    //  titles.emplace_back(reinterpret_cast<char*>(pos), 80);
    //  pos += 80;
    //}
    pos += 80 * ntitle;

    std::int32_t title_blk_size_confirm;

    std::memcpy(&title_blk_size_confirm, pos, 4);   pos += 4;
  
    if (title_blk_size_confirm != title_blk_size)
    {
      log_debug(FMT_COMPILE("bad title block sizes, got: {} and {}"), title_blk_size, title_blk_size_confirm);
      return std::nullopt;
    }

    // read in number of atoms block

    if (dcd_data.size() <= (pos - addr) + 12)
    {
      log_debug(FMT_COMPILE("bad size: {}"), dcd_data.size());
      return std::nullopt;
    }

    std::int32_t len_start;
    std::int32_t len_end;

    std::int32_t natoms;

    std::memcpy(&len_start, pos, 4);  pos += 4;
    std::memcpy(&natoms,    pos, 4);  pos += 4;
    std::memcpy(&len_end,   pos, 4);  pos += 4;

    if (len_start != 4 || len_end != 4)
    {
      log_debug(FMT_COMPILE("bad len_start: {} len_end: {}"), len_start, len_end);
      return std::nullopt;
    }

    i.number_atoms = natoms;
    i.start_offset = pos - addr;

    return {i};
  }


  auto get_number_of_atoms() const noexcept
  {
    return number_atoms_;
  }


  const std::string& get_pdb_data() const noexcept
  {
    return pdb_data_;
  }


private:


  std::vector<std::int32_t> indexes_;

  std::string pdb_data_;

  std::int32_t number_atoms_{0};

  bool keepCAs_;
  bool keepNonCAs_;


}; // class dcd2pdb

} // namespace animol
