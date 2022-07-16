#include "../worker/dcd2pdb.hpp"

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <iostream>
#include <string_view>
#include <fstream>
#include <vector>
#include <charconv>
#include <array>
#include <iomanip>
#include <cstddef>
#include <cstring>

int main(int argc, char* argv[])
{
  // parse command line options
  
  const std::string exitArgMessage = fmt::format("usage: {} <psffile.pdb> <trajectory.dcd> <output directory> <file prefix> [-atom[=ca, =nca]] no options defaults to keep all atoms\n", argv[0]);
  
  constexpr int totalArgs = 6;
  constexpr int optionalArgs = 1;
  
  // If there are too many or too few arguments
  if (argc > totalArgs || argc < totalArgs - optionalArgs) 
  {
    fmt::print(fmt::runtime(exitArgMessage));
    return EXIT_FAILURE;
  }

  int return_value = EXIT_SUCCESS;

  std::string psf_filename(argv[1]);
  std::string dcd_filename(argv[2]);
  std::string output_dir(argv[3]); // Output files saved inside folder output_dir
  std::string filePrefix(argv[4]); // Name of output file is {filePrefix}{file number}.pdb
  
  // default to keep All
  bool keepCAs = true; // CA = Carbon Alpha
  bool keepNonCAs = true; // Everything else execpt CA
  
  /*
  -atom=ca keepAll=false, keepCA=true || -atom=!ca keepAll=true, keepCA=false || -atom=all keepAll=true, keepCA=true ||
  */ 
   
  if (argc == totalArgs) // Atom options (last optional argument)
  {
    std::string_view sv(argv[argc-1]);
    
    if      (sv == "-atom=ca")  {keepCAs=true;  keepNonCAs=false;}
    else if (sv == "-atom=nca") {keepCAs=false; keepNonCAs=true; }
    else if (sv == "-atom")     {keepCAs=true;  keepNonCAs=true; }
    else                        {fmt::print(fmt::runtime(exitArgMessage)); return EXIT_FAILURE;}
  }

  // open psf file

  auto psf_fd = open(psf_filename.c_str(), O_RDONLY);

  if (psf_fd == -1)
  {
    fmt::print("cannot open file: {}\n", psf_filename);
    return EXIT_FAILURE;
  }

  struct stat sb;

  if (fstat(psf_fd, &sb) == -1)
  {
    fmt::print("cannot stat file: {}\n", psf_filename);
    return EXIT_FAILURE;
  }

  std::uint64_t psf_size = sb.st_size;

  auto psf_addr = static_cast<const char*>(mmap(NULL, psf_size, PROT_READ, MAP_SHARED, psf_fd, 0));

  // open dcd file

  auto dcd_fd = open(dcd_filename.c_str(), O_RDONLY);

  if (dcd_fd == -1)
  {
    fmt::print("cannot open file: {}\n", dcd_filename);
    return EXIT_FAILURE;
  }

  if (fstat(dcd_fd, &sb) == -1)
  {
    fmt::print("cannot stat file: {}\n", dcd_filename);
    return EXIT_FAILURE;
  }

  std::uint64_t dcd_size = sb.st_size;

  auto dcd_addr = static_cast<std::byte*>(mmap(NULL, dcd_size, PROT_READ, MAP_SHARED, dcd_fd, 0));

  // convert

  animol::dcd2pdb converter;

  if (!converter.generate_template({psf_addr, psf_size}, keepCAs, keepNonCAs))
  {
    fmt::print("cannot generate template\n");
    return EXIT_FAILURE;
  }

  auto dcd_info = converter.get_dcd_data_info({dcd_addr, dcd_size});

  if (!dcd_info)
  {
    fmt::print("cannot get dcd data info\n");
    return EXIT_FAILURE;
  }

  if (dcd_info->number_atoms != converter.get_number_of_atoms())
  {
    fmt::print(FMT_COMPILE("bad number of atoms, dcd has: {} psf has: {}"), dcd_info->number_atoms, converter.get_number_of_atoms());
    return EXIT_FAILURE;
  }

  auto dcd_size_needed = dcd_info->start_offset + (((4 + 4 + (4 * dcd_info->number_atoms)) * 3) * dcd_info->number_frames);

  if (dcd_size_needed != dcd_size)
  {
    fmt::print(FMT_COMPILE("bad dcd size, have: {} wanted: {}"), dcd_size, dcd_size_needed);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < dcd_info->number_frames; ++i)
  {
    if (!converter.populate_template({dcd_addr + dcd_info->start_offset + (((4 + 4 + (4 * dcd_info->number_atoms)) * 3) * i), ((4 + 4 + (4 * dcd_info->number_atoms)) * 3)}))
    {
      fmt::print(FMT_COMPILE("unable to populate frame: {}"), i);
      return EXIT_FAILURE;
    }

    // save to file

    std::ofstream out_file(fmt::format("{}/{}{}.pdb", output_dir, filePrefix, i));
    out_file << converter.get_pdb_data();
  }

  return EXIT_SUCCESS;
}
