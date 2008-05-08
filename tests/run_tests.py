import sys
import os.path as path
sys.path.append(path.realpath("."))

import unittest
import lt_proc

if __name__ == "__main__":
    suite = unittest.TestLoader().loadTestsFromModule(lt_proc)
    unittest.TextTestRunner(verbosity = 2).run(suite)

