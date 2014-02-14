# -*- coding: utf-8 -*-

import unittest

from subprocess import call
from tempfile import mkdtemp
from shutil import rmtree

from proctest import ProcTest
from subprocess import Popen, PIPE

class Trim(unittest.TestCase, ProcTest):
    inputs = ["abc", "ab", "y", "n",
              "jg", "jh", "kg"
    ]
    expectedOutputs = ["^abc/ab<n><def>$", "^ab/ab<n><ind>$", "^y/y<n><ind>$", "^n/*n$",
                       "^jg/j<pr>+g<n>$", "^jh/*jh$", "^kg/*kg$"
    ]
    expectedRetCode = 0

    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                      "lr",
                                      "data/minimal-mono.dix",
                                      tmpd+"/minimal-mono.bin"],
                                     stdout=PIPE))
            self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                      "lr",
                                      "data/minimal-bi.dix",
                                      tmpd+"/minimal-bi.bin"],
                                     stdout=PIPE))
            self.assertEqual(0, call(["../lttoolbox/lt-trim",
                                      tmpd+"/minimal-mono.bin",
                                      tmpd+"/minimal-bi.bin",
                                      tmpd+"/minimal-trimmed.bin"],
                                     stdout=PIPE))

            self.cmdLine = ["../lttoolbox/.libs/lt-proc", "-z", tmpd+"/minimal-trimmed.bin"]
            self.proc = Popen(self.cmdLine, stdin=PIPE, stdout=PIPE, stderr=PIPE)

            for inp,exp in zip(self.inputs, self.expectedOutputs):
                self.assertEqual( self.communicateFlush(inp+"[][\n]"),
                                  exp+"[][\n]" )

            self.proc.communicate() # let it terminate
            self.proc.stdin.close()
            self.proc.stdout.close()
            self.proc.stderr.close()
            self.assertEqual( self.proc.poll(),
                              self.expectedRetCode )


        finally:
            rmtree(tmpd)
