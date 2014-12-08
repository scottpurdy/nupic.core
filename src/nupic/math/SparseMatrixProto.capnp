@0xeefc26a597cc6a4e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nupic");

# Next ID: 1
struct SparseMatrixProto {
  values @0 :List(List(SparseFloat));
  numRows @1 :UInt32;
  numCols @2 :UInt32;
}

# Next ID: 2
struct SparseFloat {
  index @0 :UInt32;
  value @1 :Float32;
}
