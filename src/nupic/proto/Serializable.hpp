/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2015, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/*
 * Define the Serializable abstract class for Cap'n Proto serialization.
 */

#ifndef NUPIC_SERIALIZABLE_HPP
#define NUPIC_SERIALIZABLE_HPP

#include <iostream>

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <kj/std/iostream.h>

namespace nupic
{

  // This defines the abstract class for Cap'n Proto serialization.
  //
  // The read and write pure virtual functions must be implemented by
  // the subclasses. These functions perform the reading or population
  // of the struct readers or builders.
  //
  // The template type is the Cap'n Proto struct type for serialization.
  // This is necessary for the pre-defined methods that must create
  // struct-type-specific builders and readers.
  template <class T>
  class Serializable
  {
    public:
      virtual void write(T::Builder proto) = 0;
      virtual void read(T::Reader proto) = 0;

      void write(std::ostream& stream)
      {
        capnp::MallocMessageBuilder message;
        T::Builder proto = message.initRoot<T>();
        write(proto);

        kj::std::StdOutputStream out(stream);
        capnp::writeMessage(out, message);
      }

      void read(std::istream& stream)
      {
        kj::std::StdInputStream in(stream);
        capnp::InputStreamMessageReader message(in);
        T::Reader proto = message.getRoot<T>();
        read(proto);
      }
  };

}

#endif // NUPIC_SERIALIZABLE_HPP
