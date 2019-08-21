# Weighting of automata
The project aims at implementing a set of algorithms for weighting transducers.

- [Weighting of automata](#weighting-of-automata)
  * [Dependencies](#dependencies)
  * [Models Description](#models-description)
      - [lt-weight](#lt-weight)
        * [Methodology](#methodology)
        * [Usage](#usage)
        * [Example](#example)
    + [Weightlist generation algorithms](#weightlist-generation-algorithms)
      - [annotated-corpus-to-weightlist](#annotated-corpus-to-weightlist)
        * [Methodology](#methodology-1)
        * [Usage](#usage-1)
        * [Example](#example-1)
        * [Limitations](#limitations)
        * [Future work](#future-work)
      - [unannotated-corpus-to-weightlist](#unannotated-corpus-to-weightlist)
        * [Methodology](#methodology-2)
        * [Usage](#usage-2)
        * [Example](#example-2)
        * [Future work](#future-work-1)
      - [equal-weightlist](#equal-weightlist)
        * [Methodology](#methodology-3)
        * [Usage](#usage-3)
        * [Example](#example-3)
      - [analysis-length-reweight](#analysis-length-reweight)
        * [Methodology](#methodology-4)
        * [Usage](#usage-4)
        * [Example](#example-4)
        * [Future work](#future-work-2)
      - [w2v-weightlist](#w2v-weightlist)
        * [Methodology](#methodology-5)
        * [Usage](#usage-5)
        * [Example](#example-5)
        * [Limitations](#limitations-1)
        * [Future work](#future-work-3)
  * [Evaluation](#evaluation)
    + [eval/corpus_split.py](#eval-corpus-splitpy)
      - [Usage](#usage-6)
    + [eval/\*\_fit.py](#eval-----fitpy)
      - [Usage](#usage-7)
    + [Example](#example-6)
    + [Results](#results)
  * [Appendix](#appendix)
    + [Xerox regexp](#xerox-regexp)

## Dependencies
**lt-weight**
- `lttoolbox`
- `hfst`

**Most of the weighting scripts**
- `python3`

**Word2vec scripts**
- `tqdm` (A python package for showing the progress)
- `gensim` (A python package for training word2vec models)
`pip install tqdm gensim`

**Evaluation scripts**
- `tabulate` (A python package for generating evaluation results in Markdown table format)
`pip install tabulate`

## Models Description
#### lt-weight

##### Methodology
Add weights to a compiled dictionary using a set of weightlists.
- The weightlists are written as Xerox regexp which is more powerful than using weighted string pairs as it permits matching all analyses of certain prefixes, suffixes.
BUT, take care when using complex regexp since composing a transducer with a complex regexp transducer is time and memory consuming (TODO: WHAT IS THE EXACT COMPLEXITY?)
- The script can make use of a sequence of a weightlists and it considers each weightlist to be a fallback one for paths that weren't weighted by the preceding weightlist.
- It's also advised to use a default weightlist - in the form `[?*]::LARGE_WEIGHT` - as the last weightlist  such that paths that weren't part of any of the weightlists aren't dropped from the final composed transducer and are given large default weight.
- Note: The script makes use of composition of weighted FSTs using `hfst-compose` and assumes that the weights belong to a tropical semi-ring.
So, if the input fst had a path mapping `cat` to `dog` with a weight of `1` and the weightlist was in form `[d o g]::2` then the weighted fst will map `cat` to `dog` with a weight of `3 (1+2)`
TODO: SO?? ADVANTAGE IN ANY WAY?

##### Usage
TODO: PRINT USAGE

##### Example
For the ambiguous English word `saw`, `apertium-eng` already gives it four distinct morphological analyses:
`^saw/saw<n><sg>/saw<vblex><inf>/saw<vblex><pres>/saw<vblex><imp>/see<vblex><past>$`
Let's assume we want to priortize the analyses as follows:
1) `^saw/see<vblex><past>$` as the verb see is somehow common in language.
2) `^saw/saw<n><sg>$` as it's both a popular movie and a useful tool.
3) `^saw/saw<vblex><imp>$` as imperative verbs seem to occur a lot in the linguist's opinion.
4) Any other possible should be given high default weight.

```
$ cat saw.att 
0       1       s       s
1       2       a       a
2       3       w       w
3       9       ε       <n>
9       8       ε       <sg>
3       6       ε       <vblex>
6       8       ε       <inf>
6       8       ε       <pres>
6       8       ε       <imp>
1       4       a       e
4       5       w       e
5       7       ε       <vblex>
7       8       ε       <past>
8

$ lt-comp lr saw.att saw.bin
main@standard 10 13

$ echo '[s e e %<vblex%> %<past%>]::1' > wl1
$ echo '[s a w %<n%> %<sg%>]::2' > wl2
$ echo '[?* %<vblex%> %<imp%>]::3' > wl3
$ echo '[?*]::4' > wl4

$ ./lt-weight saw.bin weighted_saw.bin wl1 wl2 wl3 wl4
Reading from /tmp/tmp.mOItp9wzUO/transducer.hfst and /tmp/tmp.mOItp9wzUO/weighted-regexp.hfst, writing to /tmp/tmp.mOItp9wzUO/weighted-transducer.hfst
Composing text(/tmp/tmp.mOItp9wzUO/transducer.att) and xre(?)...
Reading from /tmp/tmp.mOItp9wzUO/transducer.hfst and /tmp/tmp.mOItp9wzUO/weighted-regexp.hfst, writing to /tmp/tmp.mOItp9wzUO/weighted-transducer.hfst
Composing text(/tmp/tmp.mOItp9wzUO/transducer.att) and xre(?)...
Reading from /tmp/tmp.mOItp9wzUO/transducer.hfst and /tmp/tmp.mOItp9wzUO/weighted-regexp.hfst, writing to /tmp/tmp.mOItp9wzUO/weighted-transducer.hfst
Composing text(/tmp/tmp.mOItp9wzUO/transducer.att) and xre(?)...
Reading from /tmp/tmp.mOItp9wzUO/transducer.hfst and /tmp/tmp.mOItp9wzUO/weighted-regexp.hfst, writing to /tmp/tmp.mOItp9wzUO/weighted-transducer.hfst
Composing text(/tmp/tmp.mOItp9wzUO/transducer.att) and xre(?)...
main@standard 10 13

$ echo 'saw' | lt-proc weighted_saw.bin -W
^saw/see<vblex><past><W:1.000000>/saw<n><sg><W:2.000000>/saw<vblex><imp><W:3.000000>/saw<vblex><pres><W:4.000000>/saw<vblex><inf><W:4.000000>$
```

**Note**:
- wl1 and wl2 can be merged into a single weightlist since they aren't actually acting as fallback for each other.
- On the other hand, if wl3 and wl4 were merged into a single weightlist then the analysis `saw<vblex><imp>` would actually be added twice to the weighted transducer with two different weights 3 and 4!

### Weightlist generation algorithms
#### annotated-corpus-to-weightlist
##### Methodology
Generate a weightlist given an annotated corpus.
The annotated corpus is in form `^surface_form/analyzed_form$`.
The script will estimate the weight for the analyzed form by calculating the probability of the analysis in the corpus.
`P(analysis) = Count(analysis) / size of corpus`
This model acts as a benchmark model for the other unsupervised techniques.

To account for the OOV analyses, laplace smoothing is used such that:
- the weight for analyses that aren't part of the corpus is
`1 / (size of corpus + number of unique analysis in corpus + 1)`
- the weight for an analysis that is part of the corpus is
`(1 + Count(analysis)) / (size of corpus + number of unique analysis in corpus + 1)`

A tweak to the weightlist generation (`--tag_weightlist`) was also added to give priority to analyses with tags that are common in the corpus.
i.e: If the noun `<n>` tag is highly probable in the corpus then we might want to weight OOV analyses with a <n> tag with a lower weight (higher probablility).
However, using the tag weightlist won't make the weights probabilistic.

##### Usage
TODO: PRINT USAGE

##### Example
TODO
##### Limitations
- The `--tag_weightlist` seems to be slow since it envolves the composition of the original fst with a regexp-fst in the form [?* <tag>].

##### Future work
- Make use of advanced techniques other than the unigram counts (example: n-grams).

#### unannotated-corpus-to-weightlist
##### Methodology
To generate a tagged corpus similar to that used in the `annotated-corpus-to-weightlist` method, the script will:
- analyze a raw corpus using a compiled unweighted dictionary.
- make use of a constraint grammar to discard immpossible analyses or select certain ones.
- write all the possible analyses to a file using the following format for each line `^surface_form/analyzed_form$`.
- use the `annotated-corpus-to-weightlist` to estimate a weightlist.

##### Usage
TODO: PRINT USAGE

##### Example
TODO

##### Future work
- Investigate the effect of using a constraint grammar and perform a thorough error analysis

#### equal-weightlist
##### Methodology
Generate a simple weightlist with the same weight for all analyses.
This model acts as the baseline for all the other techniques.

##### Usage
TODO: PRINT USAGE

##### Example
TODO

#### analysis-length-reweight
##### Methodology
Use `hfst-reweight` to directly give a weight of one to all the edges of the finite state transducer.

##### Usage
TODO: PRINT USAGE

##### Example
TODO

##### Future work
- Compare the results of using `equal-weightlist` and `analysis-length-reweight`
Weighting epsilon input might be the reason for the drift in the results of both methods.

#### w2v-weightlist
##### Methodology
Generate a weightlist for words based on a word2vec CBOW (continuous bag of words) model.
- First, use the whole raw corpus to train a word2vec model.
- Then, use a sliding window and for each center word, predict the most probable words given the current context.
- Finally, If the center word isn't ambiguous, just increment the count of its analysis.
Else, For each ambiguous analysis of the center word, Count the number of similar words that aren't ambiguous AND have tags matching the tag of the center word's analysis.

##### Usage
TODO: PRINT USAGE

##### Example
TODO

##### Limitations
- Training a word2vec model using a raw corpus is both time and memory intense (that's why the script currently avoids loading the whole data into memory at the same time)

##### Future work
- Refactor the word2vec training function

## Evaluation
To evaluate and compare the performance of the weighting methods, cross validation is used.

### eval/corpus_split.py
A script to divide a tagged corpus into n folds (where n is a parameter)

#### Usage
TODO: PRINT USAGE

### eval/\*\_fit.py
Each weighting method has a script for training n models and report the metrics for each one in a tabular form
#### Usage
TODO: PRINT USAGE

### Example
```
REPO="../../apertium-kaz"
BIN="${REPO}/kaz.automorf.bin"
TAGGED_CORPUS="${REPO}/corpus/kaz.tagged"
UNTAGGED_CORPUS="${REPO}/kaz-clean"
CONSTRAINT_GRAMMAR="${REPO}/kaz.rlx.bin"
BASE_DIR=$(mktemp -d)
FOLDS_DIR="folds"
CLEANED_CORPUS="${BASE_DIR}/kaz.cleaned"

apertium-cleanstream -n < "$TAGGED_CORPUS" > "$CLEANED_CORPUS"
python eval/corpus_split.py -o "$FOLDS_DIR" "$CLEANED_CORPUS"

python eval/unigram_fit.py -i "$FOLDS_DIR" -b "$BIN" -o temp_uni_bin
python eval/equalweight_fit.py -i "$FOLDS_DIR" -b "$BIN" -o temp_eq_bin
python eval/constraintgrammar_fit.py -i "$FOLDS_DIR" -cg "$CONSTRAINT_GRAMMAR" -corpus "$UNTAGGED_CORPUS" -b "$BIN" -o temp_cg_bin
python eval/analysis_length_fit.py -i "$FOLDS_DIR" -b "$BIN" -o temp_ana_bin
python eval/w2v_fit.py -i "$FOLDS_DIR" -b "$BIN" -o temp_w2v_bin -corpus "$UNTAGGED_CORPUS"

echo 'Uni'
python eval/metrics_report.py -i "$FOLDS_DIR" -b temp_uni_bin
echo 'Eq'
python eval/metrics_report.py -i "$FOLDS_DIR" -b temp_eq_bin
echo 'Cg'
python eval/metrics_report.py -i "$FOLDS_DIR" -b temp_cg_bin
echo 'Length'
python eval/metrics_report.py -i "$FOLDS_DIR" -b temp_ana_bin
echo 'W2V'
python eval/metrics_report.py -i "$FOLDS_DIR" -b temp_w2v_bin
```
### Results
TODO: Add the results on apertium repos

## Appendix
### Xerox regexp
After a series of shot-gun debugging, I found this file (ftp://ftp.cis.upenn.edu/pub/cis639/public_html/docs/xfst.html) and it was a great help in understanding how to use XEROX regexp them -especially the `Commands <-> RE operators` section.
