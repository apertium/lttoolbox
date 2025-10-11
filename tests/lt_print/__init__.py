# -*- coding: utf-8 -*-
import unittest
from basictest import PrintTest


class NonWeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/biproc-skips-tags-mono.dix"
    printdir = "lr"
    expectedOutput = "0\t1\tv\tv\t0.000000\n1\t2\ti\ti\t0.000000\n2\t3\th\th\t0.000000\n3\t4\tk\tk\t0.000000\n4\t5\ti\ti\t0.000000\n5\t6\t<KEPT>\t<KEPT>\t0.000000\n6\t10\t\u03b5\t\u03b5\t0.000000\n6\t7\t<MATCHSOFAR>\t<MATCHSOFAR>\t0.000000\n7\t8\t<STILLMATCHING>\t<STILLMATCHING>\t0.000000\n8\t9\t<NONMATCHL>\t<NONMATCHR>\t0.000000\n9\t10\t\u03b5\t\u03b5\t0.000000\n10\t0.000000\n"


class WeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/cat-weight.att"
    printdir = "lr"
    expectedOutput = "0\t1\tc\tc\t4.567895\n1\t2\ta\ta\t0.989532\n2\t3\tt\tt\t2.796193\n3\t4\tε\t+\t0.824564\n4\t5\tε\tn\t1.824564\n4\t5\tε\tv\t2.856296\n5\t0.525487\n"


class NegativeWeightedFst(unittest.TestCase, PrintTest):
    printdix = "data/cat-weight-negative.att"
    printdir = "lr"
    expectedOutput = "0\t1\tc\tc\t4.567895\n1\t2\ta\ta\t0.989532\n2\t3\tt\tt\t2.796193\n3\t4\tε\t+\t-0.824564\n4\t5\tε\tn\t1.824564\n4\t5\tε\tv\t2.856296\n5\t-0.525487\n"


class MulticharCompFst(unittest.TestCase, PrintTest):
    printdix = "data/multichar.att"
    printdir = "lr"
    expectedOutput = "0\t1\tא\tא\t0.000000\n1\t2\tε\tַ\t0.000000\n2\t3\tε\tן\t0.000000\n3\t4\tε\t<blah>\t0.000000\n4\t0.000000\n"


class SectionsFst(unittest.TestCase, PrintTest):
    printdix = "data/sections.dix"
    printdir = "lr"
    expectedOutput = """0\t1\t.\t.\t0.000000
1\t2\tε\t<sent>\t0.000000
2\t0.000000
--
0\t1\tX\tX\t0.000000
1\t2\tε\t<np>\t0.000000
2\t0.000000
"""


class Alphabet(unittest.TestCase, PrintTest):
    printdix = "data/alphabet.att"
    printdir = "lr"
    printflags = ["-a"]
    expectedOutput = """A
B
C
a
b
c
<h>
"""
