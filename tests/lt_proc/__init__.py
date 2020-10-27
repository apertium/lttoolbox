# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from proctest import ProcTest

from typing import List


class ValidInput(ProcTest):
    inputs = ["ab",
              "ABC jg",
              "y n"]
    expectedOutputs = ["^ab/ab<n><ind>$",
                       "^ABC/AB<n><def>$ ^jg/j<pr>+g<n>$",
                       "^y/y<n><ind>$ ^n/n<n><ind>$"]

class BiprocSkipTags(ProcTest):
    procdix = "data/biproc-skips-tags-mono.dix"
    procflags = ["-b", "-z"]
    inputs = ["^vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>$"]
    expectedOutputs = ["^vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>/vihki<KEPT><MATCHSOFAR><STILLMATCHING><SOMEHOWKEPT1><@SOMEHOWKEPT2>$"]

class WeightedTransducer(ProcTest):
    procdix = "data/walk-weight.att"
    inputs = ["walk",
              "walks"]
    expectedOutputs = ["^walk/*walk$",
                       "^walks/*walks$"]

class WeightedCatTransducer(ProcTest):
    procdix = "data/cat-weight.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n/cat+v$"]

class WeightedCatInitialTransducer(ProcTest):
    procdix = "data/cat-weight-initial.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatMiddleTransducer(ProcTest):
    procdix = "data/cat-weight-middle.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatFinalTransducer(ProcTest):
    procdix = "data/cat-weight-final.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatHeavyTransducer(ProcTest):
    procdix = "data/cat-weight-heavy.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat$"]

class WeightedCatNegativeTransducer(ProcTest):
    procdix = "data/cat-weight-negative.att"
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n/cat+v$"]

class PrintWeights(ProcTest):
    procdix = "data/cat-weight.att"
    procflags = ["-W"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n<W:11.528235>/cat+v<W:12.559967>$"]

class PrintWeightsNegative(ProcTest):
    procdix = "data/cat-weight-negative.att"
    procflags = ["-W"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n<W:8.828133>/cat+v<W:9.859865>$"]

class PrintNAnalyses(ProcTest):
    procdix = "data/cat-weight.att"
    procflags = ["-N 1"]
    inputs = ["cat"]
    expectedOutputs = ["^cat/cat+n$"]

class LemmaEntryWeights(ProcTest):
    procdix = "data/lemma-entry-weights.dix"
    procflags = ["-W"]
    inputs = ["walk"]
    expectedOutputs = ["^walk/walk<n><W:0.100000>/walk<vblex><W:0.900000>$"]

class AllEntryWeights(ProcTest):
    procdix = "data/entry-weights.dix"
    procflags = ["-W"]
    inputs = ["nanow"]
    expectedOutputs = ["^nanow/nan<n><ma><du><gen><W:32.120000>/nan<n><ma><du><acc><W:34.120000>/nan<n><ma><pl><gen><W:39.120000>/nan<n><ma><pl><acc><W:41.120000>$"]

class Intergeneration(ProcTest):
    procdix = "data/intergen.dix"
    procflags = ["-x"]
    inputs = ["la dona ~dóna tot"]
    expectedOutputs = ["la dona dona tot"]

class GardenPathMwe(ProcTest):
    procdix = "data/gardenpath-mwe.dix"
    inputs          = ["x[ <br/> ]opp.",
                        "legge opp[<br/>]x.",
                       "y[A]x",
                        "[A]y x",
                        "legge opp[<br/>]",
                        "legge[][\n]L[<\/p>\n]",
                       ]
    expectedOutputs = ["^x/*x$[ <br/> ]^opp/opp<pr>$^./.<sent>$",
                        "^legge/legge<vblex><inf>$ ^opp/opp<pr>$[<br/>]^x/*x$^./.<sent>$",
                       "^y/*y$[A]^x/*x$",
                        "[A]^y/*y$ ^x/*x$",
                        "^legge/legge<vblex><inf>$ ^opp/opp<pr>$[<br/>]",
                        "^legge/legge<vblex><inf>$[][\n]^L/*L$[<\/p>\n]",
                       ]

class GardenPathMweNewlines(ProcTest):
    procdix = "data/gardenpath-mwe.dix"
    inputs          = ["""St.[
]Petersburg[
]X y.[][

]F G.[][
]"""]
    expectedOutputs = ["""^St. Petersburg/St. Petersburg<np>$[
][
]^X y/X y<np>$^./.<sent>$[][

]^F/F<np>$ ^G/G<np>$^./.<sent>$[][
]"""
                       ]

class CatMultipleFstsTransducer(ProcTest):
    procdix = "data/cat-multiple-fst.att"
    inputs = ["cat", "cats"]
    expectedOutputs = ["^cat/cat+n/cat+v$", "^cats/cat+n+<pl>$"]

class WordboundBlankAnalysisTest(ProcTest):
    procdix = "data/wordbound-blank.dix"
    inputs          = ["x  [[t:i:123456]]opp.",
                        "[[t:b:456123; t:i:90hfbn]]legge  [[t:s:xyz789]]opp  opp [[t:b:abc124]]x opp.",
                       ]
    expectedOutputs = ["^x/*x$  [[t:i:123456]]^opp/opp<pr>$^./.<sent>$",
                        "[[t:b:456123; t:i:90hfbn]]^legge/legge<vblex><inf>$  [[t:s:xyz789]]^opp/opp<pr>$  ^opp/opp<pr>$ [[t:b:abc124]]^x/*x$ ^opp/opp<pr>$^./.<sent>$",
                       ]

class PostgenerationBasicTest(ProcTest):
    procdix = "data/postgen.dix"
    procflags = ["-p", "-z"]
    inputs          = [ "xyz ejemplo ~o ho nombre.",
                        "xyz ~le la pelota.",
                        "El perro ~de el amigo.",
                        "abc ~les testword"]
    expectedOutputs = [ "xyz ejemplo u ho nombre.",
                        "xyz se la pelota.", 
                        "El perro del amigo.", 
                        "abc le pe test testword"]

class PostgenerationWordboundBlankTest(ProcTest):
    procdix = "data/postgen.dix"
    procflags = ["-p", "-z"]
    inputs          = [ "xyz ejemplo [[t:i:123456]]~o[[/]] [[t:b:abc123; t:i:123456]]ho[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo [[t:b:poim230]]~o[[/]] ho [[t:i:mnbj203]]nombre[[/]].",
                        "xyz ejemplo ~o [[t:b:abc123; t:i:123456]]ho[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo ~o [[t:b:abc123; t:i:123456]]ho[[/]] ~le la [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo [[t:i:1235gb]]~o[[/]] [[t:b:abc123; t:i:123456]]ho[[/]] [[t:b:i4x56fb]]~le[[/]] la nombre.",
                        "xyz [[t:i:123456]]~le[[/]] [[t:b:123gfv]]la[[/]] pelota.",
                        "xyz ~le [[t:b:123gfv]]la[[/]] pelota.",
                        "xyz ejemplo ~o [[t:b:abc123; t:i:123456]]ho[[/]] ~le [[t:b:io1245b]]la[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] [[t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:h5lVhA]]El[[/]] [[t:b:Z9eiLA; t:i:4_tPUA]]perro[[/]] [[t:b:Z9eiLA; t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:npAFwg]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] [[t:i:4_tPUA]]~de[[/]] el [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] ~de [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] ~de el [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]~les[[/]] [[t:b:abc123; t:i:123456]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] ~les [[t:b:abc123; t:i:123456]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]~les[[/]] [[t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]~les pes[[/]] [[t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]~les[[/]] [[t:b:12bsa23]]pes[[/]] [[t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] ~les [[t:b:12bsa23]]pes[[/]] [[t:i:4_tPUA]]~de[[/]] [[t:b:Z9eiLA]]el[[/]] [[t:i:wSM6RQ]]testword[[/]]"]

    expectedOutputs = [ "xyz ejemplo [[t:i:123456; t:b:abc123; t:i:123456]]u ho[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo [[t:b:poim230]]u ho[[/]] [[t:i:mnbj203]]nombre[[/]].",
                        "xyz ejemplo [[t:b:abc123; t:i:123456]]u ho[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo [[t:b:abc123; t:i:123456]]u ho[[/]] se la [[t:b:iopmnb]]nombre[[/]].",
                        "xyz ejemplo [[t:i:1235gb; t:b:abc123; t:i:123456]]u ho[[/]] [[t:b:i4x56fb]]se la[[/]] nombre.",
                        "xyz [[t:i:123456; t:b:123gfv]]se la[[/]] pelota.",
                        "xyz [[t:b:123gfv]]se la[[/]] pelota.",
                        "xyz ejemplo [[t:b:abc123; t:i:123456]]u ho[[/]] [[t:b:io1245b]]se la[[/]] [[t:b:iopmnb]]nombre[[/]].",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] [[t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:h5lVhA]]El[[/]] [[t:b:Z9eiLA; t:i:4_tPUA]]perro[[/]] [[t:b:Z9eiLA; t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:npAFwg]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] [[t:i:4_tPUA]]del[[/]] [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] [[t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]El[[/]] [[t:s:8AjRFw]]perro[[/]] del [[t:i:wSM6RQ]]amigo[[/]][]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]le pe test[[/]] [[t:b:abc123; t:i:123456]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] le pe test [[t:b:abc123; t:i:123456]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]le pe test[[/]] [[t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456]]les pes test[[/]] [[t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:i:123456; t:b:12bsa23]]les pes test[[/]] [[t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]testword[[/]]",
                        "[[t:b:Z9eiLA]]abc[[/]] [[t:b:12bsa23]]les pes test[[/]] [[t:i:4_tPUA; t:b:Z9eiLA]]del[[/]] [[t:i:wSM6RQ]]testword[[/]]"]


class PostgenerationWordboundBlankEscapingTest(ProcTest):
    procdix = "data/postgen.dix"
    procflags = ["-p", "-z"]
    inputs          = [ "Systran ([[t:a:PJD9GA]]http:\/\/www.systran.de\/[[/]]).[] Systran (http:\/\/www.systran.de\/).[]"]

    expectedOutputs = [ "Systran ([[t:a:PJD9GA]]http:\/\/www.systran.de\/[[/]]).[] Systran (http:\/\/www.systran.de\/).[]"]


class PostgenerationWordboundBlankNoRuleMatchTest(ProcTest):
    procdix = "data/postgen.dix"
    procflags = ["-p", "-z"]
    inputs          = [ "[[t:span:HIIiRQ]]Complacer[[/]] [[t:span01:HIIiRQ]]~le[[/]] [[t:span02:HIIiRQ]]ayuda[[/]] [[11t:span:HIIiRQ; t:a:_IOHRg]]mejora[[/]] [[22t:span:HIIiRQ; t:a:_IOHRg]]~la[[/]] [[33t:span:HIIiRQ; t:a:_IOHRg]]prenda[[/]]"]

    expectedOutputs = [ "[[t:span:HIIiRQ]]Complacer[[/]] [[t:span01:HIIiRQ]]le[[/]] [[t:span02:HIIiRQ]]ayuda[[/]] [[11t:span:HIIiRQ; t:a:_IOHRg]]mejora[[/]] [[22t:span:HIIiRQ; t:a:_IOHRg]]la[[/]] [[33t:span:HIIiRQ; t:a:_IOHRg]]prenda[[/]]"]



# These fail on some systems:
#from null_flush_invalid_stream_format import *
