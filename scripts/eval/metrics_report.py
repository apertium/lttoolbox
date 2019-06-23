#!/usr/bin/env python3

import os
import argparse
import tempfile
import tabulate
import statistics
from pathlib import Path
from eval_utils import get_apertium_analyses, split_X_y

def get_sorted_files_in_directory(dir):
	return sorted(os.listdir(dir))

def compute_weighted_precision(tag, metrics_dict, corpus_size):
	den = (metrics_dict[tag]['TP'] + metrics_dict[tag]['FP'])
	if not den:
		return 0

	return ((metrics_dict[tag]['support'] / corpus_size) *
		(metrics_dict[tag]['TP'] / den))

def compute_weighted_recall(tag, metrics_dict, corpus_size):
	den = (metrics_dict[tag]['TP'] + metrics_dict[tag]['FN'])
	if not den:
		return 0

	return ((metrics_dict[tag]['support'] / corpus_size) *
		(metrics_dict[tag]['TP'] / den))

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='compute metrics for a compiled dictionary')
	parser.add_argument('-i', '--input_directory', required=True,
						help='input directory of n folds')
	parser.add_argument('-b', '--apertium_bins', required=True,
						help='directory of compiled dictionary')
	args = parser.parse_args()

	base_dir = tempfile.mkdtemp()

	precision = []
	recall = []
	for testing_corpus, bin_file in zip(
		get_sorted_files_in_directory(args.input_directory),
		get_sorted_files_in_directory(args.apertium_bins)):
		test_corpus = str(Path(args.input_directory, testing_corpus))

		with open(test_corpus, 'r') as f:
			X, y = split_X_y(f.readlines())
			pred = get_apertium_analyses(X, Path(args.apertium_bins, bin_file), base_dir)
			assert(len(y) == len(pred)), 'Target and Predicted vectors size mismatch ({}!={})'.format(len(y), len(pred))
			metrics_dict = {}

			metrics_vars = ['TP', 'FP', 'FN', 'support']
			for target, prediction in zip(y, pred):
				# IGNORE MISSING TARGET TAGS?
				if target.startswith('*'):
					continue
				# IGNORE MISSING PREDICTION TAGS?
				if prediction.startswith('*'):
					continue
				if not target in metrics_dict:
					metrics_dict[target] = {var:0 for var in metrics_vars}
				metrics_dict[target]['support'] += 1
				metrics_dict[target]['TP'] += target == prediction
				metrics_dict[target]['FN'] += target != prediction
				if not prediction in metrics_dict:
					metrics_dict[prediction] = {var:0 for var in metrics_vars}
				metrics_dict[prediction]['FP'] += target != prediction

			average_precision = 0
			average_recall = 0
			for tag in metrics_dict:
				prec = compute_weighted_precision(tag, metrics_dict, len(X))
				if prec:
					average_precision += prec
				rec = compute_weighted_recall(tag, metrics_dict, len(X))
				if rec:
					average_recall += rec
			recall.append(average_recall)
			precision.append(average_precision)

	metrics_dict = {'testing_corpus': get_sorted_files_in_directory(args.input_directory),
					'precision': precision,
					'recall': recall}

	print('Precision: {0:0.5f} +- {1:0.5f}'.format(statistics.mean(precision), statistics.stdev(precision)))
	print('Recall: {0:0.5f} +- {1:0.5f}'.format(statistics.mean(recall), statistics.stdev(recall)))

	print(tabulate.tabulate(metrics_dict, headers=metrics_dict.keys(), showindex=False, tablefmt='github'))
	print()
