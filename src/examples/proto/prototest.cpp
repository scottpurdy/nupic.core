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

#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <nta/algorithms/SpatialPooler.hpp>
#include <nta/utils/Random.hpp>

using namespace std;
using namespace nta;
using namespace nta::algorithms::spatial_pooler;

int main(int argc, const char * argv[])
{
  Random random(10);

  const UInt inputSize = 500;
  const UInt numColumns = 500;
  const UInt w = 50;

  vector<UInt> inputDims{inputSize};
  vector<UInt> colDims{numColumns};

  SpatialPooler sp1;
  sp1.initialize(inputDims, colDims);

  UInt input[inputSize];
  for (UInt i = 0; i < inputSize; ++i)
  {
    if (i < w)
    {
      input[i] = 1;
    } else {
      input[i] = 0;
    }
  }
  UInt output[numColumns];

  for (UInt i = 0; i < 100; ++i)
  {
    random.shuffle(input, input + inputSize);
    sp1.compute(input, true, output, false);
  }

  // Now we reuse the last input to test after serialization

  vector<UInt> activeColumnsBefore;
  for (UInt i = 0; i < numColumns; ++i)
  {
    if (output[i] == 1)
    {
      activeColumnsBefore.push_back(i);
    }
  }

  cout << "A" << endl;

  SpatialPooler sp2;

  if (false)
  {
    // Serialize
    ofstream os("out.proto", ofstream::binary);
    sp1.save(os);
    os.close();

    cout << "B" << endl;

    // Deserialize
    ifstream is("out.proto", ifstream::binary);
    sp2.load(is);
    is.close();

  } else {
    // Serialize
    int out = open("out.proto", O_WRONLY);
    sp1.write(out);
    close(out);

    cout << "B" << endl;

    // Deserialize
    int in = open("out.proto", O_RDONLY);
    sp2.read(in);
    close(in);

  }

  cout << "C" << endl;

  UInt outputBaseline[numColumns];
  sp1.compute(input, true, outputBaseline, false);
  UInt outputLoaded[numColumns];
  sp2.compute(input, true, outputLoaded, false);

  for (UInt i = 0; i < numColumns; ++i)
  {
    NTA_ASSERT(outputBaseline[i] == outputLoaded[i]);
  }

  return 0;
}
