# -*- coding: utf-8 -*-

import itertools
from subprocess import Popen, PIPE

import signal
class Alarm(Exception):
    pass

class ProcTest():
    """See lt_proc test for how to use this. Override runTest if you don't
    want to use NUL flushing."""
    
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
