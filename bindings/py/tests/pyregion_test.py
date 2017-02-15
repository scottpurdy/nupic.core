# ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2014-2015, Numenta, Inc.  Unless you have an agreement
# with Numenta, Inc., for a separate license for this software code, the
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
# ----------------------------------------------------------------------

import os
import sys
import unittest

from nupic.bindings.engine_internal import Network
from nupic.bindings.regions.PyRegion import PyRegion



# Classes used for testing

class X(PyRegion):
  def __init__(self):
    self.x = 5



class Y(PyRegion):
  def __init__(self):
    self.zzz = 5
    self._zzz = 3
  def initialize(self): pass
  def compute(self): pass
  def getOutputElementCount(self): pass



class Z(object):
  def __init__(self):
    y = Y()
    y.setParameter('zzz', 0, 4)



class CounterSource(PyRegion):
  def __init__(self):
    self._count = 0
  @classmethod
  def getSpec(cls):
    return {
      "description": (
        "Source region that outputs the number of times it has run, "
        "inclusive."),
      "outputs": {
        "count": {
          "description": "The number of times the region has run.",
          "dataType": "uint32",
          "count": 1,
          "regionLevel": True,
          "isDefaultOutput": True,
        },
      },
    }
  def initialize(self, inputs, outputs): pass
  def compute(self, inputs, outputs):
    self._count += 1
    outputs["count"][0] = self._count



class TestSink(PyRegion):
  def __init__(self):
    self._lastInput = None
  @classmethod
  def getSpec(cls):
    return {
      "description": (
        "Source region that outputs the number of times it has run, "
        "inclusive."),
      "inputs": {
        "inputValue": {
          "description": "The value from the input link.",
          "dataType": "uint32",
          "count": 1,
          "regionLevel": True,
          "isDefaultInput": True,
        },
      },
    }
  def initialize(self, inputs, outputs): pass
  def compute(self, inputs, outputs):
    self._lastInput = self.inputs["inputValue"][0]
  def getLastInput(self):
    return self._lastInput



class PyRegionTest(unittest.TestCase):


  def testNoInit(self):
    """Test unimplemented init method"""
    class NoInit(PyRegion):
      pass

    with self.assertRaises(TypeError) as cw:
      _ni = NoInit()

    self.assertEqual(str(cw.exception), "Can't instantiate abstract class " +
      "NoInit with abstract methods __init__, compute, initialize")


  def testUnimplementedAbstractMethods(self):
    """Test unimplemented abstract methods"""
    # Test unimplemented getSpec (results in NotImplementedError)
    with self.assertRaises(NotImplementedError):
      X.getSpec()

    # Test unimplemented abstract methods (x can't be instantiated)
    with self.assertRaises(TypeError) as cw:
      _x = X()

    self.assertEqual(str(cw.exception), "Can't instantiate abstract class " +
      "X with abstract methods compute, initialize")

  def testUnimplementedNotImplementedMethods(self):
    """Test unimplemented @not_implemented methods"""
    # Can instantiate because all abstract methods are implemented
    y = Y()

    # Can call the default getParameter() from PyRegion
    self.assertEqual(y.getParameter('zzz', -1), 5)

    # Accessing an attribute whose name starts with '_' via getParameter()
    with self.assertRaises(Exception) as cw:
      _ = y.getParameter('_zzz', -1) == 5

    self.assertEqual(str(cw.exception), "Parameter name must not " +
      "start with an underscore")

    # Calling not implemented method result in NotImplementedError
    with self.assertRaises(NotImplementedError) as cw:
      y.setParameter('zzz', 4, 5)

    self.assertEqual(str(cw.exception),
                     "The method setParameter is not implemented.")

  def testCallUnimplementedMethod(self):
    """Test calling an unimplemented method"""
    with self.assertRaises(NotImplementedError) as cw:
      _z = Z()

    self.assertEqual(str(cw.exception),
                     "The method setParameter is not implemented.")


  def testDelayedLinks(self):
    moduleDir, moduleName = os.path.split(__file__)
    moduleDir = os.path.abspath(os.path.join(os.getcwd(), moduleDir))
    moduleName = moduleName.split(".")[0]
    print "sys.path start: ", sys.path
    sys.path.append(moduleDir)
    print "sys.path before: ", sys.path
    print "registering: ", moduleName, CounterSource.__name__
    Network.registerPyRegion(moduleName, CounterSource.__name__)
    Network.registerPyRegion(moduleName, TestSink.__name__)

    net = Network()
    net.addRegion("source", "py.CounterSource", "")
    net.addRegion("sink", "py.TestSink", "")
    net.link("source", "sink", "UniformLink", "")
    network.initialize()

    del sys.path[-1]
    print "sys.path after: ", sys.path



if __name__ == "__main__":
  unittest.main()
