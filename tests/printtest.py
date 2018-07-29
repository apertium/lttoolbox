# -*- coding: utf-8 -*-

import itertools
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree

import signal
class Alarm(Exception):
    pass

class PrintTest():
    """See lt_print test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    printdix = "data/minimal-mono.dix"
    printdir = "lr"
    expectedOutput = itertools.repeat("")
    expectedRetCodeFail = False

    def alarmHandler(self, signum, frame):
        raise Alarm

    def withTimeout(self, seconds, cmd, *args, **kwds):
        signal.signal(signal.SIGALRM, self.alarmHandler)
        signal.alarm(seconds)
        ret = cmd(*args, **kwds)
        signal.alarm(0)         # reset the alarm
        return ret

    def communicateFlush(self):
        output = []
        char = None
        try:
            char = self.withTimeout(2, self.printresult.stdout.read, 1)
        except Alarm:
            pass
        while char and char != b'\0':
            output.append(char)
            try:
                char = self.withTimeout(2, self.printresult.stdout.read, 1)
            except Alarm:
                break           # send what we got up till now

        return b"".join(output).decode('utf-8')

    def compileTest(self, tmpd):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.printdir,
                                  self.printdix,
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))

    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.compileTest(tmpd)
            self.printresult = Popen(["../lttoolbox/lt-print"] + [tmpd+"/compiled.bin"],
                              stdout=PIPE,
                              stderr=PIPE)

            self.assertEqual(self.communicateFlush(), self.expectedOutput)

            self.printresult.communicate() # let it terminate
            self.printresult.stdout.close()
            self.printresult.stderr.close()
            retCode = self.printresult.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)

        finally:
            rmtree(tmpd)
