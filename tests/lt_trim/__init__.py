# -*- coding: utf-8 -*-
from __future__ import unicode_literals

# If you have HFST installed, you can diff lttoolbox binaries like this:
# $ lt-print full.bin | sed 's/ /@_SPACE_@/g' | hfst-txt2fst -e ε | hfst-fst2strings -c1 > full.strings 
# $ lt-print trim.bin | sed 's/ /@_SPACE_@/g' | hfst-txt2fst -e ε | hfst-fst2strings -c1 > trim.strings 
# $ diff -y full.strings trim.strings | less
# This is similar to diffing the lt-expand of uncompiled XML dictionaries.
# See also `man hfst-fst2strings'.

from proctest import ProcTest
import unittest

from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree

class TrimProcTest(ProcTest):
    monodix = "data/minimal-mono.dix"
    monodir = "lr"
    bidix = "data/minimal-bi.dix"
    bidir = "lr"
    procflags = ["-z"]

    def compileTest(self, tmpd):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.monodir,
                                  self.monodix,
                                  tmpd+"/mono.bin"],
                                 stdout=PIPE))
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.bidir,
                                  self.bidix,
                                  tmpd+"/bi.bin"],
                                 stdout=PIPE))
        self.assertEqual(0, call(["../lttoolbox/lt-trim",
                                  tmpd+"/mono.bin",
                                  tmpd+"/bi.bin",
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))


class TrimNormalAndJoin(unittest.TestCase, TrimProcTest):
    inputs = ["abc", "ab", "y", "n", "jg", "jh", "kg"]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/*n$", "^jg/j<pr>+g<n>$", "^jh/*jh$", "^kg/*kg$"]

class TrimCmp(unittest.TestCase, TrimProcTest):
    inputs = ["a", "b", "c", "d", "aa", "ab", "ac", "ad", "ba", "bb", "bc", "bd", "ca", "cb", "cc", "cd", "da", "db", "dc", "dd", ]
    expectedOutputs = ["^a/*a$", "^b/b<n>$", "^c/*c$", "^d/d<n>$", "^aa/*aa$", "^ab/a<n>+b<n>$", "^ac/*ac$", "^ad/a<n>+d<n>$", "^ba/*ba$", "^bb/*bb$", "^bc/*bc$", "^bd/*bd$", "^ca/*ca$", "^cb/d<n>+b<n>$", "^cc/*cc$", "^cd/d<n>+d<n>$", "^da/*da$", "^db/*db$", "^dc/*dc$", "^dd/*dd$"]
    monodix = "data/cmp-mono.dix"
    bidix = "data/cmp-bi.dix"
    procflags = ["-e", "-z"]

class TrimLongleft(unittest.TestCase, TrimProcTest):
    inputs = ["herdende"]
    expectedOutputs = ["^herdende/herde<adj><pprs>$"]
    monodix = "data/longleft-mono.dix"
    bidix = "data/longleft-bi.dix"

class DivergingPaths(unittest.TestCase, TrimProcTest):
    inputs = ["xa ya"]
    expectedOutputs = ["^xa/*xa$ ^ya/ya<vblex>$"]
    monodix = "data/diverging-paths-mono.dix"
    bidix = "data/diverging-paths-bi.dix"

class MergingPaths(unittest.TestCase, TrimProcTest):
    inputs = ["en ei"]
    expectedOutputs = ["^en/en<det><qnt><m><sg>$ ^ei/en<det><qnt><f><sg>$"]
    monodix = "data/merging-paths-mono.dix"
    bidir = "rl"
    bidix = "data/merging-paths-bi.dix"

class BidixPardef(unittest.TestCase, TrimProcTest):
    inputs = ["c"]
    expectedOutputs = ["^c/c<vblex><inf>$"]
    monodix = "data/bidixpardef-mono.dix"
    bidir = "rl"
    bidix = "data/bidixpardef-bi.dix"

class UnbalancedEpsilons(unittest.TestCase, TrimProcTest):
    inputs = ["re", "rer", "res", "ret"]
    expectedOutputs = ["^re/re<vblex><inf>$", "^rer/re<vblex><pres>$", "^res/re<vblex><pres>$", "^ret/re<vblex><pret>$"]
    monodix = "data/unbalanced-epsilons-mono.dix"
    bidir = "rl"
    bidix = "data/unbalanced-epsilons-bi.dix"

class LeftUnbalancedEpsilons(unittest.TestCase, TrimProcTest):
    inputs = ["a"]
    expectedOutputs = ["^a/a<adv>$"]
    monodix = "data/left-unbalanced-epsilons-mono.dix"
    bidir = "rl"
    bidix = "data/left-unbalanced-epsilons-bi.dix"

class Group(unittest.TestCase, TrimProcTest):
    inputs = ["abc", "pq", "pqr", "pqs", "xyz"]
    expectedOutputs = ["^abc/ab<n><ind>#c$", "^pq/pq<n><ind>$", "^pqr/pq<n><ind>#r$", "^pqs/*pqs$", "^xyz/*xyz$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"

class GroupUnbalancedEpsilons(unittest.TestCase, TrimProcTest):
    inputs = ["def"]
    expectedOutputs = ["^def/de<n><f><sg>#f$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"

class BothJoinAndGroup(unittest.TestCase, TrimProcTest):
    inputs = ["jkl", "jkm", "jnl"]
    expectedOutputs = ["^jkl/j<n><ind>+k<n><ind>#l$", "^jkm/*jkm$", "^jnl/*jnl$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"


class FinalEpsilons(unittest.TestCase, TrimProcTest):
    inputs = ["ea"]
    expectedOutputs = ["^ea/e<n>#a$"]
    monodix = "data/final-epsilons-mono.dix"
    bidix = "data/final-epsilons-bi.dix"

class BidixEpsilons(unittest.TestCase, TrimProcTest):
    inputs = ["aa ba"]
    expectedOutputs = ["^aa/aa<vblex><pp>$ ^ba/*ba$"]
    monodix = "data/bidix-epsilons-mono.dix"
    bidix = "data/bidix-epsilons-bi.dix"
    bidir = "rl"

class AlphabeticAfterGroup(unittest.TestCase, TrimProcTest):
    inputs = ["as"]
    expectedOutputs = ["^as/*as$"]
    monodix = "data/alphabetic-after-group-mono.dix"
    bidix = "data/alphabetic-after-group-bi.dix"
    bidir = "lr"

class DoubleClitics(unittest.TestCase, TrimProcTest):
    inputs = ["a-b-c d"]
    expectedOutputs = ["^a-b-c d/a<vblex><ger>+b<prn><enc>+c<prn><enc># d$"]
    monodix = "data/double-clitics-mono.dix"
    bidix = "data/double-clitics-bi.dix"
    bidir = "lr"


class GroupAfterJoin(unittest.TestCase, TrimProcTest):
    "https://sourceforge.net/p/apertium/tickets/117/"
    inputs = ["notG a"]
    expectedOutputs = ["^notG/notG<vblex><inf>$ ^a/*a$"]
    monodix = "data/group-after-join-mono.dix"
    bidix = "data/group-after-join-bi.dix"
    bidir = "lr"


class Empty(unittest.TestCase, TrimProcTest):
    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                      "lr",
                                      "data/empty-mono.dix",
                                      tmpd+"/empty-mono.bin"],
                                     stdout=PIPE))
            self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                      "rl", # rl!
                                      "data/empty-bi.dix",
                                      tmpd+"/empty-bi.bin"],
                                     stdout=PIPE))
            self.assertEqual(1, call(["../lttoolbox/lt-trim",
                                      tmpd+"/empty-mono.bin",
                                      tmpd+"/empty-bi.bin",
                                      tmpd+"/empty-trimmed.bin"],
                                     stdout=PIPE,
                                     stderr=PIPE))

        finally:
            rmtree(tmpd)
