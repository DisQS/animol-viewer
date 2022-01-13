#!/bin/bash

# fast_float

rm -rf fast_float

git clone https://github.com/fastfloat/fast_float.git

pushd fast_float

  if [ $? != "0" ]
  then
    exit 1
  fi

  mkdir -p ../include
  cp -rp include/fast_float  ../include/

popd

