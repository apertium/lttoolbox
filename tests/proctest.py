# -*- coding: utf-8 -*-

import itertools
from subprocess import Popen, PIPE, call
from tempfile import mkdtemp
from shutil import rmtree

import signal
class Alarm(Exception):
    pass

class ProcTest():
    """See lt_proc test for how to use this. Override runTest if you don't
    want to use NUL flushing."""
    
    procdix = "data/minimal-mono.dix"
    procdir = "lr"
    procflags = ["-z"]
    inputs = itertools.repeat("")
    expectedOutputs = itertools.repeat("")
    expectedRetCodeFail = False

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
        self.proc.stdin.write(b'\0')
        self.proc.stdin.flush()

        output = []
        char = None
        try:
            char = self.withTimeout(2, self.proc.stdout.read, 1)
        except Alarm:
            pass
        while char and char != b'\0':
            output.append(char)
            try:
                char = self.withTimeout(2, self.proc.stdout.read, 1)
            except Alarm:
                break           # send what we got up till now

        return b"".join(output).decode('utf-8')

    def compileTest(self, tmpd):
        self.assertEqual(0, call(["../lttoolbox/lt-comp",
                                  self.procdir,
                                  self.procdix,
                                  tmpd+"/compiled.bin"],
                                 stdout=PIPE))

    def runTest(self):
        tmpd = mkdtemp()
        try:
            self.compileTest(tmpd)
            self.proc = Popen(["../lttoolbox/lt-proc"] + self.procflags + [tmpd+"/compiled.bin"],
                              stdin=PIPE,
                              stdout=PIPE,
                              stderr=PIPE)

            for inp, exp in zip(self.inputs, self.expectedOutputs):
                self.assertEqual(self.communicateFlush(inp+"[][\n]"),
                                 exp+"[][\n]")

            self.proc.communicate() # let it terminate
            self.proc.stdin.close()
            self.proc.stdout.close()
            self.proc.stderr.close()
            retCode = self.proc.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)

        finally:
            rmtree(tmpd)
