
all: cif2pdb dcd2pdb

cif2pdb: cif2pdb.cpp
	g++ -O3 -std=c++2b -I ../ -I ../../external/include/ -I../../external/plate/ -DPLATE -DPLATE_WEBGL cif2pdb.cpp  -o cif2pdb

dcd2pdb: dcd2pdb.cpp
	g++ -O3 -std=c++2b -I ../ -I ../../external/include/ -I../../external/plate/ -DPLATE -DPLATE_WEBGL dcd2pdb.cpp  -o dcd2pdb

clean:
	rm -f cif2pdb dcd2pdb
