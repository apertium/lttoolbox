# -*- coding: utf-8 -*-
import signal
class Alarm(Exception):
    pass

class BasicTest:
    def alarmHandler(self, signum, frame):
        raise Alarm

    def withTimeout(self, seconds, cmd, *args, **kwds):
        signal.signal(signal.SIGALRM, self.alarmHandler)
        signal.alarm(seconds)
        ret = cmd(*args, **kwds)
        signal.alarm(0)         # reset the alarm
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
            pass
        while char and char != b'\0':
            output.append(char)
            try:
                char = self.withTimeout(2, process.stdout.read, 1)
            except Alarm:
                break           # send what we got up till now

        return b"".join(output).decode('utf-8')
