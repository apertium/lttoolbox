# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from printtest import PrintTest

class NonWeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/biproc-skips-tags-mono.dix"
    printdir = "lr"
    expectedOutput = "0\t1\tv\tv\t0.000000\t\n1\t2\ti\ti\t0.000000\t\n2\t3\th\th\t0.000000\t\n3\t4\tk\tk\t0.000000\t\n4\t5\ti\ti\t0.000000\t\n5\t6\t<KEPT>\t<KEPT>\t0.000000\t\n6\t10\t\u03b5\t\u03b5\t0.000000\t\n6\t7\t<MATCHSOFAR>\t<MATCHSOFAR>\t0.000000\t\n7\t8\t<STILLMATCHING>\t<STILLMATCHING>\t0.000000\t\n8\t9\t<NONMATCHL>\t<NONMATCHR>\t0.000000\t\n9\t10\t\u03b5\t\u03b5\t0.000000\t\n10\t0.000000\n"

class WeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/cat-weight.att"
    printdir = "lr"
    expectedOutput = "0\t1\tc\tc\t4.567895\t\n1\t2\ta\ta\t0.989532\t\n2\t3\tt\tt\t2.796193\t\n3\t4\tε\t+\t0.824564\t\n4\t5\tε\tn\t1.824564\t\n4\t5\tε\tv\t2.856296\t\n5\t0.525487\n"

class NegativeWeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/cat-weight-negative.att"
    printdir = "lr"
    expectedOutput = "0\t1\tc\tc\t4.567895\t\n1\t2\ta\ta\t0.989532\t\n2\t3\tt\tt\t2.796193\t\n3\t4\tε\t+\t-0.824564\t\n4\t5\tε\tn\t1.824564\t\n4\t5\tε\tv\t2.856296\t\n5\t-0.525487\n"
