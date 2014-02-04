# -*- coding: utf-8 -*-

import sys
import itertools
import unittest

from apertium_process import ApertiumProcess

############################################################################################################
# Tests which only test null flushing and which only close stdin after all tests have been executed.

class NullFlushTestKeepOpen(object):
    def assertEqualPipeOutput(self, proc, inputs, expectedOutputs, expectedErrors = itertools.repeat("")):
        for inString, expectedOutString, expectedErrorString in zip(inputs, expectedOutputs, expectedErrors):
            proc.send(inString.encode('utf-8'))

            outString = proc.readUntilNull(proc.recv).decode('utf-8')
            self.assertEqual(expectedOutString, outString)

            errorString = proc.readUntilNull(proc.recv_err).decode('utf-8')
            self.assertEqual(expectedErrorString, errorString)


    def ensureOutput(self, proc, expectedOutput, expectedError):
            outString = proc.readUntilNull(proc.recv)
            self.assertEqual(expectedOutput, outString)

            errorString = proc.readUntilNull(proc.recv_err)
            self.assertEqual(expectedError, errorString)


    def runTest(self):
        self.runNullTest()

        self.ensureOutput(self.proc, "", "")
        self.proc.stdin.close()
        self.ensureOutput(self.proc, '\x00', "") # TODO: _should_ this be null here??
        self.assertEqual(self.proc.wait(), 0)


class OnlyValidInput(unittest.TestCase, NullFlushTestKeepOpen):
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


############################################################################################################
# Tests which only test null flushing and which restart proc for each test

class NullFlushTestRestartProc(object):
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


class OnlyValidInputWithStdinClose(unittest.TestCase, NullFlushTestRestartProc):
    def runNullTest(self):
        inputs = [u"The dog gladly eats homework.",
                  u"If wé swim fast enough,\x00we should reach shallow waters.",
                  u"before;\x00the sharks;\x00come."]

        outputs = [u"^The/The<det><def><sp>$ ^dog/dog<n><sg>$ ^gladly/gladly<adv>$ ^eats/eat<vblex><pri><p3><sg>$ ^homework/homework<n><unc><sg>$\x00",
                   u"^If/If<cnjadv>$ ^wé/*wé$ ^swim/swim<vblex><inf>/swim<vblex><pres>$ ^fast/fast<adj><sint>/fast<n><sg>$ ^enough/enough<adv>/enough<det><qnt><sp>$\x00^we/prpers<prn><subj><p1><mf><pl>$ ^should/should<vaux><inf>$ ^reach/reach<vblex><inf>/reach<vblex><pres>$ ^shallow/shallow<adj><sint>$ ^waters/water<n><pl>$\x00",
                   u"^before/before<adv>/before<cnjadv>/before<pr>$\x00^the/the<det><def><sp>$ ^sharks/shark<n><pl>$\x00^come/come<vblex><inf>/come<vblex><pres>/come<vblex><pp>$\x00"]

        errorOutputs = [u"",
                        u"",
                        u"",]

        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"], inputs, outputs, errorOutputs)

class WronglyEscapedLetter(unittest.TestCase, NullFlushTestRestartProc):
    def runNullTest(self):
        inputs = ["before you g\\o to bed.[][\n]\x00"]

        outputs = ["^before/before<adv>/before<cnjadv>/before<pr>$ ^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ "]

        errorOutputs = ["std::exception"]

        errorCodes = [1]

        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"],
                                   inputs, outputs, errorOutputs, errorCodes)


class UnescapedAngleBracket(unittest.TestCase, NullFlushTestRestartProc):
    def runNullTest(self):
        inputs = ["Simon prefers dark chocolate>.[][\n]\x00"]

        outputs = ["^Simon/Simon<np><ant><m><sg>$ ^prefers/prefer<vblex><pri><p3><sg>$ ^dark/dark<adj><sint>/dark<n><sg>$ "]

        errorOutputs = ["std::exception"]

        errorCodes = [1]
        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"],
                                   inputs, outputs, errorOutputs, errorCodes)


class UnclosedSuperblank(unittest.TestCase, NullFlushTestRestartProc):
    def runNullTest(self):
        inputs = ["you should always[ eat\x00"]

        outputs = ["^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ ^should/should<vaux><inf>$ "]

        errorOutputs = ["std::exception"]

        errorCodes = [1]
        self.assertEqualPipeOutput(["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"],
                                   inputs, outputs, errorOutputs, errorCodes)
