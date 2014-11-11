/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2014, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file */

#include <iostream>
#include <capnp/serialize-packed.h>

#include "Proto.hpp"

using namespace nta::proto;

StdOutputStream::StdOutputStream(std::ostream& stream) : stream_(stream) {}

StdOutputStream::~StdOutputStream() noexcept(false)
{}

void StdOutputStream::write(const void* src, size_t size)
{
  stream_.write((char*)src, size);
}

void writeMessage(std::ostream& stream, ::capnp::MessageBuilder& builder)
{
  StdOutputStream out(stream);
  ::capnp::writeMessage(out, builder);
}

StdInputStream::StdInputStream(std::istream& stream) : stream_(stream) {}

StdInputStream::~StdInputStream() noexcept(false)
{}

size_t StdInputStream::tryRead(void* buffer, size_t minBytes, size_t maxBytes)
{
  return stream_.readsome((char*)buffer, maxBytes);
}
