#!/usr/bin/env python3

import os
import sys
import random
import argparse
from pathlib import Path

def write_to_file(lines_list, file_name):
	with open(file_name, 'w') as f:
		f.write('\n'.join(lines_list))

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='split a tagged corpus into n folds')
	parser.add_argument('input_tagged_corpus',
						type=argparse.FileType('r'),
						help='a tagged corpus')
	parser.add_argument('-n', '--no_of_folds', type=int, default=5,
						help='number of folds')
	parser.add_argument('-s', '--seed', type=int, default=42,
						help='number of folds')
	parser.add_argument('-o', '--output_directory',required=True,
						help='output directory')
	args = parser.parse_args()
	cv = args.no_of_folds
	output_directory = args.output_directory
	input_tagged_corpus = args.input_tagged_corpus
	random.seed(args.seed)

	if not os.path.exists(output_directory):
		os.mkdir(output_directory)

	splitted_corpus = [[] for _ in range(cv)]

	# For each line, generate a random index
	for line_number, line in enumerate(input_tagged_corpus):
		splitted_corpus[random.randint(0, cv - 1)].append(line.strip())

	# Extract the base file name of the corpus
	corpus_file_name = Path(input_tagged_corpus.name).name

	for cv_index in range(cv):
		write_to_file(splitted_corpus[cv_index],
					  str(Path(output_directory, '{}_{}'.format(corpus_file_name, cv_index))))
