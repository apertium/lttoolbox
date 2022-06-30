# -*- coding: utf-8 -*-

from basictest import BasicTest, TempDir
import unittest

class ProcTest(unittest.TestCase, BasicTest):
    """See lt_proc test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    procdix = "data/minimal-mono.dix"
    procdir = "lr"
    procflags = ["-z"]
    inputs = [""]
    expectedOutputs = [""]
    expectedRetCodeFail = False
    expectedCompRetCodeFail = False
    flushing = True

    def compileTest(self, tmpd):
        return self.compileDix(self.procdir, self.procdix,
                               binName=tmpd+'/compiled.bin',
                               expectFail=self.expectedCompRetCodeFail)

    def runTest(self):
        with TempDir() as tmpd:
            if not self.compileTest(tmpd):
                return
            if self.flushing:
                self.runTestFlush(tmpd)
            else:
                self.runTestNoFlush(tmpd)

    def openProc(self, tmpd):
        return self.openPipe('lt-proc', self.procflags+[tmpd+'/compiled.bin'])

    def runTestFlush(self, tmpd):
        proc = self.openProc(tmpd)
        self.assertEqual(len(self.inputs),
                         len(self.expectedOutputs))
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            self.assertEqual(self.communicateFlush(inp+"[][\n]", proc),
                             exp+"[][\n]")
        self.closePipe(proc, self.expectedRetCodeFail)

    def runTestNoFlush(self, tmpd):
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            proc = self.openProc(tmpd)
            self.assertEqual(proc.communicate(input=inp.encode('utf-8'))[0],
                             exp.encode('utf-8'))
            retCode = proc.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)
