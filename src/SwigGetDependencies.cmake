# This code was pull from an old version of CMake. Following is the corresponding copyright notice for this code:
#
# ---------------------------------------------------------------------
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2017 Kitware, Inc. and Contributors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of Contributors
#   may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ---------------------------------------------------------------------

#
# Get dependencies of the generated wrapper.
#
MACRO(SWIG_GET_WRAPPER_DEPENDENCIES swigFile genWrapper language DEST_VARIABLE)
  GET_FILENAME_COMPONENT(swig_getdeps_basename ${swigFile} NAME_WE)
  GET_FILENAME_COMPONENT(swig_getdeps_outdir ${genWrapper} PATH)
  GET_SOURCE_FILE_PROPERTY(swig_getdeps_extra_flags "${swigFile}" SWIG_FLAGS)
  IF("${swig_getdeps_extra_flags}" STREQUAL "NOTFOUND")
    SET(swig_getdeps_extra_flags "")
  ENDIF("${swig_getdeps_extra_flags}" STREQUAL "NOTFOUND")

  IF(NOT swig_getdeps_outdir)
    SET(swig_getdeps_outdir ${CMAKE_CURRENT_BINARY_DIR})
  ENDIF(NOT swig_getdeps_outdir)
  SET(swig_getdeps_depsfile
    ${swig_getdeps_outdir}/swig_${swig_getdeps_basename}_deps.txt)
  GET_DIRECTORY_PROPERTY(swig_getdeps_include_directories INCLUDE_DIRECTORIES)
  SET(swig_getdeps_include_dirs)
  FOREACH(it ${swig_getdeps_include_directories})
    SET(swig_getdeps_include_dirs ${swig_getdeps_include_dirs} "-I${it}")
  ENDFOREACH(it)
  EXECUTE_PROCESS(
    COMMAND ${SWIG_EXECUTABLE}
    -MM -MF ${swig_getdeps_depsfile} ${swig_getdeps_extra_flags}
    ${CMAKE_SWIG_FLAGS} -${language}
    -o ${genWrapper} ${swig_getdeps_include_dirs} ${swigFile}
    RESULT_VARIABLE swig_getdeps_result
    ERROR_VARIABLE swig_getdeps_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  IF(NOT ${swig_getdeps_error} EQUAL 0)
    MESSAGE(SEND_ERROR "Command \"${SWIG_EXECUTABLE} -MM -MF ${swig_getdeps_depsfile} ${swig_getdeps_extra_flags} ${CMAKE_SWIG_FLAGS} -${language} -o ${genWrapper} ${swig_getdeps_include_dirs} ${swigFile}\" failed with output:\n${swig_getdeps_error}")
    SET(swig_getdeps_dependencies "")
  ELSE(NOT ${swig_getdeps_error} EQUAL 0)
    FILE(READ ${swig_getdeps_depsfile} ${DEST_VARIABLE})
    # Remove the first line
    STRING(REGEX REPLACE "^.+: +\\\\\n +" ""
      ${DEST_VARIABLE} "${${DEST_VARIABLE}}")
    # Clean the end of each line
    STRING(REGEX REPLACE " +(\\\\)?\n" "\n" ${DEST_VARIABLE}
      "${${DEST_VARIABLE}}")
    # Clean beginning of each line
    STRING(REGEX REPLACE "\n +" "\n"
      ${DEST_VARIABLE} "${${DEST_VARIABLE}}")
    # clean paths
    STRING(REGEX REPLACE "\\\\\\\\" "/" ${DEST_VARIABLE}
      "${${DEST_VARIABLE}}")
    STRING(REGEX REPLACE "\n" ";"
      ${DEST_VARIABLE} "${${DEST_VARIABLE}}")
  ENDIF(NOT ${swig_getdeps_error} EQUAL 0)
ENDMACRO(SWIG_GET_WRAPPER_DEPENDENCIES)
