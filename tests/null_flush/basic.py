import unittest
import sys

from async.subproc import Popen, PIPE
import config

def readUntilNull(proc):
    ret = []
    last_read = ''
    while last_read != '\x00':
        last_read = proc.recv(1)
        ret.append(last_read)

    return ''.join(ret)

class TestBasic(unittest.TestCase):
    def runTest(self):
        inputs = ["I",
                  "like apples",
                  "very much"]
        inputs = [s + ".[][\n]\x00" for s in inputs]

        outputs = ["^I/prpers<prn><subj><p1><mf><sg>/PRPERS<prn><subj><p1><mf><sg>$",
                   "^like/like<pr>/like<vblex><inf>/like<vblex><pres>$ ^apples/apple<n><pl>$",
                   "^very much/very much<adv>$"]
        outputs = [s + "^./.<sent>$[][\n]\x00" for s in outputs]
        
        proc = Popen(["../lttoolbox/lt-proc", "-z", config.automorf_bin], stdin=PIPE, stdout=PIPE, stderr=PIPE)

        for i, o in zip(inputs, outputs):
            proc.send(i)
            test_o = readUntilNull(proc)
            self.failUnless(test_o == o)
        
