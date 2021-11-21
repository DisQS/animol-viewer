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


void save_to_file(std::span<std::byte> data, std::string filename)
{
  auto f = fopen(filename.c_str(), "w");
  fwrite(data.data(), 1, data.size(), f);
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
  
  emscripten_worker_respond_provisionally(reinterpret_cast<char*>(script_file.data()), script_file.size());
}


// data is the url of the pdb file to download and auto script

void script(char* data, int size)
{
  std::string_view sv(data, size);

  auto h = plate::async::request(std::string(sv), "GET", "", [] (std::uint32_t handle, plate::data_store&& d)
  {
    save_to_file(d.span(), "/i.pdb");

    do_script();

  }, [] (std::uint32_t handle, int error_code, std::string error_msg)
  {
    log_debug(FMT_COMPILE("failed to download, error_code: {} msg: {}"), error_code, error_msg);

    emscripten_worker_respond_provisionally(nullptr, 0);
  },
  {});
}


// data is the pdb file contents

void script_with(char* data, int size)
{
  save_to_file(std::span<std::byte>(reinterpret_cast<std::byte*>(data), size), "/i.pdb");
  
  do_script();
}


// data is the script contents to use

void use_script(char* data, int size)
{
  save_to_file(std::span<std::byte>(reinterpret_cast<std::byte*>(data), size), "/i.script");

  emscripten_worker_respond_provisionally(nullptr, 0);
}


void do_decode(int options)
{
  vertex_clear();

  const char *argv[] = { "molscript", "-vertex", "-s", "-in", "/i.script" };
    
  molscript(sizeof(argv) / sizeof(argv[0]), const_cast<char**>(argv));

  char* cdata = nullptr;
  int csize = compact(&cdata, options);

  emscripten_worker_respond_provisionally(cdata, csize);

  free(cdata);
}


// data is the url of the pdb file to download and decode

void decode_url(char* data, int size, int options) // does not return colours
{

  std::string_view sv(data, size);

  auto h = plate::async::request(std::string(sv), "GET", "", [options] (std::uint32_t handle, plate::data_store&& d)
  {
    save_to_file(d.span(), "/i.pdb");

    do_decode(options);

  }, [] (std::uint32_t handle, int error_code, std::string error_msg)
  {
    log_debug(FMT_COMPILE("failed to download, error_code: {} msg: {}"), error_code, error_msg);
  
    emscripten_worker_respond_provisionally(nullptr, 0);
  },
  {});
}


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


// data is the pdb file contents, or, if size is 0, use the current pdb file

void decode_contents(char* data, int size, int options)
{
  if (size > 0)
    save_to_file(std::span<std::byte>(reinterpret_cast<std::byte*>(data), size), "/i.pdb");

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
