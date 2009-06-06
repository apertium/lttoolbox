import time, cmd;
from subproc import Popen, PIPE

class ApertiumProcess(Popen):
    def __init__(self, arguments, stdin=PIPE, stdout=PIPE, stderr=PIPE):
        super(ApertiumProcess, self).__init__(arguments, stdin=stdin, stdout=stdout, stderr=stderr)


    def readChar(self, reader):
        last_read = self.recv(1)
        sleeps = 0

        while last_read in (None, '') and sleeps < 3:
            time.sleep(0.1)
            last_read = reader(1)
            sleeps += 1

        if last_read != None:
            return last_read
        else:
            return ''


    def readUntilNull(self, reader):
        ret = []
        last_read = '?'

        while last_read not in ('\x00', ''):
            last_read = self.readChar(reader)
            ret.append(last_read)

        return ''.join(ret)


    def kill(self):
        import os
        try:
	    print self.pid;
	    os.system('kill -9 ' + self.pid);
            os.kill(self.pid, 9)
        except OSError:
            pass # if we get here, the process either never started, or has already died


    def __del__(self):
        self.kill()

