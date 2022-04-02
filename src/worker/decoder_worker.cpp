#include "plate.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include "visual.hpp"
#include "cif2pdb.hpp"


extern "C" {

extern void vertex_free();
extern void vertex_clear();

extern int molauto (int argc, char *argv[]);
extern int molscript (int argc, char *argv[]);

#define OPTION_COLOR      1
#define OPTION_INTERLEAVE 2

extern int compact(char** cdata, int options);


float fast_float_c(const char* s)
{
  float f;

  if (animol::string_data::parse_float(f, std::string_view{s, strlen(s)}))
    return f;

  //printf("bad fast_float: >%s<\n", s);
  return 0;
}


// basic check to see if a file is in PDBx/mmCIF format
// currently just check it starts with data_

bool is_cif(std::span<const std::byte> data)
{
  if (data.size() < 5)
    return false;

  return std::memcmp(data.data(), "data_", 5) == 0;
}


struct fp
{
  short vert[3];
  short norm[3];
  unsigned char col[4];
};


void save_to_file(std::span<const char> data, std::string filename)
{
  auto f = fopen(filename.c_str(), "w");
  fwrite(data.data(), 1, data.size(), f);
  fclose(f);
}

void save_bytes_to_file(std::span<std::byte> data, std::string filename)
{
  auto f = fopen(filename.c_str(), "w");
  fwrite(reinterpret_cast<char*>(data.data()), 1, data.size(), f);
  fclose(f);
}


void read_from_file(std::vector<char>& data, std::string filename)
{
  auto f = fopen(filename.c_str(), "r");

  fseek(f, 0, SEEK_END);
  auto fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  data.resize(fsize);

  fread(data.data(), 1, data.size(), f);
  fclose(f);
}


void end_worker(char* data, int size)
{
  emscripten_worker_respond(nullptr, 0);
}


/* default script generated is cartoon. for others create our script:

title ""

plot
  read mol "/i.pdb";
  transform atom * by centre position atom *;
  set segments 8;
  bonds atom *;
end_plot

*/

void do_script()
{
  //const char *argv[] = { "molauto", "-cpk", "-out", "/i.script", "/i.pdb" }; // -stick
  const char *argv[] = { "molauto", "-out", "/i.script", "/i.pdb" };
  
  molauto(sizeof(argv) / sizeof(argv[0]), const_cast<char**>(argv));
  
  std::vector<char> script_file;
  
  read_from_file(script_file, "/i.script");
  
  emscripten_worker_respond(script_file.data(), script_file.size());
}


// data is the url of the pdb file to download and auto script

void script(char* data, int size)
{
  using namespace magic_enum::bitwise_operators;

  std::string s(data, size);

  log_debug(FMT_COMPILE("scipt: {}"), s);

  auto h = plate::async::request(s, "GET", "", [s] (std::uint32_t handle, plate::data_store&& d)
  {
    if (is_cif(d.span()))
    {
      animol::cif2pdb converter(d.span(), animol::cif2pdb::options::ca_atoms | animol::cif2pdb::options::sheet | animol::cif2pdb::options::helix);

      auto r = converter.convert();

      if (r)
      {
        //log_debug(FMT_COMPILE("script converted cif to: {}"), *r);
        save_to_file(std::span<const char>(r->data(), r->size()), "/i.pdb");
      }
      else
      {
        log_debug("script failed to convert cif");
        emscripten_worker_respond(nullptr, 0);
        return;
      }
    }
    else
      save_bytes_to_file(d.span(), "/i.pdb");

    do_script();

  }, [] (std::uint32_t handle, int error_code, std::string error_msg)
  {
    log_debug(FMT_COMPILE("failed to download, error_code: {} msg: {}"), error_code, error_msg);

    emscripten_worker_respond(nullptr, 0);
  },
  {});
}


// data is the pdb file contents

void script_with(char* data, int size)
{
  save_to_file(std::span<char>(data, size), "/i.pdb");
  
  do_script();
}


void do_decode(int options)
{
  vertex_clear();

  const char *argv[] = { "molscript", "-vertex", "-s", "-in", "/i.script" };
    
  molscript(sizeof(argv) / sizeof(argv[0]), const_cast<char**>(argv));

  char* cdata = nullptr;
  int csize = compact(&cdata, options);

  emscripten_worker_respond(cdata, csize);

  free(cdata);
}


// data is: size_of_script, script_contents, pdb_url file to download and decode

void decode_url(char* data, int size, int options)
{
  using namespace magic_enum::bitwise_operators;

  if (size < 4)
  {
    log_debug(FMT_COMPILE("decode_url: size too small: {}"), size);
    emscripten_worker_respond(nullptr, 0);
  }

  std::uint32_t script_sz;
  std::memcpy(&script_sz, data, 4);

  if (script_sz + 4 >= size)
  {
    log_debug(FMT_COMPILE("decode_url: size too small for script and url: {}"), size);
    emscripten_worker_respond(nullptr, 0);
  }

  std::span<char> script(data + 4, script_sz);

  std::string url(data + 4 + script_sz, size - (script_sz + 4));

  save_to_file(script, "i.script");

  auto h = plate::async::request(url, "GET", "", [url, options] (std::uint32_t handle, plate::data_store&& d)
  {
    if (is_cif(d.span()))
    {
      animol::cif2pdb converter(d.span(), animol::cif2pdb::options::ca_atoms | animol::cif2pdb::options::sheet | animol::cif2pdb::options::helix);

      auto r = converter.convert();

      if (r)
      {
        //log_debug(FMT_COMPILE("decode_url converted cif to: {}"), *r);
        save_to_file(std::span<const char>(r->data(), r->size()), "/i.pdb");
      }
      else
      {
        log_debug("decode_url failed to convert cif");
        emscripten_worker_respond(nullptr, 0);
        return;
      }
    }
    else
      save_bytes_to_file(d.span(), "/i.pdb");

    do_decode(options);

  }, [] (std::uint32_t handle, int error_code, std::string error_msg)
  {
    log_debug(FMT_COMPILE("failed to download, error_code: {} msg: {}"), error_code, error_msg);
  
    emscripten_worker_respond(nullptr, 0);
  },
  {});
}


// data is: size_of_script, script_data, pdb_url

void decode_url_color_interleaved(char* data, int size)
{
  decode_url(data, size, OPTION_COLOR | OPTION_INTERLEAVE);
}


void decode_url_color_non_interleaved(char* data, int size)
{
  decode_url(data, size, OPTION_COLOR);
}


void decode_url_no_color(char* data, int size)
{
  decode_url(data, size, 0);
}


// data is: size_of_script, script_contents, pdb file contents

void decode_contents(char* data, int size, int options)
{
  if (size < 4)
  {
    log_debug(FMT_COMPILE("decode_contents: size too small: {}"), size);
    emscripten_worker_respond(nullptr, 0);
  }
    
  std::uint32_t script_sz;
  std::memcpy(&script_sz, data, 4);
      
  if (script_sz + 4 >= size)
  {
    log_debug(FMT_COMPILE("decode_contents: size too small for script and pdb contents: {} script_size: {}"),
                                                                                                size, script_sz);
    emscripten_worker_respond(nullptr, 0);
  }
    
  std::span<char> script(data + 4, script_sz);
    
  save_to_file(script, "/i.script");

  std::span<char> contents(data + 4 + script_sz, size - (script_sz + 4));
  
  save_to_file(contents, "/i.pdb");

  do_decode(options);
}


void decode_contents_color_interleaved(char* data, int size)
{
  decode_contents(data, size, OPTION_COLOR | OPTION_INTERLEAVE);
}


void decode_contents_color_non_interleaved(char* data, int size)
{
  decode_contents(data, size, OPTION_COLOR);
}


void decode_contents_no_color(char* data, int size)
{
  decode_contents(data, size, 0);
}


// data is pdb file contents

void visualise_atoms(char* data, int size)
{
  using namespace animol;

  visualise v({data, static_cast<std::size_t>(size) });

  std::vector<visualise::atom> res;

  v.generate_atoms(res, visualise::ATOMS | visualise::HETATOMS | visualise::SHIFT_TO_CENTER_OF_MASS);

  emscripten_worker_respond(reinterpret_cast<char*>(res.data()), res.size() * sizeof(visualise::atom));
}

void visualise_atoms_url(char* data, int size)
{
  using namespace magic_enum::bitwise_operators;

  std::string url(data, size);

  auto h = plate::async::request(url, "GET", "", [url] (std::uint32_t handle, plate::data_store&& d)
  {
    if (is_cif(d.span()))
    {
      animol::cif2pdb converter(d.span(), animol::cif2pdb::options::ca_atoms | animol::cif2pdb::options::non_ca_atoms);

      auto r = converter.convert();

      if (r)
      {
        //log_debug(FMT_COMPILE("visualise_atoms_url converted cif to: {}"), *r);
        visualise_atoms(r->data(), r->size());
      }
      else
      {
        log_debug("visualise_atoms_url failed to convert cif");
        emscripten_worker_respond(nullptr, 0);
      }
    }
    else
      visualise_atoms(reinterpret_cast<char*>(d.data()), d.size());

  }, [] (std::uint32_t handle, int error_code, std::string error_msg)
  {
    log_debug(FMT_COMPILE("failed to download, error_code: {} msg: {}"), error_code, error_msg);

    emscripten_worker_respond(nullptr, 0);
  },
  {});
}


} // extern "C"
