# -*- coding: utf-8 -*-

import sys
import itertools
import unittest

from subprocess import Popen, PIPE

import signal
class Alarm(Exception):
    pass

class NullFlushTest():
    cmdLine = ["../lttoolbox/.libs/lt-proc", "-z", "data/en-af.automorf.bin"]
    inputs = itertools.repeat("")
    expectedOutputs = itertools.repeat("")
    expectedRetCode = 0

    def alarmHandler(self, signum, frame):
        raise Alarm

    def withTimeout(self, seconds, cmd, *args, **kwds):
        signal.signal(signal.SIGALRM, self.alarmHandler)
        signal.alarm(seconds)
        ret = cmd(*args, **kwds)
        signal.alarm(0)         # reset the alarm
        return ret

    def communicateFlush(self, string):
        self.proc.stdin.write(string.encode('utf-8'))
        self.proc.stdin.write('\0')
        self.proc.stdin.flush()

        output = []
        char = None
        try:
            char = self.withTimeout(2, self.proc.stdout.read, 1)
        except Alarm:
            pass
        while char and char != '\0':
            output.append(char)
            try:
                char = self.withTimeout(2, self.proc.stdout.read, 1)
            except Alarm:
                break           # send what we got up till now

        return "".join(output).decode('utf-8')

    def runTest(self):
        self.proc = Popen(self.cmdLine, stdin=PIPE, stdout=PIPE, stderr=PIPE)

        for inp,exp in zip(self.inputs, self.expectedOutputs):
            self.assertEqual( self.communicateFlush(inp),
                              exp )

        self.proc.communicate() # let it terminate
        self.proc.stdin.close()
        self.proc.stdout.close()
        self.proc.stderr.close()
        self.assertEqual( self.proc.poll(),
                          self.expectedRetCode )

class ValidInput(unittest.TestCase, NullFlushTest):
    inputs = [s + ".[][\n]" for s in
              ["I",
               "like apples",
               "very much"]]

    expectedOutputs = [s + "^./.<sent>$[][\n]" for s in
                       ["^I/prpers<prn><subj><p1><mf><sg>/PRPERS<prn><subj><p1><mf><sg>$",
                        "^like/like<pr>/like<vblex><inf>/like<vblex><pres>$ ^apples/apple<n><pl>$",
                        "^very much/very much<adv>$"]]

class NoSuperblankBeforeNUL(unittest.TestCase, NullFlushTest):
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

class WronglyEscapedLetter(unittest.TestCase, NullFlushTest):
    inputs = ["before you g\\o to bed.[][\n]"]
    expectedOutputs = ["^before/before<adv>/before<cnjadv>/before<pr>$ ^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ "]
    expectedRetCode = 1


class UnescapedAngleBracket(unittest.TestCase, NullFlushTest):
    inputs = ["Simon prefers dark chocolate>.[][\n]"]
    expectedOutputs = ["^Simon/Simon<np><ant><m><sg>$ ^prefers/prefer<vblex><pri><p3><sg>$ ^dark/dark<adj><sint>/dark<n><sg>$ "]
    expectedRetCode = 1

class UnclosedSuperblank(unittest.TestCase, NullFlushTest):
    inputs = ["you should always[ eat"]
    #expectedOutputs = ["^you/prpers<prn><subj><p2><mf><sp>/prpers<prn><obj><p2><mf><sp>$ ^should/should<vaux><inf>$ "]
    expectedOutputs = [""]
    expectedRetCode = 1
