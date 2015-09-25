# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from proctest import ProcTest

class ValidInput(unittest.TestCase, ProcTest):
    inputs = ["ab",
              "ABC jg",
              "y n"]
    expectedOutputs = ["^ab/ab<n><ind>$",
                       "^ABC/AB<n><def>$ ^jg/j<pr>+g<n>$",
                       "^y/y<n><ind>$ ^n/n<n><ind>$"]
