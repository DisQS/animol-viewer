#!/bin/bash

# fmt

rm -rf pfr

git clone https://github.com/apolukhin/pfr_non_boost.git pfr

pushd  pfr

  if [ $? != "0" ]
  then
    exit 1
  fi

  mkdir -p ../include
  cp -rp include/*  ../include/

popd

