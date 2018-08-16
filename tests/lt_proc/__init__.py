# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from proctest import ProcTest

class ValidInput(unittest.TestCase, ProcTest):
    inputs = ["ab",
              "ABC jg",
              "y n"]
    expectedOutputs = ["^ab/ab<n><ind>$",
                       "^ABC/AB<n><def>$ ^jg/j<pr>+g<n>$",
                       "^y/y<n><ind>$ ^n/n<n><ind>$"]

class BiprocSkipTags(unittest.TestCase, ProcTest):
    procdix = "data/biproc-skips-tags-mono.dix"
    procflags = ["-b", "-z"]
    inputs = ["^vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>$"]
    expectedOutputs = ["^vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>/vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>$"]

class WeightedTransducer(unittest.TestCase, ProcTest):
    procdix = "data/walk-weight.att"
    inputs = ["walk",
              "walks"]
    expectedOutputs = ["^walk/*walk$",
                       "^walks/*walks$"]

class WeightedCatTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n/cat+v$"]

class WeightedCatInitialTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-initial.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatMiddleTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-middle.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatFinalTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-final.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatHeavyTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-heavy.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatNegativeTransducer(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-negative.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n/cat+v$"]

class PrintWeights(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight.att"
    procflags = ["-W"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n<W:11.002748>/cat+v<W:12.034480>$"]

class PrintWeightsNegative(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight-negative.att"
    procflags = ["-W"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n<W:9.353620>/cat+v<W:10.385352>$"]

class PrintNAnalyses(unittest.TestCase, ProcTest):
    procdix = "data/cat-weight.att"
    procflags = ["-N 1"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n$"]

class LemmaEntryWeights(unittest.TestCase, ProcTest):
    procdix = "data/lemma-entry-weights.dix"
    procflags = ["-W"]
    inputs = ["walk"]
    expectedOutputs = ["^walk/walk<n><W:0.100000>/walk<vblex><W:0.900000>$"]

class AllEntryWeights(unittest.TestCase, ProcTest):
    procdix = "data/entry-weights.dix"
    procflags = ["-W"]
    inputs = ["nanow"]
    expectedOutputs = ["^nanow/nan<n><ma><du><gen><W:32.120000>/nan<n><ma><du><acc><W:34.120000>/nan<n><ma><pl><gen><W:39.120000>/nan<n><ma><pl><acc><W:41.120000>$"]
# These fail on some systems:
#from null_flush_invalid_stream_format import *
