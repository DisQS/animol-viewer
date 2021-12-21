#include "plate.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

//#include "../utils/mol.hpp"


extern "C" {

extern void vertex_free();
extern void vertex_clear();

extern int molauto (int argc, char *argv[]);
extern int molscript (int argc, char *argv[]);

#define OPTION_COLOR      1
#define OPTION_INTERLEAVE 2

extern int compact(char** cdata, int options);


struct fp
{
  short vert[3];
  short norm[3];
  unsigned char col[4];
};


void save_to_file(std::span<char> data, std::string filename)
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


void do_script()
{
  const char *argv[] = { "molauto", "-out", "/i.script", "/i.pdb" };
  
  molauto(sizeof(argv) / sizeof(argv[0]), const_cast<char**>(argv));
  
  std::vector<char> script_file;
  
  read_from_file(script_file, "/i.script");
  
  emscripten_worker_respond(script_file.data(), script_file.size());
}


// data is the url of the pdb file to download and auto script

void script(char* data, int size)
{
  std::string_view sv(data, size);

  auto h = plate::async::request(std::string(sv), "GET", "", [] (std::uint32_t handle, plate::data_store&& d)
  {
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

  auto h = plate::async::request(url, "GET", "", [options] (std::uint32_t handle, plate::data_store&& d)
  {
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


} // extern "C"
