#include "../worker/cif2pdb.hpp"

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
  using namespace magic_enum::bitwise_operators;

  const std::string exitArgMessage = fmt::format("usage: {} [-sheet | -helix | -atom[=ca, =nca] | -hetatm] <PDBx/mmCIF.cif> <out.pdb>\n  no options defaults to output everything.\n", argv[0]);

  if (argc < 3)
  {
    fmt::print("{}", exitArgMessage);
    return EXIT_FAILURE;
  }

  animol::cif2pdb::options opt = animol::cif2pdb::options::none;

  for (int i = 1; i < argc - 2; ++i)
  {
    std::string_view sv(argv[i]);

    if (sv == "-sheet")   { opt |= animol::cif2pdb::options::sheet; continue; }
    if (sv == "-helix")   { opt |= animol::cif2pdb::options::helix; continue; }
    if (sv == "-hetatm")  { opt |= animol::cif2pdb::options::hetatm; continue; }
    if (sv == "-atom=ca") { opt |= animol::cif2pdb::options::ca_atoms; continue; }
    if (sv == "-atom=nca"){ opt |= animol::cif2pdb::options::non_ca_atoms; continue; }
    if (sv == "-atom")    { opt |= animol::cif2pdb::options::ca_atoms | animol::cif2pdb::options::non_ca_atoms; continue; }

    fmt::print("{}", exitArgMessage);
    return EXIT_FAILURE;
  }

  if (argc == 3) // default to all
    opt = animol::cif2pdb::options::sheet | animol::cif2pdb::options::helix | animol::cif2pdb::options::hetatm |
          animol::cif2pdb::options::ca_atoms | animol::cif2pdb::options::non_ca_atoms;

  std::string cif_filename(argv[argc - 2]);
  std::string pdb_filename(argv[argc - 1]);
  auto fd = open(cif_filename.c_str(), O_RDONLY);

  if (fd == -1)
  {
    fmt::print("cannot open cif file: {}\n", cif_filename);
    return EXIT_FAILURE;
  }

  struct stat sb;

  if (fstat(fd, &sb) == -1)
  {
    fmt::print("cannot stat cif file: {}\n", cif_filename);
    close(fd);
    return EXIT_FAILURE;
  }

  std::uint32_t size = sb.st_size;

  auto data = static_cast<std::byte*>(mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0));

  animol::cif2pdb converter({data, size}, opt);

  auto r = converter.convert();

  if (r)
  {
    std::ofstream out_file(pdb_filename);
    out_file << *r;
  }

  munmap(data, size);
  close(fd);

  return r ? EXIT_SUCCESS : EXIT_FAILURE;
}

