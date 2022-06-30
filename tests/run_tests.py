#!/usr/bin/env python3

import sys
import os
sys.path.append(os.path.realpath("."))

import unittest
import lt_proc
import lt_trim
import lt_print
import lt_comp
import lt_append
import lt_paradigm

os.environ['LTTOOLBOX_PATH'] = '../lttoolbox'
if len(sys.argv) > 1:
	os.environ['LTTOOLBOX_PATH'] = sys.argv[1]

if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    failures = 0
    for module in [lt_trim, lt_proc, lt_print, lt_comp, lt_append, lt_paradigm]:
        suite = unittest.TestLoader().loadTestsFromModule(module)
        res = unittest.TextTestRunner(verbosity = 2).run(suite)
        failures += len(res.failures)
    sys.exit(min(failures, 255))
