# -*- coding: utf-8 -*-

import os
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree
from basictest import BasicTest
import unittest

from typing import List


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
        retCode = call([os.environ['LTTOOLBOX_PATH']+"/lt-comp",
                        self.procdir,
                        self.procdix,
						tmpd+"/compiled.bin"],
					   stdout=PIPE, stderr=PIPE)
        if self.expectedCompRetCodeFail:
            self.assertNotEqual(retCode, 0)
        else:
            self.assertEqual(retCode, 0)
        return retCode == 0

    def runTest(self):
        tmpd = mkdtemp()
        try:
            if not self.compileTest(tmpd):
                return
            if self.flushing:
                self.runTestFlush(tmpd)
            else:
                self.runTestNoFlush(tmpd)
        finally:
            rmtree(tmpd)

    def openProc(self, tmpd):
        return Popen([os.environ['LTTOOLBOX_PATH']+"/lt-proc"]
                     + self.procflags
                     + [tmpd+"/compiled.bin"],
                     stdin=PIPE,
                     stdout=PIPE,
                     stderr=PIPE)

    def runTestFlush(self, tmpd):
        proc = self.openProc(tmpd)
        self.assertEqual(len(self.inputs),
                         len(self.expectedOutputs))
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            self.assertEqual(self.communicateFlush(inp+"[][\n]", proc),
                             exp+"[][\n]")
        proc.communicate()  # let it terminate
        proc.stdin.close()
        proc.stdout.close()
        proc.stderr.close()
        retCode = proc.poll()
        if self.expectedRetCodeFail:
            self.assertNotEqual(retCode, 0)
        else:
            self.assertEqual(retCode, 0)

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
