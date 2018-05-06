# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from printtest import PrintTest

class WeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/biproc-skips-tags-mono.dix"
    printdir = "lr"
    expectedOutput = "0\t1\tv\tv\t0.000000\t\n1\t2\ti\ti\t0.000000\t\n2\t3\th\th\t0.000000\t\n3\t4\tk\tk\t0.000000\t\n4\t5\ti\ti\t0.000000\t\n5\t6\t<KEPT>\t<KEPT>\t0.000000\t\n6\t10\t\u03b5\t\u03b5\t0.000000\t\n6\t7\t<MATCHSOFAR>\t<MATCHSOFAR>\t0.000000\t\n7\t8\t<STILLMATCHING>\t<STILLMATCHING>\t0.000000\t\n8\t9\t<NONMATCHL>\t<NONMATCHR>\t0.000000\t\n9\t10\t\u03b5\t\u03b5\t0.000000\t\n10\t0.000000\n"
