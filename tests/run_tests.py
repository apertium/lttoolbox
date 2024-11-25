#!/usr/bin/env python3

import sys
import os
sys.path.append(os.path.realpath("."))
import unittest

os.environ['LTTOOLBOX_PATH'] = '../lttoolbox'
if len(sys.argv) > 1:
    os.environ['LTTOOLBOX_PATH'] = sys.argv[1]

modules = ['lt_proc', 'lt_trim', 'lt_print', 'lt_comp', 'lt_append',
           'lt_paradigm', 'lt_expand', 'lt_apply_acx', 'lt_compose',
           'lt_tmxproc', 'lt_merge']

# modules = ['lt_merge']


if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    failures = 0
    for module in modules:
        suite = unittest.TestLoader().loadTestsFromName(module)
        res = unittest.TextTestRunner(verbosity = 2).run(suite)
        failures += len(res.failures)
    sys.exit(min(failures, 255))
