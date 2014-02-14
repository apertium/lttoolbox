#!/usr/bin/env python2

import sys
import os
sys.path.append(os.path.realpath("."))

import unittest
import lt_proc, lt_trim

if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    for module in [lt_trim, lt_proc]:
        suite = unittest.TestLoader().loadTestsFromModule(module)
        unittest.TextTestRunner(verbosity = 2).run(suite)
