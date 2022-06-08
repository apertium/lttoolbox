# -*- coding: utf-8 -*-

from proctest import ProcTest
from printtest import PrintTest
import unittest

class CompNormalAndJoin(ProcTest):
    inputs = ["abc", "ab", "y", "n", "jg", "jh", "kg"]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/n<n><ind>$", "^jg/j<pr>+g<n>$", "^jh/j<pr>+h<n>$", "^kg/k<pr>+g<n>$"]


class EmptyDixOk(ProcTest):
	procdix = "data/entirely-empty.dix"
	inputs = ["abc"]
	expectedOutputs = ["^abc/*abc$"]


class CompEmptyLhsShouldError(ProcTest):
    procdix = "data/lhs-empty-mono.dix"
    expectedCompRetCodeFail = True


class CompEmptyRhsShouldError(ProcTest):
    procdir = "rl"
    procdix = "data/rhs-empty-mono.dix"
    expectedCompRetCodeFail = True


class CompLhsInitialSpaceShouldError(ProcTest):
    procdix = "data/lhs-ws-mono.dix"
    expectedCompRetCodeFail = True


class CompRhsInitialSpaceShouldError(ProcTest):
    procdix = "data/rhs-ws-mono.dix"
    procdir = "rl"
    expectedCompRetCodeFail = True


class CompAttEpsilonLoopShouldError(ProcTest):
    procdix = "data/cat-epsilon-loop.att"
    expectedCompRetCodeFail = True

class CompAttEpsilonToFinalShouldError(ProcTest):
    procdix = "data/cat-epsilon-to-final.att"
    expectedCompRetCodeFail = True

class CompSplitMultichar(ProcTest):
    procdix = "data/multichar.att"
    inputs = ["א"]
    expectedOutputs = ["^א/אַן<blah>$"]

class CompLSX(unittest.TestCase, PrintTest):
    printdix = "data/basic.lsx"
    expectedOutput = '''0	1	<ANY_CHAR>	<ANY_CHAR>	0.000000\t
1	1	<ANY_CHAR>	<ANY_CHAR>	0.000000\t
1	2	<vblex>	<vblex>	0.000000\t
2	3	<ANY_TAG>	<ANY_TAG>	0.000000\t
3	3	<ANY_TAG>	<ANY_TAG>	0.000000\t
3	4	<$>	<prn>	0.000000\t
4	5	p	<$>	0.000000\t
5	6	r	ε	0.000000\t
6	7	p	ε	0.000000\t
7	8	e	ε	0.000000\t
8	9	r	ε	0.000000\t
9	10	s	ε	0.000000\t
10	11	<prn>	ε	0.000000\t
11	12	<$>	ε	0.000000\t
12	14	ε	ε	0.000000\t
12	13	<$>	<$>	0.000000\t
13	14	ε	ε	0.000000\t
14	0.000000
'''
