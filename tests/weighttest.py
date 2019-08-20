# -*- coding: utf-8 -*-
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree
from pathlib import Path
import itertools
from basictest import BasicTest

class WeightTest(BasicTest):
    compdix = "data/minimal-mono.dix"
    compdir = "lr"
    weightlists = []
    procflags = ["-z"]
    inputs = itertools.repeat("")
    expectedOutputs = itertools.repeat("")

    def compileTest(self, compiled_dix):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.compdir,
                                  self.compdix,
                                  compiled_dix],
                                 stdout=PIPE))
    def runTest(self):
        tmpd = mkdtemp()
        compiled_dix = Path(tmpd, "compiled.bin")
        weighted_dix = Path(tmpd, "weighted.bin")

        try:
            self.compileTest(compiled_dix)
            self.assertEqual(0, call(["../scripts/lt-weight"] +
                              [compiled_dix, weighted_dix] +
                              self.weightlists,
                              stdout=PIPE))
            self.proc = Popen(["../lttoolbox/lt-proc"] +
                               self.procflags +
                               [weighted_dix],
                              stdin=PIPE,
                              stdout=PIPE,
                              stderr=PIPE)

            self.assertEqual(len(self.inputs),
                             len(self.expectedOutputs))

            for inp, exp in zip(self.inputs, self.expectedOutputs):
                output = self.communicateFlush(inp+"[][\n]", self.proc)
                self.assertEqual(output,
                                 exp+"[][\n]")

            self.proc.communicate() # let it terminate
            self.proc.stdin.close()
            self.proc.stdout.close()
            self.proc.stderr.close()
            retCode = self.proc.poll()

        finally:
            rmtree(tmpd)
            pass
