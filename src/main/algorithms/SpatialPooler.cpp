/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.  Unless you have an agreement
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
 * ----------------------------------------------------------------------
 */

/** @file
 * Implementation of SpatialPooler
 */

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <nta/algorithms/SpatialPooler.hpp>
#include <nta/math/Math.hpp>
#include <nta/utils/Proto.hpp>

#include "SpatialPoolerProto.capnp.h"

using namespace std;
using namespace nta;
using namespace nta::algorithms::spatial_pooler;

// MSVC doesn't provide round() which only became standard in C99 or C++11
#if defined(NTA_COMPILER_MSVC)
  template<typename T> 
  T round(T num) {
      return (num > 0.0) ? floor(num + 0.5) : ceil(num - 0.5);
  }
#endif


// Round f to 5 digits of precision. This is used to set
// permanence values and help avoid small amounts of drift between
// platforms/implementations
static Real round5_(const Real f)
{
  Real p = ((Real) ((Int) (f * 100000))) / 100000.0;
  return p;
}



class CoordinateConverter2D {

  public:
    CoordinateConverter2D(UInt nrows, UInt ncols) : //TODO param nrows is unused
      ncols_(ncols) {}
    UInt toRow(UInt index) { return index / ncols_; };
    UInt toCol(UInt index) { return index % ncols_; };
    UInt toIndex(UInt row, UInt col) { return row * ncols_ + col; };

  private:
    UInt ncols_;
};


class CoordinateConverterND {

  public:
    CoordinateConverterND(vector<UInt>& dimensions)
    {
      dimensions_ = dimensions;
      UInt b = 1;
      for (Int i = (Int) dimensions.size()-1; i >= 0; i--) {
        bounds_.insert(bounds_.begin(), b);
        b *= dimensions[i];
      }
    }

    void toCoord(UInt index, vector<UInt>& coord)
    {
      coord.clear();
      for (UInt i = 0; i < bounds_.size(); i++)  {
        coord.push_back((index / bounds_[i]) % dimensions_[i]);
      }
    };

    UInt toIndex(vector<UInt>& coord)
    {
      UInt index = 0;
      for (UInt i = 0; i < coord.size(); i++) {
        index += coord[i] * bounds_[i];
      }
      return index;
    };

  private:
    vector<UInt> dimensions_;
    vector<UInt> bounds_;
};

SpatialPooler::SpatialPooler() {
  // The current version number. 
  version_ = 2;
}

vector<UInt> SpatialPooler::getColumnDimensions() {
  return columnDimensions_;
}

vector<UInt> SpatialPooler::getInputDimensions() {
  return inputDimensions_;
}

UInt SpatialPooler::getNumColumns() {
  return numColumns_;
}

UInt SpatialPooler::getNumInputs() {
  return numInputs_;
}

UInt SpatialPooler::getPotentialRadius()
{
  return potentialRadius_;
}

void SpatialPooler::setPotentialRadius(UInt potentialRadius)
{
  potentialRadius_ = potentialRadius;
}

Real SpatialPooler::getPotentialPct()
{
  return potentialPct_;
}

void SpatialPooler::setPotentialPct(Real potentialPct)
{
  potentialPct_ = potentialPct;
}

bool SpatialPooler::getGlobalInhibition()
{
  return globalInhibition_;
}

void SpatialPooler::setGlobalInhibition(bool globalInhibition)
{
  globalInhibition_ = globalInhibition;
}

Int SpatialPooler::getNumActiveColumnsPerInhArea()
{
  return numActiveColumnsPerInhArea_;
}

void SpatialPooler::setNumActiveColumnsPerInhArea(
    UInt numActiveColumnsPerInhArea)
{
  NTA_ASSERT(numActiveColumnsPerInhArea > 0);
  numActiveColumnsPerInhArea_ = numActiveColumnsPerInhArea;
  localAreaDensity_ = 0;
}

Real SpatialPooler::getLocalAreaDensity()
{
  return localAreaDensity_;
}

void SpatialPooler::setLocalAreaDensity(Real localAreaDensity)
{
  NTA_ASSERT(localAreaDensity > 0 && localAreaDensity <= 1);
  localAreaDensity_ = localAreaDensity;
  numActiveColumnsPerInhArea_ = 0;
}

UInt SpatialPooler::getStimulusThreshold()
{
  return stimulusThreshold_;
}

void SpatialPooler::setStimulusThreshold(UInt stimulusThreshold)
{
  stimulusThreshold_ = stimulusThreshold;
}

UInt SpatialPooler::getInhibitionRadius()
{
  return inhibitionRadius_;
}

void SpatialPooler::setInhibitionRadius(UInt inhibitionRadius)
{
  inhibitionRadius_ = inhibitionRadius;
}

UInt SpatialPooler::getDutyCyclePeriod()
{
  return dutyCyclePeriod_;
}

void SpatialPooler::setDutyCyclePeriod(UInt dutyCyclePeriod)
{
  dutyCyclePeriod_ = dutyCyclePeriod;
}

Real SpatialPooler::getMaxBoost()
{
  return maxBoost_;
}

void SpatialPooler::setMaxBoost(Real maxBoost)
{
  maxBoost_ = maxBoost;
}

UInt SpatialPooler::getIterationNum()
{
  return iterationNum_;
}

void SpatialPooler::setIterationNum(UInt iterationNum)
{
  iterationNum_ = iterationNum;
}

UInt SpatialPooler::getIterationLearnNum()
{
  return iterationLearnNum_;
}

void SpatialPooler::setIterationLearnNum(UInt iterationLearnNum)
{
  iterationLearnNum_ = iterationLearnNum;
}

UInt SpatialPooler::getSpVerbosity()
{
  return spVerbosity_;
}

void SpatialPooler::setSpVerbosity(UInt spVerbosity)
{
  spVerbosity_ = spVerbosity;
}

bool SpatialPooler::getWrapAround()
{
  return wrapAround_;
}

void SpatialPooler::setWrapAround(bool wrapAround)
{
  wrapAround_ = wrapAround;
}

UInt SpatialPooler::getUpdatePeriod()
{
  return updatePeriod_;
}

void SpatialPooler::setUpdatePeriod(UInt updatePeriod)
{
  updatePeriod_ = updatePeriod;
}

Real SpatialPooler::getSynPermTrimThreshold()
{
  return synPermTrimThreshold_;
}

void SpatialPooler::setSynPermTrimThreshold(Real synPermTrimThreshold)
{
  synPermTrimThreshold_ = synPermTrimThreshold;
}

Real SpatialPooler::getSynPermActiveInc()
{
  return synPermActiveInc_;
}

void SpatialPooler::setSynPermActiveInc(Real synPermActiveInc)
{
  synPermActiveInc_ = synPermActiveInc;
}

Real SpatialPooler::getSynPermInactiveDec()
{
  return synPermInactiveDec_;
}

void SpatialPooler::setSynPermInactiveDec(Real synPermInactiveDec)
{
  synPermInactiveDec_ = synPermInactiveDec;
}

Real SpatialPooler::getSynPermBelowStimulusInc()
{
  return synPermBelowStimulusInc_;
}

void SpatialPooler::setSynPermBelowStimulusInc(Real synPermBelowStimulusInc)
{
  synPermBelowStimulusInc_ = synPermBelowStimulusInc;
}

Real SpatialPooler::getSynPermConnected()
{
  return synPermConnected_;
}

void SpatialPooler::setSynPermConnected(Real synPermConnected)
{
  synPermConnected_ = synPermConnected;
}

Real SpatialPooler::getMinPctOverlapDutyCycles()
{
  return minPctOverlapDutyCycles_;
}

void SpatialPooler::setMinPctOverlapDutyCycles(Real minPctOverlapDutyCycles)
{
  minPctOverlapDutyCycles_ = minPctOverlapDutyCycles;
}

Real SpatialPooler::getMinPctActiveDutyCycles()
{
  return minPctActiveDutyCycles_;
}

void SpatialPooler::setMinPctActiveDutyCycles(Real minPctActiveDutyCycles)
{
  minPctActiveDutyCycles_ = minPctActiveDutyCycles;
}

void SpatialPooler::getBoostFactors(Real boostFactors[])
{
  copy(boostFactors_.begin(), boostFactors_.end(), boostFactors);
}

void SpatialPooler::setBoostFactors(Real boostFactors[])
{
  boostFactors_.assign(&boostFactors[0], &boostFactors[numColumns_]);
}

void SpatialPooler::getOverlapDutyCycles(Real overlapDutyCycles[])
{
  copy(overlapDutyCycles_.begin(), overlapDutyCycles_.end(),
       overlapDutyCycles);
}

void SpatialPooler::setOverlapDutyCycles(Real overlapDutyCycles[])
{
  overlapDutyCycles_.assign(&overlapDutyCycles[0],
                            &overlapDutyCycles[numColumns_]);
}

void SpatialPooler::getActiveDutyCycles(Real activeDutyCycles[])
{
  copy(activeDutyCycles_.begin(), activeDutyCycles_.end(), activeDutyCycles);
}

void SpatialPooler::setActiveDutyCycles(Real activeDutyCycles[])
{
  activeDutyCycles_.assign(&activeDutyCycles[0],
                           &activeDutyCycles[numColumns_]);
}

void SpatialPooler::getMinOverlapDutyCycles(Real minOverlapDutyCycles[])
{
  copy(minOverlapDutyCycles_.begin(), minOverlapDutyCycles_.end(),
       minOverlapDutyCycles);
}

void SpatialPooler::setMinOverlapDutyCycles(Real minOverlapDutyCycles[])
{
  minOverlapDutyCycles_.assign(&minOverlapDutyCycles[0],
                               &minOverlapDutyCycles[numColumns_]);
}

void SpatialPooler::getMinActiveDutyCycles(Real minActiveDutyCycles[])
{
  copy(minActiveDutyCycles_.begin(), minActiveDutyCycles_.end(),
       minActiveDutyCycles);
}

void SpatialPooler::setMinActiveDutyCycles(Real minActiveDutyCycles[])
{
  minActiveDutyCycles_.assign(&minActiveDutyCycles[0],
                              &minActiveDutyCycles[numColumns_]);
}

void SpatialPooler::getPotential(UInt column, UInt potential[])
{
  NTA_ASSERT(column < numColumns_);
  potentialPools_.getRow(column, &potential[0], &potential[numInputs_]);
}

void SpatialPooler::setPotential(UInt column, UInt potential[])
{
  NTA_ASSERT(column < numColumns_);
  potentialPools_.rowFromDense(column, &potential[0], &potential[numInputs_]);
}

void SpatialPooler::getPermanence(UInt column, Real permanences[])
{
  NTA_ASSERT(column < numColumns_);
  permanences_.getRowToDense(column, permanences);
}

void SpatialPooler::setPermanence(UInt column, Real permanences[])
{
  NTA_ASSERT(column < numColumns_);
  vector<Real> perm;
  perm.assign(&permanences[0],&permanences[numInputs_]);
  updatePermanencesForColumn_(perm, column, false);
}

void SpatialPooler::getConnectedSynapses(UInt column, UInt connectedSynapses[])
{
  NTA_ASSERT(column < numColumns_);
  connectedSynapses_.getRow(column,&connectedSynapses[0],
                            &connectedSynapses[numInputs_]);
}

void SpatialPooler::getConnectedCounts(UInt connectedCounts[])
{
  copy(connectedCounts_.begin(), connectedCounts_.end(), connectedCounts);
}



void SpatialPooler::initialize(vector<UInt> inputDimensions,
  vector<UInt> columnDimensions,
  UInt potentialRadius,
  Real potentialPct,
  bool globalInhibition,
  Real localAreaDensity,
  UInt numActiveColumnsPerInhArea,
  UInt stimulusThreshold,
  Real synPermInactiveDec,
  Real synPermActiveInc,
  Real synPermConnected,
  Real minPctOverlapDutyCycles,
  Real minPctActiveDutyCycles,
  UInt dutyCyclePeriod,
  Real maxBoost,
  Int seed,
  UInt spVerbosity,
  bool wrapAround)
{

  numInputs_ = 1;
  inputDimensions_.clear();
  for (auto & inputDimension : inputDimensions)
  {
    numInputs_ *= inputDimension;
    inputDimensions_.push_back(inputDimension);
  }

  numColumns_ = 1;
  columnDimensions_.clear();
  for (auto & columnDimension : columnDimensions)
  {
    numColumns_ *= columnDimension;
    columnDimensions_.push_back(columnDimension);
  }

  NTA_ASSERT(numColumns_ > 0);
  NTA_ASSERT(numInputs_ > 0);
  NTA_ASSERT(inputDimensions_.size() == columnDimensions_.size());
  NTA_ASSERT(numActiveColumnsPerInhArea > 0 ||
            (localAreaDensity > 0 && localAreaDensity <= 0.5));
  NTA_ASSERT(potentialPct > 0 && potentialPct <= 1);

  seed_( (UInt64)(seed < 0 ? rand() : seed) );

  potentialRadius_ = potentialRadius > numInputs_ ? numInputs_ :
                                                    potentialRadius;
  potentialPct_ = potentialPct;
  globalInhibition_ = globalInhibition;
  numActiveColumnsPerInhArea_ = numActiveColumnsPerInhArea;
  localAreaDensity_ = localAreaDensity;
  stimulusThreshold_ = stimulusThreshold;
  synPermInactiveDec_ = synPermInactiveDec;
  synPermActiveInc_ = synPermActiveInc;
  synPermBelowStimulusInc_ = synPermConnected / 10.0;
  synPermConnected_ = synPermConnected;
  minPctOverlapDutyCycles_ = minPctOverlapDutyCycles;
  minPctActiveDutyCycles_ = minPctActiveDutyCycles;
  dutyCyclePeriod_ = dutyCyclePeriod;
  maxBoost_ = maxBoost;
  spVerbosity_ = spVerbosity;
  wrapAround_ = wrapAround;
  synPermMin_ = 0.0;
  synPermMax_ = 1.0;
  synPermTrimThreshold_ = synPermActiveInc / 2.0;
  NTA_ASSERT(synPermTrimThreshold_ < synPermConnected_);
  updatePeriod_ = 50;
  initConnectedPct_ = 0.5;
  iterationNum_ = 0;
  iterationLearnNum_ = 0;

  tieBreaker_.resize(numColumns_);
  for (UInt i = 0; i < numColumns_; i++) {
    tieBreaker_[i] = 0.01 * rng_.getReal64();
  }

  potentialPools_.resize(numColumns_, numInputs_);
  permanences_.resize(numColumns_, numInputs_);
  connectedSynapses_.resize(numColumns_, numInputs_);
  connectedCounts_.resize(numColumns_);

  overlapDutyCycles_.assign(numColumns_, 0);
  activeDutyCycles_.assign(numColumns_, 0);
  minOverlapDutyCycles_.assign(numColumns_, 0.0);
  minActiveDutyCycles_.assign(numColumns_, 0.0);
  boostFactors_.assign(numColumns_, 1);
  overlaps_.resize(numColumns_);
  overlapsPct_.resize(numColumns_);
  boostedOverlaps_.resize(numColumns_);

  inhibitionRadius_ = 0;

  for (UInt i = 0; i < numColumns_; ++i)
  {
    vector<UInt> potential = mapPotential_(i, wrapAround_);
    vector<Real> perm = initPermanence_(potential, initConnectedPct_);
    potentialPools_.rowFromDense(i,potential.begin(),potential.end());
    updatePermanencesForColumn_(perm,i,true);
  }

  updateInhibitionRadius_();

  if (spVerbosity_ > 0) {
    printParameters();
    std::cout << "CPP SP seed                 = " << seed << std::endl;
  }
}

void SpatialPooler::compute(UInt inputArray[], bool learn,
                            UInt activeArray[], bool stripNeverLearned)
{
  updateBookeepingVars_(learn);
  calculateOverlap_(inputArray, overlaps_);
  calculateOverlapPct_(overlaps_, overlapsPct_);

  if (learn) {
    boostOverlaps_(overlaps_, boostedOverlaps_);
  } else {
    boostedOverlaps_.assign(overlaps_.begin(), overlaps_.end());
  }

  inhibitColumns_(boostedOverlaps_, activeColumns_);
  toDense_(activeColumns_, activeArray, numColumns_);

  if (learn) {
    adaptSynapses_(inputArray, activeColumns_);
    updateDutyCycles_(overlaps_, activeArray);
    bumpUpWeakColumns_();
    updateBoostFactors_();
    if (isUpdateRound_()) {
      updateInhibitionRadius_();
      updateMinDutyCycles_();
    }
  } else if (stripNeverLearned) {
    stripUnlearnedColumns(activeArray);
  }
}

void SpatialPooler::compute(UInt inputArray[], bool learn,
                            UInt activeArray[]) {
  compute(inputArray, learn, activeArray, true);
}

void SpatialPooler::stripUnlearnedColumns(UInt activeArray[])
{
  for (UInt i = 0; i < numColumns_; i++) {
    if (activeDutyCycles_[i] == 0) {
      activeArray[i] = 0;
    }
  }
}


void SpatialPooler::toDense_(vector<UInt>& sparse,
                            UInt dense[],
                            UInt n)
{
  std::fill(dense,dense+n, 0);
  for (auto & elem : sparse) {
    UInt index = elem;
    dense[index] = 1;
  }
}

void SpatialPooler::boostOverlaps_(vector<UInt>& overlaps,
                                   vector<Real>& boosted)
{
  for (UInt i = 0; i < numColumns_; i++) {
    boosted[i] = overlaps[i] * boostFactors_[i];
  }
}

UInt SpatialPooler::mapColumn_(UInt column)
{
  CoordinateConverterND columnConv(columnDimensions_);
  CoordinateConverterND inputConv(inputDimensions_);
  vector<UInt> columnCoord, inputCoord;

  columnConv.toCoord(column, columnCoord);

  Real ratio;
  UInt coord;
  for (UInt i = 0; i < columnCoord.size(); i++) {
    ratio = (Real)columnCoord[i] / columnDimensions_[i];
    coord = inputDimensions_[i] * ratio;
    coord += 0.5 * inputDimensions_[i] / columnDimensions_[i];
    inputCoord.push_back(coord);
  }

  return inputConv.toIndex(inputCoord);
}

vector<UInt> SpatialPooler::mapPotential_(UInt column, bool wrapAround)
{
  vector<UInt> potential(numInputs_,0);
  vector<UInt> indices;
  UInt index;

  index = mapColumn_(column);
  getNeighborsND_(index, inputDimensions_, potentialRadius_, wrapAround, indices);
  indices.push_back(index);

  // TODO: See https://github.com/numenta/nupic.core/issues/128
  sort(indices.begin(), indices.end());

  random_shuffle(indices.begin(),indices.end(),rng_);

  Int numPotential = Int(round(indices.size() * potentialPct_));
  for (Int i = 0; i < numPotential; i++) {
    potential[indices[i]] = 1;
  }

  return potential;
}

Real SpatialPooler::initPermConnected_()
{
  Real p = synPermConnected_ + rng_.getReal64() * synPermActiveInc_ / 4.0;
  return round5_(p);
}

Real SpatialPooler::initPermNonConnected_()
{
  Real p = synPermConnected_ * rng_.getReal64();
  return round5_(p);
}

vector<Real> SpatialPooler::initPermanence_(vector<UInt>& potential,
                                            Real connectedPct)
{
  vector<Real> perm(numInputs_, 0);
  for (UInt i = 0; i < numInputs_; i++) {
    if (potential[i] < 1) {
      continue;
    }

    if (rng_.getReal64() <= connectedPct) {
      perm[i] = initPermConnected_();
    } else {
      perm[i] = initPermNonConnected_();
    }
    perm[i] = perm[i] < synPermTrimThreshold_ ? 0 : perm[i];
  }

  return perm;
}

void SpatialPooler::clip_(vector<Real>& perm, bool trim=false)
{
  Real minVal = trim ? synPermTrimThreshold_ : synPermMin_;
  for (auto & elem : perm)
  {
    elem = elem > synPermMax_ ? synPermMax_ : elem;
    elem = elem < minVal ? synPermMin_ : elem;
  }
}


void SpatialPooler::updatePermanencesForColumn_(vector<Real>& perm,
                                                UInt column,
                                                bool raisePerm)
{
  vector<UInt> connectedSparse;

  UInt numConnected;
  if (raisePerm) {
    vector<UInt> potential;
    potential.resize(numInputs_);
    potential = potentialPools_.getSparseRow(column);
    raisePermanencesToThreshold_(perm,potential);
  }

  numConnected = 0;
  for (UInt i = 0; i < perm.size(); ++i)
  {
    if (perm[i] >= synPermConnected_) {
      connectedSparse.push_back(i);
      ++numConnected;
    }
  }

  clip_(perm, true);
  connectedSynapses_.replaceSparseRow(column,connectedSparse.begin(),
                                      connectedSparse.end());
  permanences_.setRowFromDense(column, perm);
  connectedCounts_[column] = numConnected;
}

UInt SpatialPooler::countConnected_(vector<Real>& perm)
{
  UInt numConnected = 0;
  for (auto & elem : perm) {
     if (elem > synPermConnected_) {
       ++numConnected;
     }
   }
  return numConnected;
}

UInt SpatialPooler::raisePermanencesToThreshold_(vector<Real>& perm,
                                                 vector<UInt>& potential)
{
  clip_(perm, false);
  UInt numConnected;
  while (true)
  {
    numConnected = countConnected_(perm);
    if (numConnected >= stimulusThreshold_)
      break;

    for (auto & elem : potential) {
      UInt index = elem;
      perm[index] += synPermBelowStimulusInc_;
    }
  }
  return numConnected;
}

void SpatialPooler::updateInhibitionRadius_()
{
  if (globalInhibition_) {
    inhibitionRadius_ = *max_element(columnDimensions_.begin(),
                                     columnDimensions_.end());
    return;
  }

  Real connectedSpan = 0;
  for (UInt i = 0; i < numColumns_; i++) {
    connectedSpan += avgConnectedSpanForColumnND_(i);
  }
  connectedSpan /= numColumns_;
  Real columnsPerInput = avgColumnsPerInput_();
  Real diameter = connectedSpan * columnsPerInput;
  Real radius = (diameter - 1) / 2.0;
  radius = max((Real) 1.0, radius);
  inhibitionRadius_ = UInt(round(radius));
}

void SpatialPooler::updateMinDutyCycles_()
{
  if (globalInhibition_ || inhibitionRadius_ >
    *max_element(columnDimensions_.begin(), columnDimensions_.end())) {
    updateMinDutyCyclesGlobal_();
  } else {
    updateMinDutyCyclesLocal_();
  }

  return;
}

void SpatialPooler::updateMinDutyCyclesGlobal_()
{
  Real maxActiveDutyCycles = *max_element(activeDutyCycles_.begin(),
                                          activeDutyCycles_.end());
  Real maxOverlapDutyCycles = *max_element(overlapDutyCycles_.begin(),
                                           overlapDutyCycles_.end());
  fill(minActiveDutyCycles_.begin(), minActiveDutyCycles_.end(),
       minPctActiveDutyCycles_ * maxActiveDutyCycles);

  fill(minOverlapDutyCycles_.begin(), minOverlapDutyCycles_.end(),
       minPctOverlapDutyCycles_ * maxOverlapDutyCycles);
}

void SpatialPooler::updateMinDutyCyclesLocal_()
{
  for (UInt i = 0; i < numColumns_; i++) {
    vector<UInt> neighbors;

    getNeighborsND_(i, columnDimensions_, inhibitionRadius_, false, neighbors);
    neighbors.push_back(i);
    Real maxActiveDuty = 0;
    Real maxOverlapDuty = 0;
    for (auto & neighbor : neighbors) {
      UInt index = neighbor;
      maxActiveDuty = max(maxActiveDuty, activeDutyCycles_[index]);
      maxOverlapDuty = max(maxOverlapDuty, overlapDutyCycles_[index]);
    }

    minActiveDutyCycles_[i] = maxActiveDuty * minPctActiveDutyCycles_;
    minOverlapDutyCycles_[i] = maxOverlapDuty * minPctOverlapDutyCycles_;
  }
}

void SpatialPooler::updateDutyCycles_(vector<UInt>& overlaps,
                       UInt activeArray[])
{
  vector<UInt> newOverlapVal(numColumns_, 0);
  vector<UInt> newActiveVal(numColumns_, 0);

  for (UInt i = 0; i < numColumns_; i++) {
    newOverlapVal[i] = overlaps[i] > 0 ? 1 : 0;
    newActiveVal[i] = activeArray[i] > 0 ? 1 : 0;
  }

  UInt period = dutyCyclePeriod_ > iterationNum_ ?
    iterationNum_ : dutyCyclePeriod_;

  updateDutyCyclesHelper_(overlapDutyCycles_, newOverlapVal, period);
  updateDutyCyclesHelper_(activeDutyCycles_, newActiveVal, period);
}

Real SpatialPooler::avgColumnsPerInput_()
{
  UInt numDim = max(columnDimensions_.size(), inputDimensions_.size());
  Real columnsPerInput = 0;
  for (UInt i = 0; i < numDim; i++) {
    Real col = (i < columnDimensions_.size()) ? columnDimensions_[i] : 1;
    Real input = (i < inputDimensions_.size()) ? inputDimensions_[i] : 1;
    columnsPerInput += col / input;
  }
  return columnsPerInput / numDim;
}

Real SpatialPooler::avgConnectedSpanForColumn1D_(UInt column)
{

  NTA_ASSERT(inputDimensions_.size() == 1);
  vector<UInt> connectedSparse = connectedSynapses_.getSparseRow(column);
  if (connectedSparse.empty())
    return 0;
  UInt minIndex = *min_element(connectedSparse.begin(),
                               connectedSparse.end());
  UInt maxIndex = *max_element(connectedSparse.begin(),
                               connectedSparse.end());
  return maxIndex - minIndex + 1;
}

Real SpatialPooler::avgConnectedSpanForColumn2D_(UInt column)
{

  NTA_ASSERT(inputDimensions_.size() == 2);

  UInt nrows = inputDimensions_[0];
  UInt ncols = inputDimensions_[1];

  CoordinateConverter2D conv(nrows,ncols);

  vector<UInt> connectedSparse = connectedSynapses_.getSparseRow(column);
  vector<UInt> rows, cols;
  for (auto & elem : connectedSparse) {
    UInt index = elem;
    rows.push_back(conv.toRow(index));
    cols.push_back(conv.toCol(index));
  }

  if (rows.empty() && cols.empty()) {
    return 0;
  }

  UInt rowSpan = *max_element(rows.begin(),rows.end()) -
                 *min_element(rows.begin(),rows.end()) + 1;

  UInt colSpan = *max_element(cols.begin(),cols.end()) -
                 *min_element(cols.begin(),cols.end()) + 1;

  return (rowSpan + colSpan) / 2.0;

}

Real SpatialPooler::avgConnectedSpanForColumnND_(UInt column)
{
  UInt numDimensions = inputDimensions_.size();
  vector<UInt> connectedSparse = connectedSynapses_.getSparseRow(column);
  vector<UInt> maxCoord(numDimensions, 0);
  vector<UInt> minCoord(numDimensions, *max_element(inputDimensions_.begin(),
                                                    inputDimensions_.end()));

  CoordinateConverterND conv(inputDimensions_);

  if (connectedSparse.empty() ) {
    return 0;
  }

  vector<UInt> columnCoord;
  for (auto & elem : connectedSparse) {
    conv.toCoord(elem,columnCoord);
    for (UInt j = 0; j < columnCoord.size(); j++) {
      maxCoord[j] = max(maxCoord[j], columnCoord[j]);
      minCoord[j] = min(minCoord[j], columnCoord[j]);
    }
  }

  UInt totalSpan = 0;
  for (UInt j = 0; j < inputDimensions_.size(); j++) {
    totalSpan += maxCoord[j] - minCoord[j] + 1;
  }

  return (Real) totalSpan / inputDimensions_.size();

}

void SpatialPooler::adaptSynapses_(UInt inputVector[],
                    vector<UInt>& activeColumns)
{
  vector<Real> permChanges(numInputs_, -1 * synPermInactiveDec_);
  for (UInt i = 0; i < numInputs_; i++) {
    if (inputVector[i] > 0) {
      permChanges[i] = synPermActiveInc_;
    }
  }

  for (UInt i = 0; i < activeColumns.size(); i++) {
    UInt column = activeColumns[i];
    vector<UInt> potential;
    vector <Real> perm(numInputs_, 0);
    potential.resize(potentialPools_.nNonZerosOnRow(i));
    potential = potentialPools_.getSparseRow(column);
    permanences_.getRowToDense(column, perm);
    for (auto & elem : potential) {
        UInt index = elem;
        perm[index] += permChanges[index];
    }
    updatePermanencesForColumn_(perm, column, true);
  }
}

void SpatialPooler::bumpUpWeakColumns_()
{
  for (UInt i = 0; i < numColumns_; i++) {
    if (overlapDutyCycles_[i] >= minOverlapDutyCycles_[i]) {
      continue;
    }
    vector<Real> perm(numInputs_, 0);
    vector<UInt> potential;
    potential.resize(potentialPools_.nNonZerosOnRow(i));
    potential = potentialPools_.getSparseRow(i);
    permanences_.getRowToDense(i, perm);
    for (auto & elem : potential) {
      UInt index = elem;
      perm[index] += synPermBelowStimulusInc_;
    }
    updatePermanencesForColumn_(perm, i, false);
  }
}

void SpatialPooler::updateDutyCyclesHelper_(vector<Real>& dutyCycles,
                                     vector<UInt>& newValues,
                                     UInt period)
{
  NTA_ASSERT(period >= 1);
  NTA_ASSERT(dutyCycles.size() == newValues.size());
  for (UInt i = 0; i < dutyCycles.size(); i++) {
    dutyCycles[i] = (dutyCycles[i] * (period - 1) + newValues[i]) / period;
  }
}

void SpatialPooler::updateBoostFactors_()
{
  for (UInt i = 0; i < numColumns_; i++) {
    if (minActiveDutyCycles_[i] <= 0) {
      continue;
    }
    if (activeDutyCycles_[i] > minActiveDutyCycles_[i]) {
      boostFactors_[i] = 1.0;
      continue;
    }
    boostFactors_[i] = ((1 - maxBoost_) / minActiveDutyCycles_[i] *
                        activeDutyCycles_[i]) + maxBoost_;
  }
}

void SpatialPooler::updateBookeepingVars_(bool learn)
{
  iterationNum_++;
  if (learn) {
    iterationLearnNum_++;
  }
}

void SpatialPooler::calculateOverlap_(UInt inputVector[],
                                      vector<UInt>& overlaps)
{
  overlaps.assign(numColumns_,0);
  connectedSynapses_.rightVecSumAtNZ(inputVector,inputVector+numInputs_,
    overlaps.begin(),overlaps.end());
  if (stimulusThreshold_ > 0) {
    for (UInt i = 0; i < numColumns_; i++) {
      if (overlaps[i] < stimulusThreshold_) {
        overlaps[i] = 0;
      }
    }
  }
}

void SpatialPooler::calculateOverlapPct_(vector<UInt>& overlaps,
                                         vector<Real>& overlapPct)
{
  overlapPct.assign(numColumns_,0);
  for (UInt i = 0; i < numColumns_; i++) {
    if (connectedCounts_[i] != 0) {
      overlapPct[i] = ((Real) overlaps[i]) / connectedCounts_[i];
    } else {
      // The intent here is to see if a cell matches its input well.
      // Therefore if nothing is connected the overlapPct is set to 0.
      overlapPct[i] = 0;
    }
  }
}

// Makes a copy of overlaps
void SpatialPooler::inhibitColumns_(vector<Real>& overlaps,
                                    vector<UInt>& activeColumns)
{
  Real density = localAreaDensity_;
  if (numActiveColumnsPerInhArea_ > 0) {
    UInt inhibitionArea = pow((Real) (2 * inhibitionRadius_ + 1),
                              (Real) columnDimensions_.size());
    inhibitionArea = min(inhibitionArea, numColumns_);
    density = ((Real) numActiveColumnsPerInhArea_) / inhibitionArea;
    density = min(density, (Real) 0.5);
  }

  vector<Real> overlapsWithNoise;
  overlapsWithNoise.resize(numColumns_);
  for (UInt i = 0; i < numColumns_; i++) {
    overlapsWithNoise[i] = overlaps[i] + tieBreaker_[i];
  }

  if (globalInhibition_ ||
      inhibitionRadius_ > *max_element(columnDimensions_.begin(),
                                       columnDimensions_.end())) {
    inhibitColumnsGlobal_(overlapsWithNoise, density, activeColumns);
  } else {
    inhibitColumnsLocal_(overlapsWithNoise, density, activeColumns);
  }
}

bool SpatialPooler::isWinner_(Real score, vector<pair<UInt, Real> >& winners,
                              UInt numWinners)
{
  if (winners.size() < numWinners) {
    return true;
  }

  if (score > winners[numWinners-1].second) {
    return true;
  }

  return false;
}

void SpatialPooler::addToWinners_(UInt index, Real score,
                                  vector<pair<UInt, Real> >& winners)
{
  pair<UInt, Real> val = make_pair(index, score);
  for (auto it = winners.begin();
       it != winners.end(); it++) {
    if (score > it->second) {
      winners.insert(it, val);
      return;
    }
  }
  winners.push_back(val);
}

void SpatialPooler::inhibitColumnsGlobal_(vector<Real>& overlaps, Real density,
                                          vector<UInt>& activeColumns)
{
  activeColumns.clear();
  UInt numActive = (UInt) (density * numColumns_);
  vector<pair<UInt, Real> > winners;
  for (UInt i = 0; i < numColumns_; i++) {
    if (isWinner_(overlaps[i], winners, numActive)) {
      addToWinners_(i,overlaps[i], winners);
    }
  }

  for (UInt i = 0; i < numActive; i++) {
    activeColumns.push_back(winners[i].first);
  }

}

void SpatialPooler::inhibitColumnsLocal_(vector<Real>& overlaps, Real density,
                           vector<UInt>& activeColumns)
{
  activeColumns.clear();
  Real arbitration = *max_element(overlaps.begin(), overlaps.end()) / 1000.0;
  vector<UInt> neighbors;
  for (UInt column = 0; column < numColumns_; column++) {
    getNeighborsND_(column, columnDimensions_, inhibitionRadius_, false,
                    neighbors);
    UInt numActive = (UInt) (0.5 + (density * (neighbors.size() + 1)));
    UInt numBigger = 0;
    for (auto & neighbor : neighbors) {
      if (overlaps[neighbor] > overlaps[column]) {
        numBigger++;
      }
    }

    if (numBigger < numActive) {
      activeColumns.push_back(column);
      overlaps[column] += arbitration;
    }

  }
}

void SpatialPooler::getNeighbors1D_(UInt column, vector<UInt>& dimensions,
                     UInt radius, bool wrapAround, vector<UInt>& neighbors)
{
  NTA_ASSERT(dimensions.size() == 1);
  neighbors.clear();
  for (Int i = (Int) column - (Int) radius;
       i < (Int) column + (Int) radius + 1; i++) {

    if (i == (Int) column) {
      continue;
    }

    if (wrapAround) {
      neighbors.push_back((i + (Int) numColumns_) % numColumns_);
    } else if (i >= 0 && i < (Int) numColumns_) {
      neighbors.push_back(i);
    }
  }
}

void SpatialPooler::getNeighbors2D_(UInt column, vector<UInt>& dimensions,
                     UInt radius, bool wrapAround, vector<UInt>& neighbors)
{
  NTA_ASSERT(dimensions.size() == 2);
  neighbors.clear();

  UInt nrows = dimensions[0];
  UInt ncols = dimensions[1];

  CoordinateConverter2D conv(nrows,ncols);

  Int row = (Int) conv.toRow(column);
  Int col = (Int) conv.toCol(column);

  for (Int r = row - (Int) radius; r <= row + (Int) radius; r++) {
    for (Int c = col - (Int) radius; c <= col + (Int) radius; c++) {
      if (r == row && c == col) {
        continue;
      }

      if (wrapAround) {
        UInt index = conv.toIndex((r + nrows) % nrows, (c + ncols) % ncols);
        neighbors.push_back(index);
      } else if (r >= 0 && r < (Int) nrows && c >= 0 && c < (Int) ncols) {
        UInt index = conv.toIndex(r,c);
        neighbors.push_back(index);
      }
    }
  }
}

void SpatialPooler::cartesianProduct_(vector<vector<UInt> >& vecs,
                                      vector<vector<UInt> >& product)
{
  if (vecs.empty()) {
    return;
  }

  if (vecs.size() == 1) {
    for (auto & elem : vecs[0]) {
      vector<UInt> v;
      v.push_back(elem);
      product.push_back(v);
    }
    return;
  }

  vector<UInt> v = vecs[0];
  vecs.erase(vecs.begin());

  vector<vector<UInt> > prod;
  cartesianProduct_(vecs, prod);
  for (auto & elem : v) {
    for (auto & prod_j : prod) {
      vector<UInt> coord = prod_j;
      coord.push_back(elem);
      product.push_back(coord);
    }
  }
}

void SpatialPooler::range_(Int start, Int end, UInt ubound, bool wrapAround,
                           vector<UInt>& rangeVector)
{
  vector<Int> range;
  vector<Int>::iterator uniqueEnd;

  // Generate indices within range, wrapping around as necessary
  for (Int i = start; i <= end; i++) {
    if (wrapAround) {
      range.push_back(emod(i, (int) ubound));
    } else if (i >= 0 && i < (Int) ubound) {
      range.push_back(i);
    }
  }

  // Add the unique range indices to rangeVector
  sort(range.begin(), range.end());
  uniqueEnd = unique(range.begin(), range.end());
  range.resize(distance(range.begin(), uniqueEnd) );

  rangeVector.clear();
  rangeVector.insert(rangeVector.begin(), range.begin(), range.end());
}

void SpatialPooler::getNeighborsND_(
    UInt column, vector<UInt>& dimensions, UInt radius, bool wrapAround,
    vector<UInt>& neighbors)
{

  neighbors.clear();
  CoordinateConverterND conv(dimensions);

  vector<UInt> columnCoord;
  conv.toCoord(column,columnCoord);

  vector<vector<UInt> > rangeND;
  vector<UInt> curRange;

  for (UInt i = 0; i < dimensions.size(); i++) {
    range_((Int) columnCoord[i] - (Int) radius,
           (Int) columnCoord[i] + (Int) radius,
           dimensions[i], wrapAround, curRange);

    rangeND.insert(rangeND.begin(), curRange);
  }

  vector<vector<UInt> > neighborCoords;
  cartesianProduct_(rangeND, neighborCoords);
  for (auto & neighborCoord : neighborCoords) {
    UInt index = conv.toIndex(neighborCoord);
    if (index != column) {
      neighbors.push_back(index);
    }
  }

}

bool SpatialPooler::isUpdateRound_()
{
  return (iterationNum_ % updatePeriod_) == 0;
}

/* create a RNG with given seed */
void SpatialPooler::seed_(UInt64 seed)
{
  rng_ = Random(seed);
}

void SpatialPooler::buildMessage(capnp::MallocMessageBuilder& message)
{
  SpatialPoolerProto::Builder spProto = message.initRoot<SpatialPoolerProto>();

  spProto.setSeed(rng_.getSeed());
  spProto.setNumInputs(numInputs_);
  spProto.setNumColumns(numColumns_);

  capnp::List<UInt32>::Builder columnDims =
      spProto.initColumnDimensions(columnDimensions_.size());
  for (int i = 0; i < columnDimensions_.size(); ++i)
  {
    columnDims.set(i, columnDimensions_[i]);
  }

  capnp::List<UInt32>::Builder inputDims =
      spProto.initInputDimensions(inputDimensions_.size());
  for (int i = 0; i < inputDimensions_.size(); ++i)
  {
    inputDims.set(i, inputDimensions_[i]);
  }

  spProto.setPotentialRadius(potentialRadius_);
  spProto.setPotentialPct(potentialPct_);
  spProto.setInhibitionRadius(inhibitionRadius_);
  spProto.setGlobalInhibition(globalInhibition_);
  spProto.setNumActiveColumnsPerInhArea(numActiveColumnsPerInhArea_);
  spProto.setLocalAreaDensity(localAreaDensity_);
  spProto.setStimulusThreshold(stimulusThreshold_);
  spProto.setSynPermInactiveDec(synPermInactiveDec_);
  spProto.setSynPermActiveInc(synPermActiveInc_);
  spProto.setSynPermBelowStimulusInc(synPermBelowStimulusInc_);
  spProto.setSynPermConnected(synPermConnected_);
  spProto.setMinPctOverlapDutyCycles(minPctOverlapDutyCycles_);
  spProto.setMinPctActiveDutyCycles(minPctActiveDutyCycles_);
  spProto.setDutyCyclePeriod(dutyCyclePeriod_);
  spProto.setMaxBoost(maxBoost_);
  spProto.setWrapAround(wrapAround_);
  spProto.setSpVerbosity(spVerbosity_);

  spProto.setSynPermMin(synPermMin_);
  spProto.setSynPermMax(synPermMax_);
  spProto.setSynPermTrimThreshold(synPermTrimThreshold_);
  spProto.setUpdatePeriod(updatePeriod_);

  spProto.setVersion(version_);
  spProto.setIterationNum(iterationNum_);
  spProto.setIterationLearnNum(iterationLearnNum_);

  capnp::List<capnp::List<UInt32>>::Builder potentialPools =
      spProto.initPotentialPools(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    vector<UInt> const & pot = potentialPools_.getSparseRow(i);
    capnp::List<UInt32>::Builder pool = potentialPools.init(i, pot.size());
    for (UInt j = 0; j < pot.size(); ++j)
    {
      pool.set(j, pot[j]);
    }
  }

  capnp::List<capnp::List<SparseFloat>>::Builder permanences =
      spProto.initPermanences(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    // TODO: Get a reference to the vector rather than building a new one?
    vector<pair<UInt, Real> > perm;
    perm.resize(permanences_.nNonZerosOnRow(i));
    permanences_.getRowToSparse(i, perm.begin());

    capnp::List<SparseFloat>::Builder pool = permanences.init(i, perm.size());

    for (UInt j = 0; j < perm.size(); ++j)
    {
      SparseFloat::Builder sf = pool[j];
      sf.setIndex(perm[j].first);
      sf.setValue(perm[j].second);
    }
  }

  capnp::List<Real32>::Builder tieBreaker =
      spProto.initTieBreaker(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    tieBreaker.set(i, tieBreaker_[i]);
  }

  // Connected synapses not saved...
  // Connected counts not saved...

  capnp::List<Real32>::Builder overlapDutyCycles =
      spProto.initOverlapDutyCycles(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    overlapDutyCycles.set(i, overlapDutyCycles_[i]);
  }

  capnp::List<Real32>::Builder activeDutyCycles =
      spProto.initActiveDutyCycles(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    activeDutyCycles.set(i, activeDutyCycles_[i]);
  }

  capnp::List<Real32>::Builder minOverlapDutyCycles =
      spProto.initMinOverlapDutyCycles(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    minOverlapDutyCycles.set(i, minOverlapDutyCycles_[i]);
  }

  capnp::List<Real32>::Builder minActiveDutyCycles =
      spProto.initMinActiveDutyCycles(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    minActiveDutyCycles.set(i, minActiveDutyCycles_[i]);
  }

  capnp::List<Real32>::Builder boostFactors =
      spProto.initBoostFactors(numColumns_);
  for (UInt i = 0; i < numColumns_; ++i)
  {
    boostFactors.set(i, boostFactors_[i]);
  }

  //outStream << rng_ << endl;
}

void SpatialPooler::save(ostream& stream)
{
  capnp::MallocMessageBuilder message;
  buildMessage(message);
  proto::StdOutputStream out(stream);
  writeMessage(out, message);
}

void SpatialPooler::write(int fd)
{
  capnp::MallocMessageBuilder message;
  buildMessage(message);
  writeMessageToFd(fd, message);
}

// Implementation note: this method sets up the instance using data from
// inStream. This method does not call initialize. As such we have to be careful
// that everything in initialize is handled properly here.
void SpatialPooler::load(istream& stream)
{
  proto::StdInputStream in(stream);
  capnp::InputStreamMessageReader message(in);
  SpatialPoolerProto::Reader spProto = message.getRoot<SpatialPoolerProto>();
  loadProto(spProto);
}

void SpatialPooler::read(int fd)
{
  capnp::StreamFdMessageReader message(fd);
  SpatialPoolerProto::Reader spProto = message.getRoot<SpatialPoolerProto>();
  loadProto(spProto);
}

void SpatialPooler::loadProto(SpatialPoolerProto::Reader& spProto)
{
  rng_ = Random(spProto.getSeed());
  numInputs_ = spProto.getNumInputs();
  numColumns_ = spProto.getNumColumns();

  columnDimensions_.clear();
  for (UInt dimension : spProto.getColumnDimensions()) {
    columnDimensions_.push_back(dimension);
  }



  //// Current version
  //version_ = 2;

  //// Check the marker
  //string marker;
  //inStream >> marker;
  //NTA_CHECK(marker == "SpatialPooler");

  //// Check the saved version.
  //UInt version;
  //inStream >> version;
  //NTA_CHECK(version <= version_);  


  //// Retrieve simple variables
  //inStream >> numInputs_
  //         >> numColumns_
  //         >> potentialRadius_ 
  //         >> potentialPct_
  //         >> initConnectedPct_
  //         >> globalInhibition_ 
  //         >> numActiveColumnsPerInhArea_
  //         >> localAreaDensity_
  //         >> stimulusThreshold_
  //         >> inhibitionRadius_
  //         >> dutyCyclePeriod_
  //         >> maxBoost_
  //         >> iterationNum_
  //         >> iterationLearnNum_
  //         >> spVerbosity_
  //         >> updatePeriod_
  //          
  //         >> synPermMin_
  //         >> synPermMax_
  //         >> synPermTrimThreshold_
  //         >> synPermInactiveDec_
  //         >> synPermActiveInc_
  //         >> synPermBelowStimulusInc_
  //         >> synPermConnected_
  //         >> minPctOverlapDutyCycles_
  //         >> minPctActiveDutyCycles_;
  //if (version == 1) {
  //  wrapAround_ = true;
  //} else {
  //  inStream >> wrapAround_;
  //}

  //// Retrieve vectors.
  //UInt numInputDimensions;
  //inStream >> numInputDimensions;
  //inputDimensions_.resize(numInputDimensions);
  //for (UInt i = 0; i < numInputDimensions; i++) {
  //  inStream >> inputDimensions_[i];
  //}

  //UInt numColumnDimensions;
  //inStream >> numColumnDimensions;
  //columnDimensions_.resize(numColumnDimensions);
  //for (UInt i = 0; i < numColumnDimensions; i++) {
  //  inStream >> columnDimensions_[i];
  //}

  //boostFactors_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> boostFactors_[i];
  //}

  //overlapDutyCycles_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> overlapDutyCycles_[i];
  //}

  //activeDutyCycles_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> activeDutyCycles_[i];
  //}

  //minOverlapDutyCycles_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> minOverlapDutyCycles_[i];
  //}

  //minActiveDutyCycles_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> minActiveDutyCycles_[i];
  //}

  //tieBreaker_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  inStream >> tieBreaker_[i];
  //}


  //// Retrieve matrices.
  //potentialPools_.resize(numColumns_, numInputs_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  UInt nNonZerosOnRow;
  //  inStream >> nNonZerosOnRow;
  //  vector<UInt> pot(nNonZerosOnRow, 0);
  //  for (UInt j = 0; j < nNonZerosOnRow; j++) {
  //    inStream >> pot[j];
  //  }
  //  potentialPools_.replaceSparseRow(i,pot.begin(), pot.end());
  //}
  //
  //permanences_.resize(numColumns_, numInputs_);
  //connectedSynapses_.resize(numColumns_, numInputs_);
  //connectedCounts_.resize(numColumns_);
  //for (UInt i = 0; i < numColumns_; i++) {
  //  UInt nNonZerosOnRow;
  //  inStream >> nNonZerosOnRow;
  //  vector<Real> perm(numInputs_, 0);

  //  for (UInt j = 0; j < nNonZerosOnRow; j++) {
  //    UInt index;
  //    Real value;
  //    inStream >> index;
  //    inStream >> value;
  //    perm[index] = value;
  //  }
  //  updatePermanencesForColumn_(perm, i, false);
  //}

  //inStream >> rng_;

  //inStream >> marker;
  //NTA_CHECK(marker == "~SpatialPooler");

  //// initialize ephemeral members
  //overlaps_.resize(numColumns_);
  //overlapsPct_.resize(numColumns_);
  //boostedOverlaps_.resize(numColumns_);

}

//----------------------------------------------------------------------
// Debugging helpers
//----------------------------------------------------------------------

// Print the main SP creation parameters
void SpatialPooler::printParameters()
{
  std::cout << "------------CPP SpatialPooler Parameters ------------------\n";
  std::cout
    << "iterationNum                = " << getIterationNum() << std::endl
    << "iterationLearnNum           = " << getIterationLearnNum() << std::endl
    << "numInputs                   = " << getNumInputs() << std::endl
    << "numColumns                  = " << getNumColumns() << std::endl
    << "numActiveColumnsPerInhArea  = "
                << getNumActiveColumnsPerInhArea() << std::endl
    << "potentialPct                = " << getPotentialPct() << std::endl
    << "globalInhibition            = " << getGlobalInhibition() << std::endl
    << "localAreaDensity            = " << getLocalAreaDensity() << std::endl
    << "stimulusThreshold           = " << getStimulusThreshold() << std::endl
    << "synPermActiveInc            = " << getSynPermActiveInc() << std::endl
    << "synPermInactiveDec          = " << getSynPermInactiveDec() << std::endl
    << "synPermConnected            = " << getSynPermConnected() << std::endl
    << "minPctOverlapDutyCycles     = "
                << getMinPctOverlapDutyCycles() << std::endl
    << "minPctActiveDutyCycles      = "
                << getMinPctActiveDutyCycles() << std::endl
    << "dutyCyclePeriod             = " << getDutyCyclePeriod() << std::endl
    << "maxBoost                    = " << getMaxBoost() << std::endl
    << "spVerbosity                 = " << getSpVerbosity() << std::endl
    << "wrapAround                  = " << getWrapAround() << std::endl
    << "version                     = " << version() << std::endl;
}

void SpatialPooler::printState(vector<UInt> &state)
{
  std::cout << "[  ";
  for (UInt i = 0; i != state.size(); ++i) {
    if (i > 0 && i % 10 == 0) {
      std::cout << "\n   ";
    }
    std::cout << state[i] << " ";
  }
  std::cout << "]\n";
}

void SpatialPooler::printState(vector<Real> &state)
{
  std::cout << "[  ";
  for (UInt i = 0; i != state.size(); ++i) {
    if (i > 0 && i % 10 == 0) {
      std::cout << "\n   ";
    }
    std::printf("%6.3f ", state[i]);
  }
  std::cout << "]\n";
}

