# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2017, Numenta, Inc.  Unless you have purchased from
# Numenta, Inc. a separate commercial license for this software code, the
# following terms and conditions apply:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
#
# http://numenta.org/licenses/
# -----------------------------------------------------------------------------

# Exit on first error and print each command
set -e -x

# Make sure appropriate env vars are set
: "${BUILD_DIR:?Need to set BUILD_DIR}"
: "${SWIG_PATH:?Need to set SWIG_PATH}"
: "${PCRE_PATH:?Need to set PCRE_PATH}"
: "${PREFIX:?Need to set PREFIX}"

SWIG_NAME="swig-3.0.2"

mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR}

cp ${SWIG_PATH} ./
tar xzf ${SWIG_NAME}.tar.gz -- ${SWIG_NAME}

cp ${PCRE_PATH} ./
./${SWIG_NAME}/Tools/pcre-build.sh
./${SWIG_NAME}/configure --prefix=${PREFIX} --enable-cpp11-testing

make
make install

popd
