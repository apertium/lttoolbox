# -*- coding: utf-8 -*-
import os
from shutil import rmtree
import signal
from subprocess import call, PIPE, Popen
from sys import stderr
from tempfile import mkdtemp

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
            char = self.withTimeout(2, process.stdout.read, 1)
        except Alarm:
            print("Timeout before reading a single character!", file=stderr)
        while char and char != b'\0':
            output.append(char)
            try:
                char = self.withTimeout(2, process.stdout.read, 1)
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
        code = call([os.environ['LTTOOLBOX_PATH']+'/lt-comp']
                    + (flags or []) + [dir, dix, binName],
                    stdout=PIPE, stderr=PIPE)
        if expectFail:
            self.assertNotEqual(0, code)
        else:
            self.assertEqual(0, code)
        return code == 0

    def callProc(self, name, bins, flags=None, retCode=0):
        self.assertEqual(retCode,
                         call([os.environ['LTTOOLBOX_PATH']+'/'+name]
                              + (flags or []) + bins,
                              stdout=PIPE, stderr=PIPE))

class TempDir:
    def __enter__(self):
        self.tmpd = mkdtemp()
        return self.tmpd
    def __exit__(self, *args):
        rmtree(self.tmpd)
