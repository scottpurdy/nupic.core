# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2015-2017, Numenta, Inc.  Unless you have purchased from
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

# Set up Swig
#
# OUTPUT VARIABLES:
#
#   SWIG_EXECUTABLE: the path to the swig executable as defined by FindSWIG.
#   SWIG_DIR: the directory where swig is installed (.i files, etc.) as defined
#             by FindSWIG.

set(swig_tarfile "swig-3.0.2.tar.gz")
set(swig_path "${REPOSITORY_DIR}/external/common/src/${swig_tarfile}")
set(swig_dir "${CMAKE_BINARY_DIR}/swig/swig-3.0.2")
set(pcre_path "${REPOSITORY_DIR}/external/common/src/pcre-8.37.tar.gz")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # We bundle pre-built Swig for Windows per official recommendation
  add_custom_target(Swig)
  set(swig_executable
      ${PROJECT_SOURCE_DIR}/${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}/bin/swig.exe)
  set(swig_dir ${PROJECT_SOURCE_DIR}/common/share/swig/3.0.2)

elseif(NOT EXISTS ${EP_BASE}/Install/bin/swig)
  # Build Swig from source on non-Windows (e.g., Linux and OS X)
  execute_process(
    COMMAND /bin/bash -c "BUILD_DIR=${CMAKE_BINARY_DIR}/swig SWIG_PATH=${swig_path} PCRE_PATH=${pcre_path} PREFIX=${EP_BASE}/Install ${REPOSITORY_DIR}/external/build_swig.sh"
  )

  set(swig_executable ${EP_BASE}/Install/bin/swig)
  set(swig_dir ${EP_BASE}/Install/share/swig/3.0.2)
endif()

set(SWIG_EXECUTABLE ${swig_executable} PARENT_SCOPE)
set(SWIG_DIR ${swig_dir} PARENT_SCOPE)
