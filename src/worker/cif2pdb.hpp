#include "system/webgl/log.hpp"

// https://mmcif.wwpdb.org/pdbx-mmcif-home-page.html

//#include <emscripten/emscripten.h>
//#include <emscripten/bind.h>


#include <string_view>
#include <string>
#include <span>
#include <map>
#include <vector>

#include "magic_enum.hpp"

#include "string_data.hpp"


namespace animol {

class cif2pdb
{

public:

  enum class options
  {
    none         =  0,
    ca_atoms     =  1,
    non_ca_atoms =  2,
    sheet        =  4,
    helix        =  8,
    hetatm       = 16
  };


  cif2pdb(std::span<const std::byte> cif_data, options o) noexcept
  {
    options_ = o;

    start_ = reinterpret_cast<const char*>(cif_data.data());
    end_   = start_ + cif_data.size();
    pos_   = start_;
  }


  std::string* convert() noexcept
  {
    bool ok = false;

    for (auto line = getline(); !line.empty(); line = getline())
    {
      if (line.starts_with('#'))
        continue;

      if (is_loop(line))
      {
        ok = process_loop();
        if (!ok)
          break;
      }
    }

    if (ok)
      return &pdb_;
    else
      return nullptr;
  }


private:


  // info about what is expected in a column of a loop section
  struct column
  {
    std::string_view name;    // column name
    bool             must;    // must have this
    int              pos;     // the column index this is in
    int              max_len; // the maximum allowable char length (-1 for do not check)
  };


  bool is_loop(std::string_view sv) const noexcept
  {
    static const std::string_view loop = "loop_";

    if (sv.size() != loop.size())
      return false;

    for (unsigned int i = 0; i < loop.size(); ++i)
      if (std::tolower(sv[i]) != loop[i])
        return false;

    return true;
  }


  bool is_stop(std::string_view sv) const noexcept
  {
    static const std::string_view stop = "stop_";

    if (sv.size() != stop.size())
      return false;

    for (unsigned int i = 0; i < stop.size(); ++i)
      if (std::tolower(sv[i]) != stop[i])
        return false;

    return true;
  }


  bool is_global(std::string_view sv) const noexcept
  {
    static const std::string_view global = "global_";

    if (sv.size() != global.size())
      return false;

    for (unsigned int i = 0; i < global.size(); ++i)
      if (std::tolower(sv[i]) != global[i])
        return false;

    return true;
  }


  bool is_reserved(std::string_view sv) const noexcept
  {
    return is_loop(sv) || is_stop(sv) || is_global(sv);
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


  inline void rewind() noexcept
  {
    pos_ = prev_pos_;
  }


  bool skip_loop() noexcept
  {
    // read to end of headers

    for (auto line = getline(); !line.empty(); line = getline())
    { 
      if (line.starts_with('#'))
        continue;

      if (!line.starts_with('_'))
      {
        rewind();
        break;
      }
    }

    // read to end of data

    for (auto line = getline(); !line.empty(); line = getline())
    { 
      if (line.starts_with('#'))
        continue;

      if (line.starts_with('_') || is_reserved(line))
      {
        rewind();
        break;
      }
    }

    return true;
  }


  // process the headers of a loop section into columns
  int read_headers(std::span<column> a) noexcept
  {
    int i = 0;
  
    for (auto line = getline(); !line.empty(); line = getline())
    {
      if (line.starts_with('#'))
        continue;

      if (!line.starts_with('_')) // at data
      {
        rewind();
        break;
      }
  
      for (auto& o : a)
      {
        if (line.starts_with(o.name))
        {
          o.pos = i;
          break;
        }
      }
  
      ++i;
    }
  
    // check we have all the headers we want
  
    for (auto& o : a)
      if (o.must && o.pos == -1)
      {
        log_debug(FMT_COMPILE("missing offset: {}"), o.name);
        return -1;
      }

    return i;
  }


  // parse the data section of a loop block
  bool read_data(int entries, std::span<const column> a, std::function< bool (std::vector<std::string_view>&)> f, std::vector<std::string_view> start_cols = std::vector<std::string_view>{}) noexcept
  {
    std::vector<std::string_view> v;

    for (auto line = getline(); !line.empty(); line = getline())
    { 
      if (line.starts_with('#'))
        continue;
      
      if (line.starts_with('_') || is_reserved(line))
      { 
        rewind();
        return true;
      }

      string_data::split(v, line);

      if (!start_cols.empty() && !v.empty())
      {
        bool use_line = false;

        for (auto start_col : start_cols)
          if (v[0] == start_col)
            use_line = true;

        if (!use_line)
          continue;
      }
      
      if (std::ssize(v) != entries)
      { 
        log_debug(FMT_COMPILE("bad number of entries: {} expected: {} line: {}"), v.size(), entries, line);
        return false;
      }

      // check strings are within limits

      for (int i = 0; i < std::ssize(a); ++i)
      {
        if (a[i].pos < 0 || a[i].max_len < 0)
          continue;

        if (a[i].max_len < std::ssize(v[a[i].pos]))
        {
          log_debug(FMT_COMPILE("bad length: {} is greater than: {} for: {} line: {}"), v[a[i].pos].size(), a[i].max_len, a[i].name, line);
          return false;
        }
      }

      if (!f(v))
        return false;
    }

    return true;
  }


  bool process_atom_site() noexcept
  {
    using namespace magic_enum::bitwise_operators;

    std::array<column, 8> offsets =
    {{
      { "_atom_site.group_PDB",     true, -1, -1 },
      { "_atom_site.label_atom_id", true, -1,  4 },
      { "_atom_site.label_comp_id", true, -1,  3 },
      { "_atom_site.label_asym_id", true, -1,  2 },
      { "_atom_site.label_seq_id",  true, -1,  4 },
      { "_atom_site.Cartn_x",       true, -1, -1 },
      { "_atom_site.Cartn_y",       true, -1, -1 },
      { "_atom_site.Cartn_z",       true, -1, -1 }
    }};

    int entries = read_headers(offsets);

    if (entries == -1)
      return false;

    std::vector<std::string_view> atom_type; // ATOM and/or HETATM

    if ((options_ & options::ca_atoms) != options::none || (options_ & options::non_ca_atoms) != options::none)
      atom_type.emplace_back("ATOM");

    if ((options_ & options::hetatm) != options::none)
      atom_type.emplace_back("HETATM");
  
    return read_data(entries, offsets, [&] (std::vector<std::string_view>& v)
    {
	    /*
      if (!match_atom_.empty() && v[offsets[1].pos] != match_atom_)
        return true;
	    */
	
  	  const bool isCA = (v[offsets[1].pos] == "CA");
	
      if (v[offsets[0].pos] == "ATOM")
        if ( !(   ((options_ & options::ca_atoms)     != options::none &&  isCA) 
               || ((options_ & options::non_ca_atoms) != options::none && !isCA) ) )
          return true;
	
      float x, y, z;

      if (auto [p, ec] = fast_float::from_chars(v[offsets[5].pos].begin(), v[offsets[5].pos].end(), x); ec != std::errc() || p != v[offsets[5].pos].end())
      {
        log_debug(FMT_COMPILE("from_chars error for x: {}"), v[offsets[5].pos]);
        return false;
      }

      if (auto [p, ec] = fast_float::from_chars(v[offsets[6].pos].begin(), v[offsets[6].pos].end(), y); ec != std::errc() || p != v[offsets[6].pos].end())
      {
        log_debug(FMT_COMPILE("from_chars error for y: {}"), v[offsets[6].pos]);
        return false;
      }

      if (auto [p, ec] = fast_float::from_chars(v[offsets[7].pos].begin(), v[offsets[7].pos].end(), z); ec != std::errc() || p != v[offsets[7].pos].end())
      {
        log_debug(FMT_COMPILE("from_chars error for z: {}"), v[offsets[7].pos]);
        return false;
      }

      if (v[offsets[1].pos].size() == 4) // left shifted as atom has 4 characters
        pdb_ += fmt::format(FMT_COMPILE("{: <6}      {: <4} {: <3} {:1}{: >4}     {: >7.3f} {: >7.3f} {: >7.3f}\n"),
            v[offsets[0].pos], v[offsets[1].pos], v[offsets[2].pos], v[offsets[3].pos][0], v[offsets[4].pos], x, y, z);
      else
        pdb_ += fmt::format(FMT_COMPILE("{: <6}       {: <3} {: <3} {:1}{: >4}     {: >7.3f} {: >7.3f} {: >7.3f}\n"),
            v[offsets[0].pos], v[offsets[1].pos], v[offsets[2].pos], v[offsets[3].pos][0], v[offsets[4].pos], x, y, z);

      return true;
    }, atom_type);
  }


  bool process_struct_conf() noexcept
  {
    std::array<column, 11> offsets = 
    {{
      { "_struct_conf.pdbx_PDB_helix_id",            true,  -1, 3 },
      { "_struct_conf.beg_label_comp_id",            true,  -1, 3 },
      { "_struct_conf.beg_label_asym_id",            true,  -1, 1 },
      { "_struct_conf.beg_label_seq_id",             true,  -1, 4 },
      { "_struct_conf.pdbx_beg_PDB_ins_code",        false, -1, 1 },
      { "_struct_conf.end_label_comp_id",            true,  -1, 3 },
      { "_struct_conf.end_label_asym_id",            true,  -1, 1 },
      { "_struct_conf.end_label_seq_id",             true,  -1, 4 },
      { "_struct_conf.pdbx_end_PDB_ins_code",        false, -1, 1 },
      { "_struct_conf.pdbx_PDB_helix_class",         true,  -1, 2 },
      { "_struct_conf.pdbx_PDB_helix_length",        true,  -1, 4 }
    }};

    int entries = read_headers(offsets);
  
    if (entries == -1)
      return false;

    int i = 1;

    return read_data(entries, offsets, [&] (std::vector<std::string_view>& v)
    {
      char id_beg = ' ';
      char id_end = ' ';

      if (offsets[4].pos != -1 && v[offsets[4].pos] != "?" && v[offsets[4].pos].size() == 1)
        id_beg = v[offsets[4].pos][0];

      if (offsets[8].pos != -1 && v[offsets[8].pos] != "?" && v[offsets[8].pos].size() == 1)
        id_end = v[offsets[8].pos][0]; 

      pdb_ += fmt::format(FMT_COMPILE("HELIX  {: >3} {: >3} {: >3} {:1} {: >4}{:1} {: >3} {:1} {: >4}{:1}{: >2}{: >36}\n"),
           i, v[offsets[0].pos], v[offsets[1].pos], v[offsets[2].pos], v[offsets[3].pos],
              id_beg, v[offsets[5].pos], v[offsets[6].pos], v[offsets[7].pos],
              id_end, v[offsets[9].pos], v[offsets[10].pos]);

      ++i;

      if (i >= 1000) // max 3 digits
        return false;

      return true;
    });
  }


  bool process_struct_sheet() noexcept
  {
    std::array<column, 2> offsets =
    {{
      { "_struct_sheet.id",             true, -1, -1 },
      { "_struct_sheet.number_strands", true, -1, -1 }
    }};

    int entries = read_headers(offsets);
  
    if (entries == -1)
      return false;
  
    return read_data(entries, offsets, [&] (std::vector<std::string_view>& v)
    {
      int x;

      if (auto [p, ec] = std::from_chars(v[offsets[1].pos].begin(), v[offsets[1].pos].end(), x); ec != std::errc() || p != v[offsets[1].pos].end())
      {
        log_debug(FMT_COMPILE("from_chars error for number_strands: {}"), v[offsets[1].pos]);
        return false;
      }

      sheet_num_strands_[std::string(v[offsets[0].pos])] = x;

      return true;
    });
  }


  bool process_struct_sheet_order() noexcept
  {
    std::array<column, 3> offsets =
    {{
      { "_struct_sheet_order.sheet_id",   true, -1, -1 },
      { "_struct_sheet_order.range_id_2", true, -1, -1 },
      { "_struct_sheet_order.sense",      true, -1, -1 }
    }};

    int entries = read_headers(offsets);
  
    if (entries == -1)
      return false;
  
    return read_data(entries, offsets, [&] (std::vector<std::string_view>& v)
    {
      int r = (v[offsets[2].pos] == "anti-parallel") ? -1 : 1;
      
      sheet_order_[std::make_pair(std::string(v[offsets[0].pos]), std::string(v[offsets[1].pos]))] = r;

      return true;
    });
  }


  bool process_struct_sheet_range() noexcept
  {
    std::array<column, 10> offsets =
    {{
      { "_struct_sheet_range.id",                    true,  -1, 3 },
      { "_struct_sheet_range.sheet_id",              true,  -1, 3 },
      { "_struct_sheet_range.beg_label_comp_id",     true,  -1, 3 },
      { "_struct_sheet_range.beg_label_asym_id",     true,  -1, 1 },
      { "_struct_sheet_range.beg_label_seq_id",      true,  -1, 4 },
      { "_struct_sheet_range.pdbx_beg_PDB_ins_code", false, -1, 1 },
      { "_struct_sheet_range.end_label_comp_id",     true,  -1, 3 },
      { "_struct_sheet_range.end_label_asym_id",     true,  -1, 1 },
      { "_struct_sheet_range.end_label_seq_id",      true,  -1, 4 },
      { "_struct_sheet_range.pdbx_end_PDB_ins_code", false, -1, 1 }
    }};

    int entries = read_headers(offsets);

    if (entries == -1)
      return false;

    return read_data(entries, offsets, [&] (std::vector<std::string_view>& v)
    {
      auto num = sheet_num_strands_.find(v[offsets[1].pos]);

      if (num == sheet_num_strands_.end())
      {
        log_debug(FMT_COMPILE("unable to find: {}"), v[offsets[1].pos]);
        return false;
      }

      int dir = 0;
      auto lu = sheet_order_.find({std::string(v[offsets[1].pos]), std::string(v[offsets[0].pos])});

      if (lu != sheet_order_.end())
        dir = lu->second;

      char id_beg = ' ';
      char id_end = ' ';
      
      if (offsets[5].pos != -1 && v[offsets[5].pos] != "?" && v[offsets[5].pos].size() == 1)
        id_beg = v[offsets[5].pos][0];
  
      if (offsets[9].pos != -1 && v[offsets[9].pos] != "?" && v[offsets[9].pos].size() == 1)
        id_end = v[offsets[9].pos][0];

      pdb_ += fmt::format(FMT_COMPILE("SHEET  {: >3} {: >3}{: >2} {: >3} {:1}{: >4}{:1} {: >3} {:1}{: >4}{:1}{: >2}\n"),
            v[offsets[0].pos], v[offsets[1].pos], num->second,
            v[offsets[2].pos], v[offsets[3].pos], v[offsets[4].pos],
            id_beg, v[offsets[6].pos], v[offsets[7].pos],
            v[offsets[8].pos], id_end, dir);

      return true;
    });
  }


  bool process_loop() noexcept
  {
    using namespace magic_enum::bitwise_operators;

    for (auto line = getline(); !line.empty(); line = getline())
    {
      if (line.starts_with('#'))
        continue;

      auto pos = line.find('.');

      if (pos == std::string::npos)
        continue;

      line = line.substr(0, pos);

      if (line == "_atom_site" && ((options_ & options::ca_atoms)     != options::none ||
                                   (options_ & options::non_ca_atoms) != options::none ||
                                   (options_ & options::hetatm)       != options::none))
      {
        rewind();
        if (!process_atom_site())
          return false;

        continue;
      }

      if (line == "_struct_conf" && (options_ & options::helix) != options::none)
      {
        rewind();
        process_struct_conf();
        //if (!process_struct_conf())
          //return false;

        continue;
      }

      if (line == "_struct_sheet" && (options_ & options::sheet) != options::none)
      {
        rewind();
        if (!process_struct_sheet())
          return false;
  
        continue;
      }

      if (line == "_struct_sheet_order" && (options_ & options::sheet) != options::none)
      {
        rewind();

        if (!process_struct_sheet_order())
          return false;
  
        continue;
      }

      if (line == "_struct_sheet_range" && (options_ & options::sheet) != options::none)
      {
        rewind();
        if (!process_struct_sheet_range())
          return false;
  
        continue;
      }

      return skip_loop();
    }

    return true;
  }


private:

  
  options options_;

  const char* start_;
  const char* end_;
  const char* pos_;
  const char* prev_pos_; // the previous line pos

  std::string match_atom_ = "";

  std::string pdb_; // output

  // map of sheet_id -> num_strands

  std::map<std::string, int, std::less<>> sheet_num_strands_;

  // map of (sheet_id, strand_num) -> anti-parallel | parallel

  std::map<std::pair<std::string, std::string>, int> sheet_order_;

}; // class cif2pdb

} // namespace animol
