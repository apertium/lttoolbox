# -*- coding: utf-8 -*-

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

    def compileTest(self, tmpd):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.procdir,
                                  self.procdix,
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))

    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.compileTest(tmpd)
            self.proc = Popen(["../lttoolbox/lt-proc"] + self.procflags + [tmpd+"/compiled.bin"],
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
