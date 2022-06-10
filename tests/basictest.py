# -*- coding: utf-8 -*-
import signal
from sys import stderr

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
