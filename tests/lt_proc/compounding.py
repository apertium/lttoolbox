# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from proctest import CompilingProcTest


class CompoundRTest(unittest.TestCase, CompilingProcTest):
    monodix = "data/cp-only-R.dix"
    monodir = "lr"
    procflags = ["-e", "-z"]
    inputs = ["est",
              "sturest"]
    expectedOutputs = ["^est/est<n><m><sg><nom><ind>$",
                       "^sturest/stur<adj><sint><pst><mf><sg><nom><ind>+est<n><m><sg><nom><ind>$"]

class CompoundOnlyRTest(unittest.TestCase, CompilingProcTest):
    monodix = "data/cp-only-R.dix"
    monodir = "lr"
    procflags = ["-e", "-z"]
    inputs = ["kulla",
              "stur",
              "sturkull",
              "sturkulla",
              "kull"]
    expectedOutputs = ["^kulla/kulla<n><f><sg><nom><ind>$",
                       "^stur/stur<adj><sint><pst><mf><sg><nom><ind>$",
                       "^sturkull/stur<adj><sint><pst><mf><sg><nom><ind>+kulla<n><f><sg><nom><ind>$",
                       "^sturkulla/*sturkulla$",
                       "^kull/*kull$"]

