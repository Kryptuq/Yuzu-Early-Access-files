#!/bin/bash -e

# SPDX-FileCopyrightText: 2021 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Setup RC file for tx
cat << EOF > ~/.transifexrc
[https://www.transifex.com]
hostname = https://www.transifex.com
username = api
password = $TRANSIFEX_API_TOKEN
EOF


set -x

echo -e "\e[1m\e[33mBuild tools information:\e[0m"
cmake --version
gcc -v
tx --version

# vcpkg needs these: curl zip unzip tar, have tar
apt-get install -y curl zip unzip

mkdir build && cd build
cmake .. -DENABLE_QT_TRANSLATION=ON -DGENERATE_QT_TRANSLATION=ON -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL2=OFF -DYUZU_TESTS=OFF -DYUZU_USE_BUNDLED_VCPKG=ON
make translation
cd ..

cd dist/languages
tx push -s
