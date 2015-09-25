# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from proctest import ProcTest

class ValidInput(unittest.TestCase, ProcTest):
    inputs = [s + ".[][\n]" for s in
              ["I",
               "like apples",
               "very much"]]

    expectedOutputs = [s + "^./.<sent>$[][\n]" for s in
                       ["^I/prpers<prn><subj><p1><mf><sg>/PRPERS<prn><subj><p1><mf><sg>$",
                        "^like/like<pr>/like<vblex><inf>/like<vblex><pres>$ ^apples/apple<n><pl>$",
                        "^very much/very much<adv>$"]]
