#pragma once

#include <string>
#include <string_view>
#include <cmath>
#include <cstring>
#include <vector>
#include <optional>
#include <fstream>

/*
  1. Using the 'cmap' table in the font, the client converts the character codes into a string of glyph indices.

  2. Using information in the GSUB table, the client modifies the resulting string, substituting positional or vertical glyphs, ligatures, or other alternatives as appropriate.

  3. Using positioning information in the GPOS table and baseline offset information in the BASE table, the client then positions the glyphs.

  4. Using design coordinates the client determines device-independent line breaks. Design coordinates are high-resolution and device-independent.

  5. Using information in the JSTF table, the client justifies the lines, if the user has specified such alignment.

  6. The operating system rasterizes the line of glyphs and renders the glyphs in device coordinates that correspond to the resolution of the output device.
*/

/* extend ttf to support atlases

  ATLS:

    u16 version
    u16 atlas count
    { u16 atlas type, u16 point_size, u32 atlas offset } atlas_offsets[atlas_count]

    atls_offset:
    u16 encoding_format
    u16 width
    u16 height
    u16 depth
    <data>

  ATLX:

    u16 version
    u32 atlas count
    u32 glyph_count
    { u16 atlas type, u32 glyphs offset } glyphs[atlas_count]

    offset:
    ASCENDING:
      u32 glyphid
      i32 bounds_l
      i32 bounds_b
      i32 bounds_r
      i32 bounds_t
      u16 atlas_l
      u16 atlas_b
      u16 atlas_r
      u16 atlas_t

*/


namespace plate {

constexpr std::uint32_t from_tag(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
  return (a << 24) | (b << 16) | (c << 8) | d;
};


class ttf_client {

public:


  struct bige_u32
  {
    unsigned char v[4];

    inline std::uint32_t get() const noexcept { return (v[0]<<24) | v[1]<<16 | v[2]<<8 | v[3]; };
    inline void set(std::uint32_t u) noexcept {
      v[0] = u>>24; v[1] = (u >> 16) & 255; v[2] = (u >> 8) & 255; v[3] = u & 255; };

    inline bool is_empty() const noexcept { return v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0; };

    inline bool operator< (const bige_u32& rhs) const noexcept { return get() <  rhs.get(); };
    inline bool operator<=(const bige_u32& rhs) const noexcept { return get() <= rhs.get(); };
    inline bool operator==(const bige_u32& rhs) const noexcept { return v[0] == rhs.v[0] && v[1] == rhs.v[1] && v[2] == rhs.v[2] && v[3] == rhs.v[3]; };
    inline bool operator>=(const bige_u32& rhs) const noexcept { return get() >= rhs.get(); };
    inline bool operator> (const bige_u32& rhs) const noexcept { return get() >  rhs.get(); };

    inline bool operator< (const std::uint32_t& rhs) const noexcept { return get() <  rhs; };
    inline bool operator<=(const std::uint32_t& rhs) const noexcept { return get() <= rhs; };
    inline bool operator==(const std::uint32_t& rhs) const noexcept { return get() == rhs; };
    inline bool operator>=(const std::uint32_t& rhs) const noexcept { return get() >= rhs; };
    inline bool operator> (const std::uint32_t& rhs) const noexcept { return get() >  rhs; };
  };


  struct bige_u16
  {
    unsigned char v[2];

    inline std::uint16_t get() const noexcept { return (v[0]<<8) | v[1]; };
    inline void set(std::uint16_t u) noexcept { v[0] = u>>8; v[1] = u & 255; };

    inline bool is_empty() const noexcept { return v[0] == 0 && v[1] == 0; };

    inline bool operator< (const bige_u16& rhs) const noexcept { return get() <  rhs.get(); };
    inline bool operator<=(const bige_u16& rhs) const noexcept { return get() <= rhs.get(); };
    inline bool operator==(const bige_u16& rhs) const noexcept { return v[0] == rhs.v[0] && v[1] == rhs.v[1]; };
    inline bool operator>=(const bige_u16& rhs) const noexcept { return get() >= rhs.get(); };
    inline bool operator> (const bige_u16& rhs) const noexcept { return get() >  rhs.get(); };

    inline bool operator< (const std::uint16_t& rhs) const noexcept { return get() <  rhs; };
    inline bool operator<=(const std::uint16_t& rhs) const noexcept { return get() <= rhs; };
    inline bool operator==(const std::uint16_t& rhs) const noexcept { return get() == rhs; };
    inline bool operator>=(const std::uint16_t& rhs) const noexcept { return get() >= rhs; };
    inline bool operator> (const std::uint16_t& rhs) const noexcept { return get() >  rhs; };
  };


  struct big2_i16
  {
    unsigned char v[2];

    inline std::int16_t get() const noexcept { return (v[0]<<8) | v[1]; };
    inline void set(std::int16_t u) noexcept { v[0] = u>>8; v[1] = u & 255; };

    inline bool is_empty() const noexcept { return v[0] == 0 && v[1] == 0; };

    inline bool operator< (const big2_i16& rhs) const noexcept { return get() <  rhs.get(); };
    inline bool operator<=(const big2_i16& rhs) const noexcept { return get() <= rhs.get(); };
    inline bool operator==(const big2_i16& rhs) const noexcept { return v[0] == rhs.v[0] && v[1] == rhs.v[1]; };
    inline bool operator>=(const big2_i16& rhs) const noexcept { return get() >= rhs.get(); };
    inline bool operator> (const big2_i16& rhs) const noexcept { return get() >  rhs.get(); };

    inline bool operator< (const std::int16_t& rhs) const noexcept { return get() <  rhs; };
    inline bool operator<=(const std::int16_t& rhs) const noexcept { return get() <= rhs; };
    inline bool operator==(const std::int16_t& rhs) const noexcept { return get() == rhs; };
    inline bool operator>=(const std::int16_t& rhs) const noexcept { return get() >= rhs; };
    inline bool operator> (const std::int16_t& rhs) const noexcept { return get() >  rhs; };
  };


  enum class ATLAS_TYPE: std::uint16_t { None = 0, HardMask = 1, SoftMask = 2, SDF = 3, MSDF = 4 };


  ttf_client() {};

  ttf_client(unsigned char* start, unsigned char* end) :
    start_(start),
    end_(end)
  {
  };


  ttf_client(std::string path)
  {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer_.resize(size);

    if (file.read(reinterpret_cast<char*>(buffer_.data()), size))
    {
      start_ = &buffer_[0];
      end_ = start_ + size;

      prepare();
    }
  };


  bool prepare(unsigned char* start, unsigned char* end)
  {
    start_ = start;
    end_ = end;

    return prepare();
  };


  bool prepare(std::byte* start, std::byte* end)
  {
    start_ = reinterpret_cast<unsigned char*>(start);
    end_ = reinterpret_cast<unsigned char*>(end);

    return prepare();
  };


  bool prepare()
  {
    auto c = start_;

    if (end_ < c + 4 + 2 + 2 + 2 + 2) return error("minimum header size"); // minimum header size

    c += 2 + 2;

    auto num_tables     = read_u16(c);

    c += 2 + 2 + 2;

    if (end_ < c + (4 + 4 + 4 + 4) * num_tables) return error("table size"); // minimum table size

    for (std::uint32_t i = 0; i < num_tables; ++i)
    {
      table_entry e;

      auto tag   = read_u32(c);

      c+=4;

      e.offset   = read_u32(c);
      e.length   = read_u32(c);

      if (start_ + e.offset + e.length > end_) return error("offset+length to big");

      switch (tag)
      {
        case from_tag('c','m','a','p'):  cmap_ = e; break;
        case from_tag('h','e','a','d'):  head_ = e; break;
        case from_tag('m','a','x','p'):  maxp_ = e; break;
        case from_tag('n','a','m','e'):  name_ = e; break;
        case from_tag('h','h','e','a'):  hhea_ = e; break;
        case from_tag('h','m','t','x'):  hmtx_ = e; break;
        case from_tag('v','h','e','a'):  vhea_ = e; break;
        case from_tag('v','m','t','x'):  vmtx_ = e; break;
        case from_tag('g','l','y','f'):  glyf_ = e; break;
        case from_tag('l','o','c','a'):  loca_ = e; break;
        case from_tag('G','S','U','B'):  GSUB_ = e; break;
        case from_tag('G','P','O','S'):  GPOS_ = e; break;
        case from_tag('C','F','F',' '):  CFF_  = e; break;
        case from_tag('p','o','s','t'):  post_ = e; break;
        case from_tag('G','D','E','F'):  GDEF_ = e; break;
        case from_tag('O','S','/','2'):  OS2_  = e; break;

        case from_tag('A','T','L','S'):  ATLS_  = e; break;
        case from_tag('A','T','L','X'):  ATLX_  = e; break;
      }
    }

    if (head_.offset && !prepare_head()) return error("Unable to prepare head");
    if (hhea_.offset && !prepare_hhea()) return error("Unable to prepare hhea");
    if (maxp_.offset && !prepare_maxp()) return error("Unable to prepare maxp");
    if (hmtx_.offset && !prepare_hmtx()) return error("Unable to prepare hmtx");
    if (cmap_.offset && !prepare_cmap()) return error("Unable to prepare cmap");
    if (GPOS_.offset && !prepare_GPOS()) return error("Unable to prepare GPOS");
    if (ATLX_.offset && !prepare_ATLX()) return error("Unable to prepare ATLX");

    return true;
  };


  // client use interfaces

  inline std::uint16_t get_number_of_metrics() const noexcept
  {
    return hheai_.metric_count;
  };


  inline std::uint16_t get_number_of_glyphs() const noexcept
  {
    return maxpi_.glyph_count;
  };


  std::uint32_t get_advance_x(std::uint32_t glyph_id) const noexcept
  {
    auto c = start_ + hmtx_.offset;

    c += (2 + 2) * glyph_id;

    return read_u16(c);
  };


  inline std::uint16_t get_units_per_em() const noexcept { return headi_.units_per_em; };
  inline  std::int16_t get_xMin()         const noexcept { return headi_.xMin;         };
  inline  std::int16_t get_yMin()         const noexcept { return headi_.yMin;         };
  inline  std::int16_t get_xMax()         const noexcept { return headi_.xMax;         };
  inline  std::int16_t get_yMax()         const noexcept { return headi_.yMax;         };


  struct boundings
  {
    std::int32_t bounds_l;
    std::int32_t bounds_b;
    std::int32_t bounds_r;
    std::int32_t bounds_t;

    float atlas_l;
    float atlas_b;
    float atlas_r;
    float atlas_t;
  };


  bool get_bounding(std::uint32_t glyph_id, boundings& b)
  {
    if (!ATLX_.offset) return false;

    if (glyph_id > ATLXi_.glyph_count) return false;

    auto c = start_ + ATLX_.offset + 8;

    for (std::int16_t i = 0; i < ATLXi_.atlas_count; ++i)
    {
      auto atlas_type    = read_u16(c);
      auto glyphs_offset = read_u32(c);

      auto d = start_ + ATLX_.offset + glyphs_offset;

      // jump straight to the correct glyph - assumes all glyph ids are used.

      d += (4 + 4 + 4 + 4 + 4 + 2 + 2 + 2 + 2) * (glyph_id - 1);

      d += 4; // skip glyph id - todo check?

      b.bounds_l = read_i32(d);
      b.bounds_b = read_i32(d);
      b.bounds_r = read_i32(d);
      b.bounds_t = read_i32(d);
      b.atlas_l  = read_u16(d) + 0.5f;
      b.atlas_b  = read_u16(d) + 0.5f;
      b.atlas_r  = read_u16(d) + 0.5f;
      b.atlas_t  = read_u16(d) + 0.5f;

      return true;
    }

    return false;
  };


  bool get_atlas(ATLAS_TYPE& type, std::uint16_t& point_size,
                 std::uint16_t& width, std::uint16_t& height, std::uint16_t& depth,
                 std::byte*& data)
  {
    if (!ATLS_.offset) return false;

    auto c = start_ + ATLS_.offset;

    auto version     = read_u16(c);
    auto atlas_count = read_u16(c);

    for (std::int16_t i = 0; i < atlas_count; ++i)
    {
      type       = static_cast<ATLAS_TYPE>(read_u16(c));
      point_size = read_u16(c);

      auto atlas_offset = read_u32(c);

      auto d = start_ + ATLS_.offset + atlas_offset;

      auto encoding_format = read_u16(d);

      width  = read_u16(d);
      height = read_u16(d);
      depth  = read_u16(d);
      data   = reinterpret_cast<std::byte*>(d);

      return true; // just take the first atlas for the mo
    }

    return false;
  };


  std::pair<std::int16_t, std::int16_t> get_kern_unicode(std::uint32_t lhs, std::uint32_t rhs)
  {
    auto lhs_glyph_id = static_cast<std::uint16_t>(get_glyph_id(lhs));
    auto rhs_glyph_id = static_cast<std::uint16_t>(get_glyph_id(rhs));

    return get_kern_glyph_id(lhs_glyph_id, rhs_glyph_id);
  };


  std::pair<std::int16_t, std::int16_t> get_kern_glyph_id(std::uint16_t lhs_glyph_id, std::uint16_t rhs_glyph_id)
  {
    if (GPOS_pair1_to_use_)
    {
      auto r = get_kern_glyph_id_1(lhs_glyph_id, rhs_glyph_id);
      if (r.first != 0) return r;
    }

    if (GPOS_pair2_to_use_)
    {
      return get_kern_glyph_id_2(lhs_glyph_id, rhs_glyph_id);
    }

    return {0,0};
  };


  std::pair<std::int16_t, std::int16_t> get_kern_glyph_id_2(std::uint16_t lhs_glyph_id, std::uint16_t rhs_glyph_id)
  {
    auto lower = std::lower_bound(GPOS2_.coverage_table, GPOS2_.coverage_table + GPOS2_.coverage_glyph_count,
                                                                                                  lhs_glyph_id);

    auto offset = std::distance(GPOS2_.coverage_table, lower);

    if (offset >= GPOS2_.coverage_glyph_count) return {0,0}; // not in coverage

    if ((*lower).get() != lhs_glyph_id) return {0,0}; // not in coverage

    // it is in coverage so check if there is a kern pair defined by classes

    // get index of lhs

    auto lower1 = std::lower_bound(GPOS2_.class1_data, GPOS2_.class1_data + GPOS2_.class1_count, lhs_glyph_id);

    auto offset1 = std::distance(GPOS2_.class1_data, lower1);

    if (offset1 >= GPOS2_.class1_count) return {0,0}; // should never happen for first glyph
    if ((*lower1).start_glyph_id.get() < lhs_glyph_id) return {0,0}; // should never happen for first glyph

    std::uint16_t class1_id = (*lower1).class_id.get();

    // get index of rhs

    auto lower2 = std::lower_bound(GPOS2_.class2_data, GPOS2_.class2_data + GPOS2_.class2_count, rhs_glyph_id);

    auto offset2 = std::distance(GPOS2_.class2_data, lower2);

    if (offset2 >= GPOS2_.class2_count) return {0,0}; // not found rhs glyph
    if ((*lower2).start_glyph_id.get() < rhs_glyph_id) return {0,0}; // not found rhs glyph

    std::uint16_t class2_id = (*lower2).class_id.get();

    // now we know the classes can extract from 2d array table

    std::uint32_t lookup_offset = (class1_id * GPOS2_.classdef2_count) + class2_id;

    big2_i16* lu = GPOS2_.lookup_table + lookup_offset;

    return { lu->get(), 0 };
  };


  std::pair<std::int16_t, std::int16_t> get_kern_glyph_id_1(std::uint16_t lhs_glyph_id, std::uint16_t rhs_glyph_id)
  {
    auto lower = std::lower_bound(GPOS1_.coverage_table, GPOS1_.coverage_table + GPOS1_.coverage_glyph_count,
                                                                                                  lhs_glyph_id);

    auto offset = std::distance(GPOS1_.coverage_table, lower);

    if (offset >= GPOS1_.coverage_glyph_count) return {0,0}; // not in coverage

    if ((*lower).get() != lhs_glyph_id) return {0,0}; // not in coverage

    // it is in coverage so check the subtable for the rhs

    auto pair_set_offset = GPOS1_.pair_offset_table[offset].get();

    auto v = reinterpret_cast<bige_u16*>(start_ + GPOS_pair1_to_use_ + pair_set_offset);

    auto pair_value_count = v->get();

    GPOS1::pv_t* data = reinterpret_cast<GPOS1::pv_t*>(start_ + GPOS_pair1_to_use_ + pair_set_offset + 2);

    auto lower2 = std::lower_bound(data, data + pair_value_count, rhs_glyph_id);

    auto offset2 = std::distance(data, lower2);

    if (offset2 >= pair_value_count) return {0,0}; // not in rhs
    if ((*lower2).second_glyph_id.get() != rhs_glyph_id) return {0,0}; // not in rhs

    return { (*lower2).value_record_1.get(), 0 };
  };


  std::uint32_t get_glyph_id(std::uint32_t u) // converts a unicode id to a glyph id
  {
    if (cmap12_to_use_.offset) return get_glyph_id_cmap12(u);
    if (cmap4_to_use_.offset)  return get_glyph_id_cmap4(u);

    return 0;
  };


  std::uint32_t get_glyph_id_cmap4(std::uint32_t u) // converts a unicode id to a glyph id
  {
    if (!cmap4_to_use_.offset) return 0;

    auto lower = std::lower_bound(cmap4_.end_code, cmap4_.end_code + cmap4_.segcount, u);

    // check entry is in range

    auto offset = std::distance(cmap4_.end_code, lower);

    if (cmap4_.start_code[offset] > u) return 0; // not in range

    if (cmap4_.id_range_offset[offset].is_empty()) // use delta
    {
      return u + cmap4_.id_delta[offset].get();
    }
    else // use glyph offset
    {
      std::uint16_t o = cmap4_.id_range_offset[offset].get()/2 + offset - cmap4_.segcount + u - cmap4_.start_code[offset].get();
      return cmap4_.ids[o].get();
    }
  };


  std::uint32_t get_glyph_id_cmap12(std::uint32_t u)
  {
    if (!cmap12_to_use_.offset) return 0;

    auto lower = std::lower_bound(cmap12_.data, cmap12_.data + cmap12_.num_groups, u);

    auto offset = std::distance(cmap12_.data, lower);

    if (offset >= cmap12_.num_groups) return 0;

    if ((*lower).start_char > u) return 0;

    return u - (*lower).start_char.get() + (*lower).start_glyph.get();
  };


private:


  bool error(std::string s)
  {
    printf("error: %s\n", s.c_str());
    return false;
  };


  bool prepare_ATLX()
  {
    auto c = start_ + ATLX_.offset;

    auto block_end = c + ATLX_.length;
  
    if (c + 2 + 2 + 2 > block_end) return error("ATLX header");

    auto version     = read_u16(c);
    auto atlas_count = read_u16(c);
    auto glyph_count = read_u32(c);

    if (c + 2 + 2 + 2 + ((2 + 4) * atlas_count) > block_end) return error("ATLS header");

    ATLXi_.atlas_count = atlas_count;
    ATLXi_.glyph_count = glyph_count;

    return true;
  };


  bool prepare_head()
  {
    auto c = start_ + head_.offset;

    c += 4 + 4 + 4 + 4 + 2;

    if (c + 2 >= end_)
      return false;

    headi_.units_per_em = read_u16(c);

    c += 8 + 8;

    if (c + 2 + 2 + 2 + 2 >= end_)
      return false;

    headi_.xMin = read_i16(c);
    headi_.yMin = read_i16(c);
    headi_.xMax = read_i16(c);
    headi_.yMax = read_i16(c);

    return true;
  };


  bool prepare_hhea()
  {
    auto c = start_ + hhea_.offset;

    c += 4 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 2+2+2+2 + 2;

    if (c + 2 >= end_) return false;

    hheai_.metric_count = read_u16(c);

    return true;
  };


  bool prepare_maxp()
  {
    auto c = start_ + maxp_.offset;

    c += 4;

    if (c + 2 >= end_) return false;

    maxpi_.glyph_count = read_u16(c);

    return true;
  };


  bool prepare_hmtx()
  {
    // check all glyph ids fit in hmtx entry

    if (hmtx_.length <= (2 + 2) * get_number_of_glyphs()) return true;

    return false;
  };


  bool prepare_cmap()
  {
    auto c = start_ + cmap_.offset;

    auto block_end = c + cmap_.length;
  
    if (c + 2 + 2 > block_end) return error("cmap header");

    c += 2;
    auto num_tables = read_u16(c);

    if (c + (8 * num_tables) > block_end) return error("cmap header too many tables");

    struct te
    {
      std::uint16_t platform_id;
      std::uint16_t encoding_id;
      std::uint32_t subtable_offset;
    };

    for (std::int16_t i = 0; i < num_tables; ++i)
    {
      te t;

      t.platform_id     = read_u16(c);
      t.encoding_id     = read_u16(c);
      t.subtable_offset = read_u32(c);

      auto d = start_ + cmap_.offset + t.subtable_offset;

      auto format = read_u16(d);

      if (t.platform_id == 0 && t.encoding_id == 3 && format == 4)
      {
        cmap4_to_use_ = { 0, cmap_.offset + t.subtable_offset, cmap_.length };

        // setup for quick lookups

        bige_u16* v = reinterpret_cast<bige_u16*>(start_ + cmap4_to_use_.offset + 2 + 2 + 2);
        cmap4_.segcount = v->get() / 2;

        cmap4_.end_code        = v + 4;
        cmap4_.start_code      = cmap4_.end_code + cmap4_.segcount + 1;
        cmap4_.id_delta        = reinterpret_cast<big2_i16*>(cmap4_.start_code + cmap4_.segcount);
        cmap4_.id_range_offset = cmap4_.start_code + cmap4_.segcount + cmap4_.segcount;
        cmap4_.ids             = cmap4_.id_range_offset + cmap4_.segcount;
      }
      if (t.platform_id == 0 && format == 12)
      {
        cmap12_to_use_ = { 0, cmap_.offset + t.subtable_offset, cmap_.length };

        // setup for quick lookups

        bige_u32* v = reinterpret_cast<bige_u32*>(start_ + cmap12_to_use_.offset + 4 + 4 + 4);

        cmap12_.num_groups = v->get();

        cmap12_.data = reinterpret_cast<cmap12::tb*>(start_ + cmap12_to_use_.offset + 4 + 4 + 4 + 4);
      }
    }

    return true;
  };


  bool prepare_GPOS()
  {
    auto c = start_ + GPOS_.offset;

    if (c + 2 + 2 + 2 + 2 + 2 > end_) return error("GPOS header");

    c += 2;
    c += 2;

    c += 2;
    c += 2;

    auto lookup_list_offset  = read_u16(c);

    if (start_ + GPOS_.offset + lookup_list_offset + 2 > end_) return error("GPOS out of bounds");

    // only process lookup_list atm

    auto block_end = start_ + GPOS_.offset + GPOS_.length;

    c = start_ + GPOS_.offset + lookup_list_offset;

    auto count = read_u16(c);

    if (c + (2 * count) > block_end) return error("lookup list out of bounds");

    std::uint16_t lookup[count];

    for (std::uint16_t i = 0; i < count; ++i)
    {
      lookup[i] = read_u16(c);

      auto d = start_ + GPOS_.offset + lookup_list_offset + lookup[i];

      if (d + 6 > block_end) return error("block size");

      auto type            = read_u16(d);
      auto flag            = read_u16(d);
      auto sub_table_count = read_u16(d);

      if (d + (2 * sub_table_count) > block_end) return error("subtable out of bounds");

      if (type == 2) // Pair adjustment
      {
        std::uint16_t sub_table[sub_table_count];

        for (std::uint16_t j = 0; j < sub_table_count; ++j)
        {
          sub_table[j] = read_u16(d);

          auto e = start_ + GPOS_.offset + lookup_list_offset + lookup[i] + sub_table[j];

          auto pos_format      = read_u16(e);

          //auto coverage_offset = read_u16(e);
          //auto value_format1   = read_u16(e);
          //auto value_format2   = read_u16(e);

          if (pos_format == 1)
          {
            GPOS_pair1_to_use_ = GPOS_.offset + lookup_list_offset + lookup[i] + sub_table[j];

            // prepare quick GPOS1 lookups

            bige_u16* v = reinterpret_cast<bige_u16*>(start_ + GPOS_pair1_to_use_ + 2);

            auto coverage_offset = v->get();
            ++v;
            GPOS1_.value_format1 = v->get();
            ++v;
            GPOS1_.value_format2 = v->get();
            ++v;
            GPOS1_.pair_set_count = v->get();
            ++v;

            v = reinterpret_cast<bige_u16*>(start_ + GPOS_pair1_to_use_ + coverage_offset);

            GPOS1_.coverage_format = v->get();
            ++v;
            GPOS1_.coverage_glyph_count = v->get();
            ++v;

            GPOS1_.coverage_table = v;

            GPOS1_.pair_offset_table = reinterpret_cast<bige_u16*>(start_ + GPOS_pair1_to_use_ + 2 + 2 + 2 + 2 + 2);
          }

          if (pos_format == 2)
          {
            GPOS_pair2_to_use_ = GPOS_.offset + lookup_list_offset + lookup[i] + sub_table[j];

            // prepare quick GPOS2 lookups

            bige_u16* v = reinterpret_cast<bige_u16*>(start_ + GPOS_pair2_to_use_ + 2);

            auto coverage_offset = v->get();
            ++v;
            GPOS2_.value_format1 = v->get();
            ++v;
            GPOS2_.value_format2 = v->get();
            ++v;

            bige_u16* v2 = reinterpret_cast<bige_u16*>(start_ + GPOS_pair2_to_use_ + coverage_offset);

            GPOS2_.coverage_format      = v2->get();
            ++v2;
            GPOS2_.coverage_glyph_count = v2->get();
            ++v2;

            GPOS2_.coverage_table = v2;

            auto classdef1_offset = v->get();
            ++v;
            auto classdef2_offset = v->get();
            ++v;
            GPOS2_.classdef1_count = v->get();
            ++v;
            GPOS2_.classdef2_count = v->get();
            ++v;

            v2 = reinterpret_cast<bige_u16*>(start_ + GPOS_pair2_to_use_ + classdef1_offset);

            GPOS2_.classdef1_format = v2->get();
            ++v2;
            GPOS2_.class1_count = v2->get();

            GPOS2_.class1_data = reinterpret_cast<GPOS2::ce*>(start_ + GPOS_pair2_to_use_ + classdef1_offset + 2 + 2);

            v2 = reinterpret_cast<bige_u16*>(start_ + GPOS_pair2_to_use_ + classdef2_offset);

            GPOS2_.classdef2_format = v2->get();
            ++v2;
            GPOS2_.class2_count = v2->get();

            GPOS2_.class2_data = reinterpret_cast<GPOS2::ce*>(start_ + GPOS_pair2_to_use_ + classdef2_offset + 2 + 2);

            GPOS2_.lookup_table = reinterpret_cast<big2_i16*>(start_ + GPOS_pair2_to_use_ + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 2);
          }
        }
      }
    }

    return true;
  };



  std::uint32_t read_u32(unsigned char*& c) const noexcept
  {
    std::uint32_t r = ((*c++) << 24);
    r |= (*c++) << 16;
    r |= (*c++) << 8;
    r |= (*c++);

    return r;
  }


  std::int32_t read_i32(unsigned char*& c) const noexcept
  {
    std::int32_t r = ((*c++) << 24);
    r |= (*c++) << 16;
    r |= (*c++) << 8;
    r |= (*c++);

    return r;
  }


  std::uint16_t read_u16(unsigned char*& c) const noexcept
  {
    std::uint16_t r = ((*c++) << 8);
    r |= (*c++);

    return r;
  }


  std::int16_t read_i16(unsigned char*& c) const noexcept
  {
    std::int16_t r = ((*c++) << 8);
    r |= (*c++);

    return r;
  }


  unsigned char* start_{nullptr};
  unsigned char* end_{nullptr};

  struct table_entry
  {
    std::uint32_t checksum;
    std::uint32_t offset;
    std::uint32_t length;
  };

  table_entry cmap_{};
  table_entry head_{};
  table_entry maxp_{};
  table_entry name_{};
  table_entry hhea_{};
  table_entry hmtx_{};
  table_entry vhea_{};
  table_entry vmtx_{};
  table_entry loca_{};
  table_entry glyf_{};
  table_entry GSUB_{};
  table_entry GPOS_{};
  table_entry CFF_{};
  table_entry post_{};
  table_entry GDEF_{};
  table_entry OS2_{};
  table_entry ATLS_{};
  table_entry ATLX_{};

  table_entry cmap4_to_use_{};
  table_entry cmap12_to_use_{};
  std::uint32_t GPOS_pair1_to_use_{};
  std::uint32_t GPOS_pair2_to_use_{};

  // data used to perform quick cmap4 lookups

  struct cmap4
  {
    std::uint16_t segcount;
    bige_u16*     end_code;
    bige_u16*     start_code;
    big2_i16*     id_delta;
    bige_u16*     id_range_offset;
    bige_u16*     ids;
  };

  cmap4 cmap4_{};

  // data used to perform quick cmap12 lookups

  struct cmap12
  {
    struct tb
    {
      bige_u32 start_char;
      bige_u32 end_char;
      bige_u32 start_glyph;
  
      inline bool operator< (const bige_u32& rhs) const noexcept { return end_char <  rhs; };
      inline bool operator<=(const bige_u32& rhs) const noexcept { return end_char <= rhs; };
      inline bool operator==(const bige_u32& rhs) const noexcept { return end_char == rhs; };
      inline bool operator>=(const bige_u32& rhs) const noexcept { return end_char >= rhs; };
      inline bool operator> (const bige_u32& rhs) const noexcept { return end_char >  rhs; };
  
      inline bool operator< (const std::uint32_t& rhs) const noexcept { return end_char <  rhs; };
      inline bool operator<=(const std::uint32_t& rhs) const noexcept { return end_char <= rhs; };
      inline bool operator==(const std::uint32_t& rhs) const noexcept { return end_char == rhs; };
      inline bool operator>=(const std::uint32_t& rhs) const noexcept { return end_char >= rhs; };
      inline bool operator> (const std::uint32_t& rhs) const noexcept { return end_char >  rhs; };
    };

    std::uint32_t num_groups;
    tb* data;
  };

  cmap12 cmap12_{};
    
  // data used to perform quick GPOS format 1 lookups

  struct GPOS1
  {
    struct pv_t
    {
      bige_u16 second_glyph_id;
      big2_i16 value_record_1;
  
      inline bool operator< (const std::uint16_t& rhs) const noexcept { return second_glyph_id <  rhs; };
      inline bool operator<=(const std::uint16_t& rhs) const noexcept { return second_glyph_id <= rhs; };
      inline bool operator==(const std::uint16_t& rhs) const noexcept { return second_glyph_id == rhs; };
      inline bool operator>=(const std::uint16_t& rhs) const noexcept { return second_glyph_id >= rhs; };
      inline bool operator> (const std::uint16_t& rhs) const noexcept { return second_glyph_id >  rhs; };
    };

    std::uint16_t value_format1;
    std::uint16_t value_format2;
    std::uint16_t pair_set_count;
    std::uint16_t coverage_format;
    std::uint16_t coverage_glyph_count;
    bige_u16*     coverage_table;

    bige_u16*     pair_offset_table;
  };

  GPOS1 GPOS1_{};

  // data used to perform quick GPOS format 2 lookups

  struct GPOS2
  {
    struct ce
    {
      bige_u16 start_glyph_id;
      bige_u16 end_glyph_id;
      bige_u16 class_id;
  
      inline bool operator< (const std::uint16_t& rhs) const noexcept { return end_glyph_id <  rhs; };
      inline bool operator<=(const std::uint16_t& rhs) const noexcept { return end_glyph_id <= rhs; };
      inline bool operator==(const std::uint16_t& rhs) const noexcept { return end_glyph_id == rhs; };
      inline bool operator>=(const std::uint16_t& rhs) const noexcept { return end_glyph_id >= rhs; };
      inline bool operator> (const std::uint16_t& rhs) const noexcept { return end_glyph_id >  rhs; };
    };

    std::uint16_t value_format1;
    std::uint16_t value_format2;

    std::uint16_t coverage_format;
    std::uint16_t coverage_glyph_count;
    bige_u16*     coverage_table;

    std::uint16_t classdef1_count;
    std::uint16_t classdef2_count;

    std::uint16_t classdef1_format;
    std::uint16_t class1_count;
    ce*           class1_data;

    std::uint16_t classdef2_format;
    std::uint16_t class2_count;
    ce*           class2_data;

    big2_i16*     lookup_table;
  };

  GPOS2 GPOS2_{};

  // data used to perform quick ATLX lookups

  struct ATLX
  {
    std::uint32_t atlas_count;
    std::uint32_t glyph_count;
  };

  ATLX ATLXi_{};

  // data used to perform quick head lookups

  struct head
  {
    std::uint16_t units_per_em;
    std::int16_t xMin, yMin, xMax, yMax;
  };

  head headi_{};

  // data used to perform quick hhea lookups

  struct hhea
  {
    std::uint16_t metric_count;
  };

  hhea hheai_{};

  // data used to perform quick maxp lookups

  struct maxp
  {
    std::uint16_t glyph_count;
  };

  maxp maxpi_{};

  std::uint16_t version_major_ = 0;
  std::uint16_t version_minor_ = 0;

  std::vector<unsigned char> buffer_;

}; // class ttf_client

} // namespace plate
