import sys
import os.path as path
sys.path.append(path.realpath("."))

import unittest
import null_flush

if __name__ == "__main__":
    suite = unittest.TestLoader().loadTestsFromModule(null_flush)
    unittest.TextTestRunner(verbosity = 2).run(suite)

