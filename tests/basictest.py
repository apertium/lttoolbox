# -*- coding: utf-8 -*-
import os
from shutil import rmtree
import signal
from subprocess import run, call, PIPE, Popen
from sys import stderr
from tempfile import mkdtemp
from typing import List

class Alarm(Exception):
    pass

class BasicTest:
    def alarmHandler(self, signum, frame):
        raise Alarm

    def withTimeout(self, seconds, cmd, *args, **kwds):
        # Windows doesn't have SIGALRM
        try:
            signal.signal(signal.SIGALRM, self.alarmHandler)
            signal.alarm(seconds)
            ret = cmd(*args, **kwds)
            signal.alarm(0)         # reset the alarm
        except AttributeError:
            ret = cmd(*args, **kwds)
        return ret

    def communicateFlush(self, string, process):
        if string:
            process.stdin.write(string.encode('utf-8'))
            process.stdin.write(b'\0')
            process.stdin.flush()

        output = []
        char = None
        try:
            char = self.withTimeout(10, process.stdout.read, 1)
        except Alarm:
            print("Timeout before reading a single character!", file=stderr)
        while char and char != b'\0':
            output.append(char)
            try:
                char = self.withTimeout(10, process.stdout.read, 1)
            except Alarm:
                print("Timeout before reading %s chars" % len(output),
                      file=stderr)
                break           # send what we got up till now

        return b"".join(output).decode('utf-8').replace('\r\n', '\n')

    def openPipe(self, procName, args):
        return Popen([os.environ['LTTOOLBOX_PATH']+'/'+procName] + args,
                     stdin=PIPE, stdout=PIPE, stderr=PIPE)
    def closePipe(self, proc, expectFail=False):
        proc.communicate() # let it terminate
        proc.stdin.close()
        proc.stdout.close()
        proc.stderr.close()
        retCode = proc.poll()
        if expectFail:
            self.assertNotEqual(retCode, 0)
        else:
            self.assertEqual(retCode, 0)

    def compileDix(self, dir, dix, flags=None, binName='compiled.bin',
                   expectFail=False):
        return self.callProc('lt-comp',
                             [dir, dix, binName],
                             flags,
                             expectFail)

    def callProc(self, name, bins, flags=None, expectFail=False):
        cmd = [os.environ['LTTOOLBOX_PATH']+'/'+name] + (flags or []) + bins
        res = run(cmd, capture_output=True)
        if (res.returncode == 0) == expectFail:
            print("\nFAILED CMD: " + " ".join(cmd))
            print("\nSTDOUT:", res.stdout)
            print("STDERR:", res.stderr)
        if expectFail:
            self.assertNotEqual(res.returncode, 0)
            return False
        else:
            self.assertEqual(res.returncode, 0)
            return True


class TempDir:
    def __enter__(self):
        self.tmpd = mkdtemp()
        return self.tmpd

    def __exit__(self, *args):
        rmtree(self.tmpd)


class PrintTest(BasicTest):
    """See lt_print test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    printdix = "data/minimal-mono.dix"
    printdir = "lr"
    expectedOutput = ""
    expectedRetCodeFail = False
    printflags = []

    def compileTest(self, tmpd):
        return self.compileDix(self.printdir, self.printdix,
                               binName=tmpd+'/compiled.bin')

    def runTest(self):
        with TempDir() as tmpd:
            self.compileTest(tmpd)
            self.printresult = self.openPipe('lt-print',
                                             self.printflags
                                             + [tmpd+'/compiled.bin'])

            self.assertEqual(self.communicateFlush(None, self.printresult),
                             self.expectedOutput)

            self.closePipe(self.printresult, self.expectedRetCodeFail)


class ProcTest(BasicTest):
    """See lt_proc test for how to use this. Override runTest if you don't
    want to use NUL flushing."""

    procdix = "data/minimal-mono.dix"
    procdir = "lr"
    compflags = []              # type: List[str]
    procflags = ["-z"]
    inputs = [""]
    expectedOutputs = [""]
    expectedRetCodeFail = False
    expectedCompRetCodeFail = False
    flushing = True

    def compileTest(self, tmpd):
        return self.compileDix(self.procdir, self.procdix,
                               flags=self.compflags,
                               binName=tmpd+'/compiled.bin',
                               expectFail=self.expectedCompRetCodeFail)

    def runTest(self):
        with TempDir() as tmpd:
            if not self.compileTest(tmpd):
                return
            if self.flushing:
                self.runTestFlush(tmpd)
            else:
                self.runTestNoFlush(tmpd)

    def openProc(self, tmpd):
        return self.openPipe('lt-proc', self.procflags+[tmpd+'/compiled.bin'])

    def runTestFlush(self, tmpd):
        proc = self.openProc(tmpd)
        self.assertEqual(len(self.inputs),
                         len(self.expectedOutputs))
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            self.assertEqual(self.communicateFlush(inp+"[][\n]", proc),
                             exp+"[][\n]")
        self.closePipe(proc, self.expectedRetCodeFail)

    def runTestNoFlush(self, tmpd):
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            proc = self.openProc(tmpd)
            self.assertEqual(proc.communicate(input=inp.encode('utf-8'))[0],
                             exp.encode('utf-8'))
            retCode = proc.poll()
            if self.expectedRetCodeFail:
                self.assertNotEqual(retCode, 0)
            else:
                self.assertEqual(retCode, 0)
