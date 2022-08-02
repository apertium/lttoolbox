import unittest
from basictest import ProcTest

class AcxTest(unittest.TestCase, ProcTest):
    dix = 'data/minimal-mono.dix'
    acx = 'data/basic.acx'
    procdir = 'lr'
    inputs = ['abc', 'ábc', 'äbc']
    expectedOutputs = ['^abc/ab<n><def>$',
                       '^ábc/ab<n><def>$',
                       '^äbc/ab<n><def>$']

    def compileTest(self, tmpd):
        ret = self.compileDix(self.procdir, self.dix,
                              binName=tmpd+'/plain.bin')
        if not ret: return ret
        self.callProc('lt-apply-acx',
                      [tmpd+'/plain.bin', self.acx, tmpd+'/compiled.bin'])
        return True
