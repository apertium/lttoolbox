from proctest import ProcTest

class ParadigmTest(ProcTest):
    inputs = ['ab<n><*>', 'y<*>', '*<n><def>']
    expectedOutputs = ['ab<n><def>:abc\nab<n><ind>:ab',
                       'y<n><ind>:y',
                       'ab<n><def>:abc']
    procdix = 'data/minimal-mono.dix'
    procdir = 'rl'

    def runTestFlush(self, tmpd):
        proc = self.openPipe('lt-paradigm',
                             self.procflags+[tmpd+'/compiled.bin'])
        self.assertEqual(len(self.inputs), len(self.expectedOutputs))
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            out = self.communicateFlush(inp + '\n', proc)
            srt = '\n'.join(sorted(out.strip().splitlines()))
            self.assertEqual(srt, exp)
        self.closePipe(proc, expectFail=self.expectedRetCodeFail)

class ParadigmAnalyzerTest(ParadigmTest):
    procdir = 'lr'
    procflags = ['-a']
