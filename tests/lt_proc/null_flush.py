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

class NoSuperblankBeforeNUL(unittest.TestCase, ProcTest):
    inputs = [u"The dog gladly eats homework.",
              u"If wé swim fast enough,",
              u"we should reach shallow waters.",
              u"before;",
              u"the sharks;",
              u"come."]

    expectedOutputs = [u"^The/The<det><def><sp>$ ^dog/dog<n><sg>$ ^gladly/gladly<adv>$ ^eats/eat<vblex><pri><p3><sg>$ ^homework/homework<n><unc><sg>$",
                       u"^If/If<cnjadv>$ ^wé/*wé$ ^swim/swim<vblex><inf>/swim<vblex><pres>$ ^fast/fast<adj><sint>/fast<n><sg>$ ^enough/enough<adv>/enough<det><qnt><sp>$",
                       u"^we/prpers<prn><subj><p1><mf><pl>$ ^should/should<vaux><inf>$ ^reach/reach<vblex><inf>/reach<vblex><pres>$ ^shallow/shallow<adj><sint>$ ^waters/water<n><pl>$",
                       u"^before/before<adv>/before<cnjadv>/before<pr>$",
                       u"^the/the<det><def><sp>$ ^sharks/shark<n><pl>$",
                       u"^come/come<vblex><inf>/come<vblex><pres>/come<vblex><pp>$"]

class WronglyEscapedLetter(unittest.TestCase, ProcTest):
    inputs = ["before you g\\o to bed.[][\n]"]
    expectedOutputs = ["^before/before<adv>/before<cnjadv>/before<pr>$ ^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ "]
    expectedRetCode = 1


class UnescapedAngleBracket(unittest.TestCase, ProcTest):
    inputs = ["Simon prefers dark chocolate>.[][\n]"]
    expectedOutputs = ["^Simon/Simon<np><ant><m><sg>$ ^prefers/prefer<vblex><pri><p3><sg>$ ^dark/dark<adj><sint>/dark<n><sg>$ "]
    expectedRetCode = 1

class UnclosedSuperblank(unittest.TestCase, ProcTest):
    inputs = ["you should always[ eat"]
    #expectedOutputs = ["^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ ^should/should<vaux><inf>$ "]
    expectedOutputs = [""]
    expectedRetCode = 1
