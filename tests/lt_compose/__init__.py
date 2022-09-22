# -*- coding: utf-8 -*-

from basictest import ProcTest
import unittest


class ComposeProcTest(unittest.TestCase, ProcTest):
    monodix = "data/compose1.dix"
    monodir = "lr"
    bidix = "data/pp2p.dix"
    bidir = "lr"
    procflags = ["-z"]
    composeflags = ["--inverted", "--anywhere"]

    def compileTest(self, tmpd):
        self.compileDix(self.monodir, self.monodix, binName=tmpd+'/f.bin')
        self.compileDix(self.bidir, self.bidix, binName=tmpd+'/g.bin')
        self.callProc('lt-compose',
                      self.composeflags + [tmpd+"/f.bin",
                                           tmpd+"/g.bin",
                                           tmpd+"/compiled.bin"])
        # The above already asserts retcode, so if we got this far we know it
        # compiled fine:
        return True


class ComposeSimpleCompound(ComposeProcTest):
    procflags = ["-e", "-z"]
    inputs = ["oppy", "appy",
              "py",
              "opp", "app"]
    expectedOutputs = ["^oppy/opp<n>+py$", "^appy/app<n>+py$",
                       "^py/py$",
                       "^opp/*opp$", "^app/*app$"]


class ComposeNotEverywhere(ComposeProcTest):
    procflags = ["-e", "-z"]
    inputs = ["upp", "up", "uppy"]
    expectedOutputs = ["^upp/upp<n>$",
                       "^up/*up$",
                       "^uppy/upp<n>+py$"]


class ComposeAnchored(ComposeProcTest):
    composeflags = ["--inverted"]
    bidix = "data/upp2up.dix"
    procflags = ["-e", "-z"]
    inputs = ["upp", "up",
              "tuppy", "tupp",
              "uppy", "upppy", "py",
              "opp", "oppy",
              "app", "appy"]
    expectedOutputs = ["^upp/*upp$", "^up/*up$",
                       "^tuppy/*tuppy$", "^tupp/*tupp$",
                       "^uppy/upp<n>+py$", "^upppy/upp<n>+py$", "^py/py$",
                       "^opp/*opp$", "^oppy/*oppy$",
                       "^app/*app$", "^appy/*appy$"
                       ]
