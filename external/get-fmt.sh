#!/bin/bash

# fmt

rm -rf fmt

git clone https://github.com/fmtlib/fmt.git

pushd fmt

  if [ $? != "0" ]
  then
    exit 1
  fi

  mkdir -p ../include/fmt
  cp -rp include/fmt/*.h  ../include/fmt

popd

