#!/usr/bin/env python2

import sys
import os
sys.path.append(os.path.realpath("."))

import unittest
import lt_proc

if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    suite = unittest.TestLoader().loadTestsFromModule(lt_proc)
    unittest.TextTestRunner(verbosity = 2).run(suite)
