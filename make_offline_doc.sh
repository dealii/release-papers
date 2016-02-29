#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Offline documentation generator for deal.II"
    echo "Usage:"
    echo "  ./make_offline_doc.sh <major> <minor> <patch>"
    echo "Execute in the base directory of a clean checkout for the release."
    echo "For an RC use: './make_offline_doc.sh 8 3 0-rc1'"
    exit 1
fi

MAJOR=$1
MINOR=$2
PATCH=$3

echo release $MAJOR.$MINOR.$PATCH

# Generate documentation
rm -rf builddoc
mkdir builddoc
cd builddoc
cmake -DDEAL_II_WITH_MPI=OFF -DCMAKE_BUILD_TYPE=Debug -DDEAL_II_COMPONENT_DOCUMENTATION=ON -DCMAKE_INSTALL_PREFIX=`pwd`/../installeddoc/ ../
make documentation -j 8

cd ../installeddoc

cd doc/doxygen/deal.II
../../../../contrib/utilities/makeofflinedoc.sh

cd ../../..
tar czf ../dealii-$MAJOR.$MINOR.$PATCH-offline_documentation.tar.gz doc
cd ..

