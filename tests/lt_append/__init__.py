# -*- coding: utf-8 -*-
from basictest import ProcTest
import unittest

class AppendProcTest(unittest.TestCase, ProcTest):
    dix1 = "data/append1.dix"
    dix2 = "data/append2.dix"
    dir1 = "lr"
    dir2 = "lr"
    procflags = ["-z"]

    def compileTest(self, tmpd):
        self.compileDix(self.dir1, self.dix1, binName=tmpd+'/dix1.bin')
        self.compileDix(self.dir2, self.dix2, binName=tmpd+'/dix2.bin')
        self.callProc('lt-append', [tmpd+"/dix1.bin",
                                    tmpd+"/dix2.bin",
                                    tmpd+"/compiled.bin"])
        return True

class SimpleAppend(AppendProcTest):
    inputs = ["a", "b"]
    expectedOutputs = ["^a/a<n>$",
					   "^b/b<v>$"]
