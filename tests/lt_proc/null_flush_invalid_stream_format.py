# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import sys
import unittest
from proctest import ProcTest

# These tests are for invalid Apertium Stream format; lt-proc's output
# for these seems system-dependent, so we can't use them as regression
# tests (until that's fixed, if it's worth fixing).

class NoSuperblankBeforeNUL(unittest.TestCase, ProcTest):
    inputs = ["The dog gladly eats homework.",
              "If wé swim fast enough,",
              "we should reach shallow waters.",
              "before;",
              "the sharks;",
              "come."]

    expectedOutputs = ["^The/The<det><def><sp>$ ^dog/dog<n><sg>$ ^gladly/gladly<adv>$ ^eats/eat<vblex><pri><p3><sg>$ ^homework/homework<n><unc><sg>$",
                       "^If/If<cnjadv>$ ^wé/*wé$ ^swim/swim<vblex><inf>/swim<vblex><pres>$ ^fast/fast<adj><sint>/fast<n><sg>$ ^enough/enough<adv>/enough<det><qnt><sp>$",
                       "^we/prpers<prn><subj><p1><mf><pl>$ ^should/should<vaux><inf>$ ^reach/reach<vblex><inf>/reach<vblex><pres>$ ^shallow/shallow<adj><sint>$ ^waters/water<n><pl>$",
                       "^before/before<adv>/before<cnjadv>/before<pr>$",
                       "^the/the<det><def><sp>$ ^sharks/shark<n><pl>$",
                       "^come/come<vblex><inf>/come<vblex><pres>/come<vblex><pp>$"]

class WronglyEscapedLetter(unittest.TestCase, ProcTest):
    inputs = ["before you g\\o to bed.[][\n]"]
    expectedOutputs = ["^before/before<adv>/before<cnjadv>/before<pr>$ ^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ "]
    expectedRetCodeFail = True


class UnescapedAngleBracket(unittest.TestCase, ProcTest):
    inputs = ["Simon prefers dark chocolate>.[][\n]"]
    expectedOutputs = ["^Simon/Simon<np><ant><m><sg>$ ^prefers/prefer<vblex><pri><p3><sg>$ ^dark/dark<adj><sint>/dark<n><sg>$ "]
    expectedRetCodeFail = True

class UnclosedSuperblank(unittest.TestCase, ProcTest):
    inputs = ["you should always[ eat"]
    #expectedOutputs = ["^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ ^should/should<vaux><inf>$ "]
    expectedOutputs = [""]
    expectedRetCodeFail = True
