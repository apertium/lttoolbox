# -*- coding: utf-8 -*-

from basictest import ProcTest, PrintTest
import unittest

class CompNormalAndJoin(unittest.TestCase, ProcTest):
    inputs = ["abc", "ab", "y", "n", "jg", "jh", "kg"]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/n<n><ind>$", "^jg/j<pr>+g<n>$", "^jh/j<pr>+h<n>$", "^kg/k<pr>+g<n>$"]


class EmptyDixOk(unittest.TestCase, ProcTest):
	procdix = "data/entirely-empty.dix"
	inputs = ["abc"]
	expectedOutputs = ["^abc/*abc$"]


class CompEmptyLhsShouldError(unittest.TestCase, ProcTest):
    procdix = "data/lhs-empty-mono.dix"
    expectedCompRetCodeFail = True


class CompEmptyRhsShouldError(unittest.TestCase, ProcTest):
    procdir = "rl"
    procdix = "data/rhs-empty-mono.dix"
    expectedCompRetCodeFail = True


class CompLhsInitialSpaceShouldError(unittest.TestCase, ProcTest):
    procdix = "data/lhs-ws-mono.dix"
    expectedCompRetCodeFail = True


class CompRhsInitialSpaceShouldError(unittest.TestCase, ProcTest):
    procdix = "data/rhs-ws-mono.dix"
    procdir = "rl"
    expectedCompRetCodeFail = True


class CompAttEpsilonLoopShouldError(unittest.TestCase, ProcTest):
    procdix = "data/cat-epsilon-loop.att"
    expectedCompRetCodeFail = True

class CompAttEpsilonToFinalShouldError(unittest.TestCase, ProcTest):
    procdix = "data/cat-epsilon-to-final.att"
    expectedCompRetCodeFail = True

class CompSplitMultichar(unittest.TestCase, ProcTest):
    procdix = "data/multichar.att"
    inputs = ["א"]
    expectedOutputs = ["^א/אַן<blah>$"]

class CompLSX(unittest.TestCase, PrintTest):
    printdix = "data/basic.lsx"
    expectedOutput = '''0	1	<ANY_CHAR>	<ANY_CHAR>	0.000000
1	1	<ANY_CHAR>	<ANY_CHAR>	0.000000
1	2	<vblex>	<vblex>	0.000000
2	3	<ANY_TAG>	<ANY_TAG>	0.000000
3	3	<ANY_TAG>	<ANY_TAG>	0.000000
3	4	<$>	<prn>	0.000000
4	5	p	<$>	0.000000
5	6	r	ε	0.000000
6	7	p	ε	0.000000
7	8	e	ε	0.000000
8	9	r	ε	0.000000
9	10	s	ε	0.000000
10	11	<prn>	ε	0.000000
11	12	<$>	ε	0.000000
12	14	ε	ε	0.000000
12	13	<$>	<$>	0.000000
13	14	ε	ε	0.000000
14	0.000000
'''


class VariantNoTest(unittest.TestCase, ProcTest):
    procdix = 'data/variants.dix'
    procdir = 'lr'
    compflags = []
    inputs = ['y']
    expectedOutputs = ['^y/*y$']


class VariantHoTest(unittest.TestCase, ProcTest):
    procdix = 'data/variants.dix'
    procdir = 'lr'
    compflags = ['--var-right=ho']
    inputs = ['y']
    expectedOutputs = ['^y/y<n><ind>$']


class RestrictTest(unittest.TestCase, ProcTest):
    procdix = 'data/variants.dix'
    procdir = 'lr'
    restrictflags = []
    inputs = ['abc', 'ab']
    expectedOutputs = ['^abc/ab<n><def>$', '^ab/*ab$']

    def compileTest(self, tmpd):
        ret = self.compileDix('u', self.procdix, binName=tmpd+'/uni.bin')
        if not ret: return ret
        self.callProc('lt-restrict',
                      [self.procdir, tmpd+'/uni.bin', tmpd+'/compiled.bin'],
                      self.restrictflags)

class RestrictRL1(RestrictTest):
    procdir = 'rl'
    restrictflags = ['-v', 'gascon']
    inputs = ['abc', 'ab']
    expectedOutputs = ['^abc/*abc$', '^ab/ab<n><ind>$']

class RestrictRL2(RestrictTest):
    procdir = 'rl'
    restrictflags = ['-v', 'oci']
    inputs = ['abc', 'ab']
    expectedOutputs = ['^abc/*abc$', '^ab/abbb<n><ind>$']

class ConflictingEntryWeights(ProcTest):
    procflags = ['-W']
    procdix = 'data/more-entry-weights.dix'
    inputs = ['house']
    expectedOutputs = ['^house/house<n><sg><W:1.000000>/house<vblex><pres><W:2.000000>/house<vblex><inf><W:3.000000>/house<vblex><imp><W:4.000000>$']
