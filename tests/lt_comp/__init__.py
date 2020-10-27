# -*- coding: utf-8 -*-

from proctest import ProcTest
import unittest

class CompNormalAndJoin(ProcTest):
    inputs = ["abc", "ab", "y", "n", "jg", "jh", "kg"]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/n<n><ind>$", "^jg/j<pr>+g<n>$", "^jh/j<pr>+h<n>$", "^kg/k<pr>+g<n>$"]


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
