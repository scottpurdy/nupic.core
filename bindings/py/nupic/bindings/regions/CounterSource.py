# ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2017, Numenta, Inc.  Unless you have an agreement
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

"""Simple region that outputs a count of the number of times it's run."""

from nupic.bindings.regions.PyRegion import PyRegion



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
