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

/**
 * @file
 * Utility functions for using Capn Proto buffers with stdlib iostreams.
 */

#ifndef NTA_PROTO_HPP
#define NTA_PROTO_HPP

#include <iostream>

#include <kj/io.h>

namespace nta {
  namespace proto {

    class StdOutputStream: public kj::OutputStream {

      public:
        explicit StdOutputStream(std::ostream& stream);
        ~StdOutputStream() noexcept(false);

        virtual void write(const void* src, size_t size) override;

      private:
        std::ostream& stream_;
    };

  } // proto
} // nta

#endif // NTA_PROTO_HPP
