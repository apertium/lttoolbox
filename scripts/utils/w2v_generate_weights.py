#!/usr/bin/env python3

import sys
import math
from collections import Counter
from utils import extract_tag_from_analysis, generate_regex

def get_weight(word_a, similar_a):
	"""
	Return the count of times the word had analyses tags
	similar to those of an un-ambiguous similar word

	word_a: The word analyses
	similar_a: The similar word analyses (list of strings)
	"""

	# TODO: Pass a string instead of a list of size 1
	word_a = [analysis.strip('$').split('/')[1:]
		for analysis in word_a if analysis][0]
	similar_a = [analysis.strip('$').split('/')[1:]
		for analysis in similar_a if analysis]
	if not word_a:
		return None

	if word_a[0].startswith('*'):
		return None

	if len(word_a) == 1:
		return Counter({generate_regex(word_a[0]):1})

	unambig_analyses = [a[0] for a in similar_a if len(a)==1 and not a[0].startswith('*')]
	tags = [extract_tag_from_analysis(word_analysis) for word_analysis in unambig_analyses]
	tags_count = Counter(tags)

	return Counter({generate_regex(analysis): tags_count[extract_tag_from_analysis(analysis)] for analysis in word_a})

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Generate a weightlist using a set of words and their similar words given the context')
	parser.add_argument('--words_file',
						type=argparse.FileType('r'),
						required=True,
						help='words file')
	parser.add_argument('--similar_file',
						type=argparse.FileType('r'),
						required=True,
						help='similar words file (each line in tab-delimited)')
	parser.add_argument('--output_weightlist',
						type=argparse.FileType('w'),
						required=True,
						help='The output weightlist using the similar words analysis')
	parser.add_argument('--default_weightlist',
						type=argparse.FileType('w'),
						required=True,
						help='The weightlist containing a laplace smoothed weight')
	args = parser.parse_args()
	words_file = args.words_file
	similar_words_file = args.similar_words_file
	output_weightlist = args.output_weightlist
	default_weightlist = args.default_weightlist
	
	words = [[l.strip()] for l in words_file.readlines() if l.strip()]
	similar_words = [l.strip().split() for l in similar_words_file.readlines() if l.strip()]

	weights = [get_weight(w, s) for w, s in zip(words, similar_words)]
	weights = [w for w in weights if w]
	counts = sum(weights, Counter())
	sum_counts = sum(counts.values()) + len(counts) + 1

	with open(output_weightlist, 'w') as f:
		for t in counts:
			f.write('{}::{}\n'.format(t, -math.log((1 + counts[t]) / sum_counts )))
	
	with open(default_weightlist, 'w') as f:
		f.write('[?*]::{}'.format(-math.log(1 / sum_counts)))
