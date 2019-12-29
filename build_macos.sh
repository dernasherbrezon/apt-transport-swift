#!/bin/zsh

rm -r build
mkdir build
cd build
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl/ ..
make
make test
cd ..

