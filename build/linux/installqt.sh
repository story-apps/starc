#!/bin/sh
set -e

f() { sudo apt-get install --no-install-recommends -y "$@"; }
g() { sudo pkg_add "$@"; }
h() { sudo pkg install -y "$@"; }

sudo apt-get update -y
f libgstreamer-plugins-base1.0-0
f cmake
f make
f pkg-config
f libx11-dev
f xcb
f libx11-xcb-dev
f libxkbcommon-x11-0
f libxkbcommon-dev
f libgtk-3-dev
f sassc