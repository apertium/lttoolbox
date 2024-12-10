# -*- coding: utf-8 -*-
import unittest
from basictest import ProcTest
import unittest

class MergeTest(unittest.TestCase, ProcTest):
    inputs = ['^nochange<n>$']
    expectedOutputs = ['^nochange<n>$']
    procflags = []

    def compileTest(self, tmpd):
        return True             # "pass"

    def openProc(self, tmpd):
        return self.openPipe('lt-merge', self.procflags+[])


class SimpleTest(MergeTest):
    inputs = ['^ikke/ikke<adv>$ ^«/«<lquot><MERGE_BEG>$^så/så<adv>$ ^veldig/v<adv>$^»/»<rquot><MERGE_END>$ ^bra/bra<adj>$' ]
    expectedOutputs = ['^ikke/ikke<adv>$ ^«så veldig»/«så veldig»<MERGED>$ ^bra/bra<adj>$']


class SingleTest(MergeTest):
    inputs = ['^not/very<useful><MERGE_BEG><MERGE_END>$' ]
    expectedOutputs = ['^not/not<MERGED>$']


class EscapeTest(MergeTest):
    # Using r'' to avoid doubling escapes even more:
    inputs = [r'^ikke/ikke<adv>$ ^«/«<lquot><MERGE_BEG>$^så/så<adv>$ ^ve\[dig/v<adv>$^»/»<rquot><MERGE_END>$ ^bra/bra<adj>$']
    expectedOutputs = [r'^ikke/ikke<adv>$ ^«så ve\\\[dig»/«så ve\\\[dig»<MERGED>$ ^bra/bra<adj>$']


class WordblankTest(MergeTest):
    # Using r'' to avoid doubling escapes even more:
    inputs = [r'^«/«<lquot><MERGE_BEG>$[[tf:i:a]]^ve\/ldig/v<adv>$[[/]]^»/»<rquot><MERGE_END>$']
    expectedOutputs = [r'^«\[\[tf:i:a\]\]ve\\\/ldig\[\[\/\]\]»/«\[\[tf:i:a\]\]ve\\\/ldig\[\[\/\]\]»<MERGED>$']
