# -*- coding: utf-8 -*-

from basictest import BasicTest, TempDir

class PrintTest(BasicTest):
    """See lt_print test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    printdix = "data/minimal-mono.dix"
    printdir = "lr"
    expectedOutput = ""
    expectedRetCodeFail = False
    printflags = []

    def compileTest(self, tmpd):
        self.compileDix(self.printdir, self.printdix,
                        binName=tmpd+'/compiled.bin')

    def runTest(self):
        with TempDir() as tmpd:
            self.compileTest(tmpd)
            self.printresult = self.openPipe('lt-print',
                                             self.printflags
                                             + [tmpd+'/compiled.bin'])

            self.assertEqual(self.communicateFlush(None, self.printresult), self.expectedOutput)

            self.closePipe(self.printresult, self.expectedRetCodeFail)
