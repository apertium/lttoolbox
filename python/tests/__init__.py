import os
import sys
import tempfile
import unittest

base_path = os.path.join(os.path.abspath(os.path.dirname(__file__)), '..')
sys.path.append(base_path)

import analysis

class TestAnalysisunittest.TestCase):
    def test_FST(self):
        with tempfile.NamedTemporaryFile('w') as input_file, tempfile.NamedTemporaryFile('r') as output_file:
            input_text = 'cats\n'
            automorf_path = "/usr/share/apertium/apertium-eng/eng.automorf.bin"
            input_file.write(input_text)
            input_file.flush()
            x = analysis.FST()
            x.init_analysis(automorf_path, input_file.name, output_file.name)
            output = output_file.read()
            self.assertEqual(output, "cats<n><pl>")