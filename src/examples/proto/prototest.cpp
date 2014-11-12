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
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <nta/algorithms/SpatialPooler.hpp>
#include <nta/utils/Random.hpp>

using namespace std;
using namespace nta;
using namespace nta::algorithms::spatial_pooler;

long diff(timeval & start, timeval & end)
{
  return (
      ((end.tv_sec - start.tv_sec) * 1000000) +
      (end.tv_usec - start.tv_usec)
  );
}

void testSP()
{
  Random random(10);
  struct timeval start, end;
  long mtime;

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

  for (UInt i = 0; i < 10000; ++i)
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

  // Save initial trained model
  ofstream osA("outA.proto", ofstream::binary);
  sp1.write(osA);
  osA.close();

  int out = open("outB.proto", O_WRONLY);
  sp1.write(out);
  close(out);

  ofstream osC("outC.proto", ofstream::binary);
  sp1.save(osC);
  osC.close();

  SpatialPooler sp2;

  long timeA, timeB, timeC = 0;

  for (UInt i = 0; i < 100; ++i)
  {
    // Create new input
    random.shuffle(input, input + inputSize);

    // Get expected output
    UInt outputBaseline[numColumns];
    sp1.compute(input, true, outputBaseline, false);

    UInt outputA[numColumns];
    UInt outputB[numColumns];
    UInt outputC[numColumns];

    // A - First do iostream version
    {
      SpatialPooler spTemp;

      gettimeofday(&start, nullptr);

      // Deserialize
      ifstream is("outA.proto", ifstream::binary);
      spTemp.read(is);
      is.close();

      // Feed new record through
      spTemp.compute(input, true, outputA, false);

      // Serialize
      ofstream os("outA.proto", ofstream::binary);
      spTemp.write(os);
      os.close();

      gettimeofday(&end, nullptr);
      timeA = timeA + diff(start, end);
    }
    // B - Next do fd version
    {
      SpatialPooler spTemp;

      gettimeofday(&start, nullptr);

      // Deserialize
      int in = open("outB.proto", O_RDONLY);
      spTemp.read(in);
      close(in);

      // Feed new record through
      spTemp.compute(input, true, outputB, false);

      // Serialize
      int out = open("outB.proto", O_WRONLY);
      spTemp.write(out);
      close(out);

      gettimeofday(&end, nullptr);
      timeB = timeB + diff(start, end);
    }
    // C - Next do old version
    {
      SpatialPooler spTemp;

      gettimeofday(&start, nullptr);

      // Deserialize
      ifstream is("outC.proto", ifstream::binary);
      spTemp.load(is);
      is.close();

      // Feed new record through
      spTemp.compute(input, true, outputC, false);

      // Serialize
      ofstream os("outC.proto", ofstream::binary);
      spTemp.save(os);
      os.close();

      gettimeofday(&end, nullptr);
      timeC = timeC + diff(start, end);
    }

    for (UInt i = 0; i < numColumns; ++i)
    {
      NTA_ASSERT(outputBaseline[i] == outputA[i]);
      NTA_ASSERT(outputBaseline[i] == outputB[i]);
      NTA_ASSERT(outputBaseline[i] == outputC[i]);
    }

  }

  cout << "Time for iostream capnp: " << ((Real)timeA / 1000.0) << endl;
  cout << "Time for fd capnp: " << ((Real)timeB / 1000.0) << endl;
  cout << "Time for old method: " << ((Real)timeC / 1000.0) << endl;

}

void testRandomFd()
{
  Random r1(7);
  Random r2;

  r1.getUInt32();

  // Serialize
  int out = open(
      "random.proto", O_WRONLY | O_CREAT | O_TRUNC,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  r1.save(out);
  fsync(out);
  close(out);

  // Deserialize
  int in = open("random.proto", O_RDONLY);
  r2.load(in);
  close(in);

  // Test
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
}

void testRandomIOStream()
{
  Random r1(7);
  Random r2;

  r1.getUInt32();

  // Serialize
  ofstream os("random2.proto", ofstream::binary);
  r1.save(os);
  os.flush();
  os.close();

  // Deserialize
  ifstream is("random2.proto", ifstream::binary);
  r2.load(is);
  is.close();

  // Test
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
}

void testRandomManual()
{
  Random r1(7);
  Random r2;

  r1.getUInt32();

  // Serialize
  ofstream os("random3.proto", ofstream::binary);
  os << r1;
  os.flush();
  os.close();

  // Deserialize
  ifstream is("random3.proto", ifstream::binary);
  is >> r2;
  is.close();

  // Test
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
}

void testRandomFdToStream()
{
  Random r1(7);
  Random r2;

  r1.getUInt32();

  // Serialize
  int out = open(
      "random.proto", O_WRONLY | O_CREAT | O_TRUNC,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  r1.save(out);
  close(out);

  // Deserialize
  ifstream is("random2.proto", ifstream::binary);
  r2.load(is);
  is.close();

  // Test
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
}

void testRandomStreamToFd()
{
  Random r1(7);
  Random r2;

  r1.getUInt32();

  // Serialize
  ofstream os("random2.proto", ofstream::binary);
  r1.save(os);
  os.flush();
  os.close();

  // Deserialize
  int in = open("random.proto", O_RDONLY);
  r2.load(in);
  close(in);

  // Test
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
  NTA_ASSERT(r1.getUInt32() == r2.getUInt32());
}

int main(int argc, const char * argv[])
{
  testSP();

  //time_t startFd, endFd;
  //time(&startFd);
  //for (UInt i = 0; i < 50000; ++i)
  //{
  //  testRandomFd();
  //}
  //time(&endFd);
  //cout << "FD time: " << (endFd - startFd) << endl;

  //time_t startStream, endStream;
  //time(&startStream);
  //for (UInt i = 0; i < 50000; ++i)
  //{
  //  testRandomIOStream();
  //}
  //time(&endStream);
  //cout << "Stream time: " << (endStream - startStream) << endl;

  //time_t startManual, endManual;
  //time(&startManual);
  //for (UInt i = 0; i < 50000; ++i)
  //{
  //  testRandomManual();
  //}
  //time(&endManual);
  //cout << "Manual time: " << (endManual - startManual) << endl;

  //testRandomFdToStream();
  //testRandomStreamToFd();

  return 0;
}
