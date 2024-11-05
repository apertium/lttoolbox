# -*- coding: utf-8 -*-
from basictest import ProcTest as _ProcTest
import unittest

class TmxProcTest(unittest.TestCase, _ProcTest):
    procdix = 'data/simple.tmx'
    procflags = []
    procdir = 'nob-nno'

    def compileDix(self, dir, dix, flags=None, binName='compiled.bin',
                   expectFail=False):
        return self.callProc('lt-tmxcomp',
                             [dir, dix, binName],
                             flags,
                             expectFail)

    def compileTest(self, tmpd):
        return self.compileDix(self.procdir, self.procdix,
                               flags=self.compflags,
                               binName=tmpd+'/compiled.bin',
                               expectFail=self.expectedCompRetCodeFail)

    def openProc(self, tmpd):
        return self.openPipe('lt-tmxproc', self.procflags+[tmpd+'/compiled.bin'])

class Simple(TmxProcTest):
    inputs = ['Ikke så merkelig.\nJa, ja.',]
    expectedOutputs = ['[Ikkje så merkeleg].\nJa, ja.']


class SimpleSpaceSep(TmxProcTest):
    procflags = ['-s']
    inputs = ['Ikke så merkelig at det skjer.',]
    expectedOutputs = ['[Ikkje så merkeleg] at det skjer.']


class Numbers(TmxProcTest):
    procdix = 'data/numbers.tmx'
    procflags = ['-s']
    inputs = [
        'kake 1 og kjeks',
        '3 og kake 8 og kjeks',
        '3 og kaffe 9 og kjeks 2',
        '3 og kaffe 9 og 7 kjeks 2',
        '3 og kaffe 9 og 2 ost ',
        '1 3 eller ost 88 eller 89 kjeks',
        '3 på halv fire',
        '1 og 3 på halv fire',
    ]
    expectedOutputs = [
        '[kake 1] og kjeks',
        '3 og [kake 8] og kjeks',
        '3 og kaffe 9 og kjeks 2',
        '3 og [kaffi 9 og 7] kjeks 2',
        '3 og [kaffi 9 og 2] ost ',
        '1 3 eller [ost 88 eller 89 kjeks]',
        '[3 på halv] fire',
        '1 og [3 på halv] fire',
    ]

@unittest.expectedFailure
class NumbersTwice(TmxProcTest):
    procdix = 'data/numbers.tmx'
    procflags = ['-s']
    inputs = [
        '3 kake 8 og kjeks 2',
        '3 kaffe 9 og kjeks 2',
        '1 3 ost 99 eller ost 88 eller 89 kjeks',
        '1 3 på halv fire',
    ]
    expectedOutputs = [
        '3 [kake 8] og kjeks 2',
        '3 kaffe 9 og kjeks 2',
        '1 3 ost 99 eller [ost 88 eller 89 kjeks]',
        '1 [3 på halv] fire',
    ]

