# -*- coding: utf-8 -*-
# If you have HFST installed, you can diff lttoolbox binaries like this:
# $ lt-print -H full.bin | hfst-txt2fst | hfst-fst2strings -c1 > full.strings
# $ lt-print -H trim.bin | hfst-txt2fst | hfst-fst2strings -c1 > trim.strings
# $ diff -y full.strings trim.strings | less
# This is similar to diffing the lt-expand of uncompiled XML dictionaries.
# See also `man hfst-fst2strings'.

from basictest import ProcTest, TempDir
import unittest


class TrimProcTest(unittest.TestCase, ProcTest):
    monodix = "data/minimal-mono.dix"
    monodir = "lr"
    bidix = "data/minimal-bi.dix"
    bidir = "lr"
    procflags = ["-z"]

    def compileTest(self, tmpd):
        self.compileDix(self.monodir, self.monodix, binName=tmpd+'/mono.bin')
        self.compileDix(self.bidir, self.bidix, binName=tmpd+'/bi.bin')
        self.callProc('lt-trim', [tmpd+"/mono.bin",
                                  tmpd+"/bi.bin",
                                  tmpd+"/compiled.bin"])
        # The above already asserts retcode, so if we got this far we know it
        # compiled fine:
        return True


class TrimNormalAndJoin(TrimProcTest):
    inputs = ["abc", "ab", "y", "n", "jg", "jh", "kg"]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/*n$", "^jg/j<pr>+g<n>$", "^jh/*jh$", "^kg/*kg$"]

class TrimCmp(TrimProcTest):
    inputs = ["a", "b", "c", "d", "aa", "ab", "ac", "ad", "ba", "bb", "bc", "bd", "ca", "cb", "cc", "cd", "da", "db", "dc", "dd", ]
    expectedOutputs = ["^a/*a$", "^b/b<n>$", "^c/*c$", "^d/d<n>$", "^aa/*aa$", "^ab/a<n>+b<n>$", "^ac/*ac$", "^ad/a<n>+d<n>$", "^ba/*ba$", "^bb/*bb$", "^bc/*bc$", "^bd/*bd$", "^ca/*ca$", "^cb/d<n>+b<n>$", "^cc/*cc$", "^cd/d<n>+d<n>$", "^da/*da$", "^db/*db$", "^dc/*dc$", "^dd/*dd$"]
    monodix = "data/cmp-mono.dix"
    bidix = "data/cmp-bi.dix"
    procflags = ["-e", "-z"]

class TrimLongleft(TrimProcTest):
    inputs = ["herdende"]
    expectedOutputs = ["^herdende/herde<adj><pprs>$"]
    monodix = "data/longleft-mono.dix"
    bidix = "data/longleft-bi.dix"

class DivergingPaths(TrimProcTest):
    inputs = ["xa ya"]
    expectedOutputs = ["^xa/*xa$ ^ya/ya<vblex>$"]
    monodix = "data/diverging-paths-mono.dix"
    bidix = "data/diverging-paths-bi.dix"

class MergingPaths(TrimProcTest):
    inputs = ["en ei"]
    expectedOutputs = ["^en/en<det><qnt><m><sg>$ ^ei/en<det><qnt><f><sg>$"]
    monodix = "data/merging-paths-mono.dix"
    bidir = "rl"
    bidix = "data/merging-paths-bi.dix"

class BidixPardef(TrimProcTest):
    inputs = ["c"]
    expectedOutputs = ["^c/c<vblex><inf>$"]
    monodix = "data/bidixpardef-mono.dix"
    bidir = "rl"
    bidix = "data/bidixpardef-bi.dix"

class UnbalancedEpsilons(TrimProcTest):
    inputs = ["re", "rer", "res", "ret"]
    expectedOutputs = ["^re/re<vblex><inf>$", "^rer/re<vblex><pres>$", "^res/re<vblex><pres>$", "^ret/re<vblex><pret>$"]
    monodix = "data/unbalanced-epsilons-mono.dix"
    bidir = "rl"
    bidix = "data/unbalanced-epsilons-bi.dix"

class LeftUnbalancedEpsilons(TrimProcTest):
    inputs = ["a"]
    expectedOutputs = ["^a/a<adv>$"]
    monodix = "data/left-unbalanced-epsilons-mono.dix"
    bidir = "rl"
    bidix = "data/left-unbalanced-epsilons-bi.dix"

class Group(TrimProcTest):
    inputs = ["abc", "pq", "pqr", "pqs", "xyz"]
    expectedOutputs = ["^abc/ab<n><ind>#c$", "^pq/pq<n><ind>$", "^pqr/pq<n><ind>#r$", "^pqs/*pqs$", "^xyz/*xyz$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"

class GroupUnbalancedEpsilons(TrimProcTest):
    inputs = ["def"]
    expectedOutputs = ["^def/de<n><f><sg>#f$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"

class BothJoinAndGroup(TrimProcTest):
    inputs = ["jkl", "jkm", "jnl"]
    expectedOutputs = ["^jkl/j<n><ind>+k<n><ind>#l$", "^jkm/*jkm$", "^jnl/*jnl$"]
    monodix = "data/group-mono.dix"
    bidix = "data/group-bi.dix"

class FinalEpsilons(TrimProcTest):
    inputs = ["ea"]
    expectedOutputs = ["^ea/e<n>#a$"]
    monodix = "data/final-epsilons-mono.dix"
    bidix = "data/final-epsilons-bi.dix"

class BidixEpsilons(TrimProcTest):
    inputs = ["aa ba"]
    expectedOutputs = ["^aa/aa<vblex><pp>$ ^ba/*ba$"]
    monodix = "data/bidix-epsilons-mono.dix"
    bidix = "data/bidix-epsilons-bi.dix"
    bidir = "rl"

class AlphabeticAfterGroup(TrimProcTest):
    inputs = ["as"]
    expectedOutputs = ["^as/*as$"]
    monodix = "data/alphabetic-after-group-mono.dix"
    bidix = "data/alphabetic-after-group-bi.dix"
    bidir = "lr"

class DoubleClitics(TrimProcTest):
    inputs = ["a-b-c d"]
    expectedOutputs = ["^a-b-c d/a<vblex><ger>+b<prn><enc>+c<prn><enc># d$"]
    monodix = "data/double-clitics-mono.dix"
    bidix = "data/double-clitics-bi.dix"
    bidir = "lr"

class GroupAfterJoin(TrimProcTest):
    "https://sourceforge.net/p/apertium/tickets/117/"
    inputs = ["notG a"]
    expectedOutputs = ["^notG/notG<vblex><inf>$ ^a/*a$"]
    monodix = "data/group-after-join-mono.dix"
    bidix = "data/group-after-join-bi.dix"
    bidir = "lr"

class Empty(TrimProcTest):
    def runTest(self):
        with TempDir() as tmpd:
            self.compileDix('lr', 'data/empty-mono.dix',
                            binName=tmpd+'/empty-mono.bin')
            self.compileDix('rl', 'data/empty-bi.dix',
                            binName=tmpd+'/empty-bi.bin')
            self.callProc('lt-trim', [tmpd+"/empty-mono.bin",
                                      tmpd+"/empty-bi.bin",
                                      tmpd+"/empty-trimmed.bin"],
                          expectFail=True)

class PlusLemma(TrimProcTest):
    monodix = 'data/plus-lemma-mono.dix'
    bidix = 'data/plus-lemma-bi.dix'
    bidir = 'lr'
    inputs = ['abc', 'I+D', 'jg']
    expectedOutputs = ['^abc/ab<n><def>$',
                       '^I+D/I+D<n><acr><f><sg>$',
                       '^jg/j<pr>+g<n>$']
