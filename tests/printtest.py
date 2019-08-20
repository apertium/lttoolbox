# -*- coding: utf-8 -*-

import itertools
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree
from basictest import BasicTest

class PrintTest(BasicTest):
    """See lt_print test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    printdix = "data/minimal-mono.dix"
    printdir = "lr"
    expectedOutput = itertools.repeat("")
    expectedRetCodeFail = False

    def compileTest(self, tmpd):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.printdir,
                                  self.printdix,
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))

    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.compileTest(tmpd)
            self.printresult = Popen(["../lttoolbox/lt-print"] + [tmpd+"/compiled.bin"],
                              stdout=PIPE,
                              stderr=PIPE)

            self.assertEqual(self.communicateFlush(None, self.printresult), self.expectedOutput)

            self.printresult.communicate() # let it terminate
            self.printresult.stdout.close()
            self.printresult.stderr.close()
            retCode = self.printresult.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)

        finally:
            rmtree(tmpd)
