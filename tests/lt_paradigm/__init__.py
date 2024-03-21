from basictest import ProcTest
import unittest

class ParadigmTest(unittest.TestCase, ProcTest):
    inputs = ['ab<n><*>', 'y<*>', '*<n><def>']
    expectedOutputs = ['ab<n><def>:abc\nab<n><ind>:ab',
                       'y<n><ind>:y',
                       'ab<n><def>:abc']
    procdix = 'data/minimal-mono.dix'
    procdir = 'rl'
    sortoutput = True

    def runTestFlush(self, tmpd):
        proc = self.openPipe('lt-paradigm',
                             self.procflags+[tmpd+'/compiled.bin'])
        self.assertEqual(len(self.inputs), len(self.expectedOutputs))
        for inp, exp in zip(self.inputs, self.expectedOutputs):
            out = self.communicateFlush(inp + '\n', proc).strip()
            if self.sortoutput:
                srt = '\n'.join(sorted(out.splitlines()))
                self.assertEqual(exp, srt)
            else:
                self.assertEqual(exp, out)
        self.closePipe(proc, expectFail=self.expectedRetCodeFail)

class ParadigmAnalyzerTest(ParadigmTest):
    procdir = 'lr'
    procflags = ['-a']

class ExcludeTest(ParadigmTest):
    procflags = ['-e', '<ind>']
    inputs = ['*<n><*>']
    expectedOutputs = ['ab<n><def>:abc']

class SortTest(ParadigmTest):
    procflags = ['-s']
    inputs = ['*<n><*>']
    expectedOutputs = ['ab<n><def>:abc\nab<n><ind>:ab\nn<n><ind>:n\ny<n><ind>:y']
    sortoutput = False

class ExcludeSingleTest(ParadigmTest):
    procdix = 'data/unbalanced-epsilons-mono.dix'
    inputs = ['*<vblex><*>', '*<vblex><*-pres>', '*<vblex><*-inf-pret>']
    expectedOutputs = [
        're<vblex><inf>:re\nre<vblex><pres>:rer\nre<vblex><pres>:res\nre<vblex><pret>:ret',
        're<vblex><inf>:re\nre<vblex><pret>:ret',
        're<vblex><pres>:rer\nre<vblex><pres>:res'
    ]

class OrTagTest(ParadigmTest):
    procdix = 'data/unbalanced-epsilons-mono.dix'
    inputs = ['re<vblex><|pres|pret>', 're<vblex><|inf>', 're<vblex><|xqz>']
    expectedOutputs = [
        're<vblex><pres>:rer\nre<vblex><pres>:res\nre<vblex><pret>:ret',
        're<vblex><inf>:re',
        ''
    ]

class OrTagRepeatTest(ParadigmTest):
    procdix = 'data/unbalanced-epsilons-mono.dix'
    inputs = [
        're<*|vblex|pres|pret>',
        're<*|inf|vblex>',
        're<*|n|adj|vblex|inf>'
    ]
    expectedOutputs = [
        're<vblex><pres>:rer\nre<vblex><pres>:res\nre<vblex><pret>:ret',
        're<vblex><inf>:re',
        're<vblex><inf>:re',
    ]
