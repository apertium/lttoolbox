# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from weighttest import WeightTest

class WeigthedTransducerMatchSameInput(unittest.TestCase, WeightTest):
    weightlists = ["data/weightlists/default"]
    inputs = ["ab",
              "ABC jg",
              "jh kg",
              "y n"]
    expectedOutputs = ["^ab/ab<n><ind>$",
                       "^ABC/AB<n><def>$ ^jg/j<pr>+g<n>$",
                       "^jh/j<pr>+h<n>$ ^kg/k<pr>+g<n>$",
                       "^y/y<n><ind>$ ^n/n<n><ind>$"]


class WeigthedTransducerMatchSameInputWithCorrectWeight(unittest.TestCase, WeightTest):
    # TODO: Investigate why is this case failing?
    # Is hfst approximating the floating values?
    weightlists = ["data/weightlists/default"]
    procflags = ["-z", "-W"]
    inputs = ["ab"]
    expectedOutputs = ["^ab/ab<n><ind><W:0.005000>$"]

class MultipleWeightlists(unittest.TestCase, WeightTest):
    # Fails because lt-print fails to print the base un-weighted fst
    compdix = "data/cat-multiple-fst.att"
    weightlists = ["data/weightlists/tags-cat-multiple-fst",
                   "data/weightlists/default"]
    procflags = ["-z", "-W"]
    inputs = ["cat",
              "cats"]
    expectedOutputs = ["^cat/cat+n<W:0.004000>/cat+v<W:0.004500>$",
                       "^cats/cat+n+<pl><W:0.005000>$"]

class WeightAnalysisUsingTheFirstMatchingWeightlist(unittest.TestCase, WeightTest):
    compdix = 'data/cmp-mono.dix'
    weightlists =["data/weightlists/analyses-comp-mono",
                  "data/weightlists/tags-comp-mono"]
    inputs = ["b"]
    expectedOutputs = ["^b/b<n><compound-R>/b<pr><compound-R>$"]
