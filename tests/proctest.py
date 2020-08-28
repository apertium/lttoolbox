# -*- coding: utf-8 -*-

import os
import itertools
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree
from basictest import BasicTest


class ProcTest(BasicTest):
    """See lt_proc test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    procdix = "data/minimal-mono.dix"
    procdir = "lr"
    procflags = ["-z"]
    inputs = itertools.repeat("")
    expectedOutputs = itertools.repeat("")
    expectedRetCodeFail = False
    expectedCompRetCodeFail = False

    def compileTest(self, tmpd):
        retCode = call([os.environ['LTTOOLBOX_PATH']+"/lt-comp",
                        self.procdir,
                        self.procdix,
                        tmpd+"/compiled.bin"],
                       stdout=PIPE)
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
            self.proc = Popen([os.environ['LTTOOLBOX_PATH']+"/lt-proc"] + self.procflags + [tmpd+"/compiled.bin"],
                              stdin=PIPE,
                              stdout=PIPE,
                              stderr=PIPE)

            self.assertEqual(len(self.inputs),
                             len(self.expectedOutputs))
            for inp, exp in zip(self.inputs, self.expectedOutputs):
                self.assertEqual(self.communicateFlush(inp+"[][\n]", self.proc),
                                 exp+"[][\n]")

            self.proc.communicate() # let it terminate
            self.proc.stdin.close()
            self.proc.stdout.close()
            self.proc.stderr.close()
            retCode = self.proc.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)

        finally:
            rmtree(tmpd)
