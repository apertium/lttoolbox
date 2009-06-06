# -*- coding: utf-8 -*-

import sys
import itertools
import unittest

from apertium_process import ApertiumProcess
#from null_flush_test import NullFlushTest

############################################################################################################
# Tests which only test null flushing and which only close stdin after all tests have been executed.

class NullFlushTest(object):
    def assertEqualPipeOutput(self, proc, inputs, expectedOutputs, expectedErrors = itertools.repeat("\x00")):
        for inString, expectedOutString, expectedErrorString in zip(inputs, expectedOutputs, expectedErrors):
            proc.send(inString.encode('utf-8'))

            outString = proc.readUntilNull(proc.recv).decode('utf-8')
            self.assertEqual(expectedOutString, outString)

            errorString = proc.readUntilNull(proc.recv_err).decode('utf-8')
            self.assertEqual(expectedErrorString, errorString)


    def ensureOutput(self, proc, output):
            outString = proc.readUntilNull(proc.recv)
            self.assertEqual(output, outString)

            errorString = proc.readUntilNull(proc.recv_err)
            self.assertEqual(output, errorString)


    def runTest(self):
        self.runNullTest()

        self.ensureOutput(self.proc, "")
        self.proc.stdin.close()
        self.ensureOutput(self.proc, "")
        self.assertEqual(self.proc.wait(), 0)


class OnlyValidInput(unittest.TestCase, NullFlushTest):
    def runNullTest(self):
        inputs = ["I",
                  "like apples",
                  "very much"]
        inputs = [s + ".[][\n]\x00" for s in inputs]

        outputs = ["^I/prpers<prn><subj><p1><mf><sg>/PRPERS<prn><subj><p1><mf><sg>$",
                   "^like/like<pr>/like<vblex><inf>/like<vblex><pres>$ ^apples/apple<n><pl>$",
                   "^very much/very much<adv>$"]
        outputs = [s + "^./.<sent>$[][\n]\x00" for s in outputs]

        self.proc = ApertiumProcess(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"])
        self.assertEqualPipeOutput(self.proc, inputs, outputs)


class OnlyInvalidInput(unittest.TestCase, NullFlushTest):
    def runNullTest(self):
        inputs = ["I.[\x00",
                  "like \\apples.[][\n]\x00",
                  "very <much.[][\n]\x00"]

        outputs = ["^I/prpers<prn><subj><p1><mf><sg>/PRPERS<prn><subj><p1><mf><sg>$\x00",
                   "\x00",
                   "\x00"]

        errorOutputs = itertools.repeat("Error: Malformed input stream.\x00")

        self.proc = ApertiumProcess(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"])
        self.assertEqualPipeOutput(self.proc, inputs, outputs, errorOutputs)


class ValidAndInvalidInputs(unittest.TestCase, NullFlushTest):
    def runNullTest(self):
        inputs = ["Simple Simon simon says that.[][\n]\x00",
                  "you should always[ eat\x00",
                  "a lot of chocolate before.[][\n]\x00",
                  "before you g\\o to bed.[][\n]\x00",
                  "Simon prefers dark chocolate>.[][\n]\x00",
                  "but sometimes he east milk chocolate.[][\n]\x00"]

        outputs = ["^Simple/Simple<adj><sint>$ ^Simon/Simon<np><ant><m><sg>$ ^simon/*simon$ ^says/say<vblex><pri><p3><sg>$ ^that/that<cnjsub>/that<det><dem><sg>/that<prn><tn><mf><sg>/that<rel><an><mf><sp>$^./.<sent>$[][\n]\x00",
                   "^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ ^should/should<vaux><inf>$ \x00",
                   "^a lot of/a lot of<det><qnt><sp>$ ^chocolate/chocolate<n><sg>$ ^before/before<adv>/before<cnjadv>/before<pr>$^./.<sent>$[][\n]\x00",
                   "^before/before<adv>/before<cnjadv>/before<pr>$ ^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ \x00",
                   "^Simon/Simon<np><ant><m><sg>$ ^prefers/prefer<vblex><pri><p3><sg>$ ^dark/dark<adj><sint>/dark<n><sg>$ \x00",
                   "^but/but<cnjcoo>/but<pr>$ ^sometimes/sometimes<adv>$ ^he/prpers<prn><subj><p3><m><sg>$ ^east/east<adj>/east<n><unc><sg>$ ^milk/milk<n><sg>/milk<vblex><inf>/milk<vblex><pres>$ ^chocolate/chocolate<n><sg>$^./.<sent>$[][\n]\x00"]

        errorOutputs = ["\x00",
                        "Error: Malformed input stream.\x00",
                        "\x00",
                        "Error: Malformed input stream.\x00",
                        "Error: Malformed input stream.\x00",
                        "\x00"]

        self.proc = ApertiumProcess(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"])
        self.assertEqualPipeOutput(self.proc, inputs, outputs, errorOutputs)

############################################################################################################
# Tests which only test null flushing and which only close stdin after all tests have been executed.

class NullFlushTest(object):
    def assertEqualPipeOutput(self, cmdLine, inputs, expectedOutputs,
                              expectedErrors = itertools.repeat(""),
                              expectedErrorCodes = itertools.repeat(0)):

        self.proc = ApertiumProcess(cmdLine)

        for inString, expectedOutString, expectedErrorString, expectedErrorCode\
        in zip(inputs, expectedOutputs, expectedErrors, expectedErrorCodes):
            proc = ApertiumProcess(cmdLine)

            outString, outError = proc.communicate(inString.encode('utf-8'))

            self.assertEqual(expectedOutString, outString.decode('utf-8'))
            self.assertEqual(expectedErrorString, outError.decode('utf-8'))
            self.assertEqual(expectedErrorCode, proc.returncode)


    def runTest(self):
        self.runNullTest()


class OnlyValidInputWithStdinClose(unittest.TestCase, NullFlushTest):
    def runNullTest(self):
        inputs = [u"The dog gladly eats homework.",
                  u"If wé swim fast enough,\x00we should reach shallow waters.",
                  u"before;\x00the sharks;\x00come."]

        outputs = [u"^The/The<det><def><sp>$ ^dog/dog<n><sg>$ ^gladly/gladly<adv>$ ^eats/eat<vblex><pri><p3><sg>$ ^homework/homework<n><unc><sg>$",
                   u"^If/If<cnjadv>$ ^wé/*wé$ ^swim/swim<vblex><inf>/swim<vblex><pres>$ ^fast/fast<adj><sint>/fast<n><sg>$ ^enough/enough<adv>/enough<det><qnt><sp>$\x00^we/prpers<prn><subj><p1><mf><pl>$ ^should/should<vaux><inf>$ ^reach/reach<vblex><inf>/reach<vblex><pres>$ ^shallow/shallow<adj><sint>$ ^waters/water<n><pl>$",
                   u"^before/before<adv>/before<cnjadv>/before<pr>$\x00^the/the<det><def><sp>$ ^sharks/shark<n><pl>$\x00^come/come<vblex><inf>/come<vblex><pres>/come<vblex><pp>$"]

        errorOutputs = [u"",
                        u"\x00",
                        u"\x00\x00"]

        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"], inputs, outputs, errorOutputs)


class OnlyInvalidInputWithStdinClose(unittest.TestCase, NullFlushTest):
    def runNullTest(self):
        inputs = [u"The dog gladly eats[ homework.",
                  u"If wé swim ^ fast enough,\x00we should >reach shallow waters.",
                  u"before];\x00the <sharks;\x00come."]

        outputs = [u"^The/The<det><def><sp>$ ^dog/dog<n><sg>$ ^gladly/gladly<adv>$ ",
                   u"^If/If<cnjadv>$ ^wé/*wé$ ^swim/swim<vblex><inf>/swim<vblex><pres>$ \x00^we/prpers<prn><subj><p1><mf><pl>$ ^should/should<vaux><inf>$ ",
                   u"\x00\x00^come/come<vblex><inf>/come<vblex><pres>/come<vblex><pp>$"]

        errorOutputs = [u"Error: Malformed input stream.",
                        u"Error: Malformed input stream.\x00Error: Malformed input stream.",
                        u"Error: Malformed input stream.\x00Error: Malformed input stream.\x00"]

        errorCodes = [1,
                      1,
                      0]

        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"], 
                                   inputs, outputs, errorOutputs, errorCodes)
