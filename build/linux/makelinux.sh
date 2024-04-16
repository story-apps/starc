#!/bin/sh
set -e

echo "pwd $(pwd)"
echo "parameters $1 $2"

export PATH=$GITHUB_WORKSPACE/Qt/$1/gcc_64/bin:$PATH

if [ "x$MAKEFLAGS" = x'' ] ; then
if [ `uname`  = "Linux" ]; then par=`nproc`; else par=`sysctl -n hw.ncpu`; fi
export MAKEFLAGS=-j$par
fi
echo "MAKEFLAGS=$MAKEFLAGS"

cp -r lib/images .

maketar() {
 cd $1
 tar -czvf ../"$1".tar.gz *
 cd -
}

run() {
QM="${QM:=qmake}"
hash $QM &> /dev/null
if [ $? -eq 1 ]; then
  echo 'use qmake-qt5' >&2
  QM=qmake-qt5
fi
./clean.l64
rm -rf "$1"
cd lib
$QM && make
cd -
cd main
$QM && make
cd -

mv bin/linux-"`uname -m`"/release $1
maketar $1
mkdir -p output
mv "$1".tar.gz output/.

ls -l "$1" || true
rm -rf "$1"
}

run jqt-"$2" "$2"

export JQTSLIM=1
run jqt-"$2"-slim "$2"

if [ -d Qt ] ; then
tar -czf "$2"-Qt.tar.gz Qt
fi

./clean.l64 || true