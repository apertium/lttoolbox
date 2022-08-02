import unittest
from basictest import BasicTest

class ExpandTest(unittest.TestCase, BasicTest):
    expanddix = 'data/minimal-mono.dix'
    expanddir = 'lr'
    expectedOutput = '''abc:ab<n><def>
ab:ab<n><ind>
y:y<n><ind>
n:n<n><ind>
jg:j<pr>+g<n>
jh:j<pr>+h<n>
kg:k<pr>+g<n>
'''
    expandflags = []

    def runTest(self):
        pp = self.openPipe('lt-expand', self.expandflags + [self.expanddix])
        self.assertEqual(self.communicateFlush(None, pp),
                         self.expectedOutput)
        self.closePipe(pp, False)

class ExpandRegex(ExpandTest):
    expanddix = 'data/expand-re.dix'
    expectedOutput = '''abc:ab<n><def>
ab:ab<n><ind>
y:y<n><ind>
n:n<n><ind>
__REGEXP__xyz\\:abc[qxj]\\+:__REGEXP__xyz\\:abc[qxj]\\+<vblex>
'''
