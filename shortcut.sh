#!/bin/bash

ORIG_VERSION="1.0.6"
PATCH_VERSION="1"

tar czf ../apt-transport-swift_${ORIG_VERSION}.orig.tar.gz --exclude='.[^/]*' --exclude shortcut.sh --exclude debian ./
debuild -S
dput ppa:rodionovamp/apt-transport-swift ../apt-transport-swift_${ORIG_VERSION}-${PATCH_VERSION}_source.changes