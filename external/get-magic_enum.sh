#!/bin/bash

# magic_enum

rm -rf magic_enum

git clone https://github.com/Neargye/magic_enum.git

pushd magic_enum

  if [ $? != "0" ]
  then
    exit 1
  fi

  mkdir -p ../include
  cp include/magic_enum.hpp  ../include/

popd

