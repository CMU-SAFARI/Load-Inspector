#!/bin/bash

###################################################
# Installation script of Load Inspector
# Author: Rahul Bera (write2bera@gmail.com)
###################################################

SDE_VERSION=9.33
SDE_DOWNLOAD_LINK="https://downloadmirror.intel.com/813591/sde-external-9.33.0-2024-01-07-lin.tar.xz"
SDE_INSTALL_DIR="sde-kit"


if ! [ -d ./${SDE_INSTALL_DIR} ]; then
    mkdir ./${SDE_INSTALL_DIR}
fi

if [ -f ./${SDE_INSTALL_DIR}/sde64 ]; then
    echo "SDE kit found. Skipping SDE download"
else
    echo "Downloading SDE v${SDE_VERSION}..."
    wget ${SDE_DOWNLOAD_LINK} -O sde.tar.xz
    echo "Uncompressing SDE kit..."
    tar -xf sde.tar.xz -C sde-kit --strip-components=1
    rm sde.tar.xz
fi

# Source variables
source setvars.sh

# Build inspector tool
cd src/
make

# Check if built properly
if test -f obj-intel64/inspector-tool.so; then
    echo "Installation finished successfully"
else
    echo "Installation failed"
fi
