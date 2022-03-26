# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os
from proctest import ProcTest
import unittest

from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree

class AppendProcTest(ProcTest):
    dix1 = "data/append1.dix"
    dix2 = "data/append2.dix"
    dir1 = "lr"
    dir2 = "lr"
    procflags = ["-z"]

    def compileTest(self, tmpd):
        self.assertEqual(0, call([os.environ['LTTOOLBOX_PATH']+"/lt-comp",
                                  self.dir1,
                                  self.dix1,
                                  tmpd+"/dix1.bin"],
                                 stdout=PIPE))
        self.assertEqual(0, call([os.environ['LTTOOLBOX_PATH']+"/lt-comp",
                                  self.dir2,
                                  self.dix2,
                                  tmpd+"/dix2.bin"],
                                 stdout=PIPE))
        self.assertEqual(0, call([os.environ['LTTOOLBOX_PATH']+"/lt-append",
                                  tmpd+"/dix1.bin",
                                  tmpd+"/dix2.bin",
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))
        return True

class SimpleAppend(AppendProcTest):
    inputs = ["a", "b"]
    expectedOutputs = ["^a/a<n>$",
					   "^b/b<v>$"]
