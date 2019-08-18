#!/usr/bin/env python3

import sys
import tqdm
import gensim
import string
import argparse

class Dataset:
	"""
	Wrap corpus for avoiding complete loading of file into memory
	"""
	def __init__(self, file, window_size=4):
		self.corpus_file = file
		self.window_size = window_size
		self.gram_size = 2 * self.window_size + 1
		self.corpus_file.seek(0)

	def __iter__(self):
		self.grams = []
		return self

	def __next__(self):
		# TODO: Consider using faster data-structures
		if not self.grams:
			self.grams = [self.read_word() for _ in range(self.gram_size)]
			return self.grams
		word = self.read_word()
		if not word:
			raise StopIteration
		self.grams.pop(0)
		self.grams.append(word)
		return self.grams

	def read_word(self):
		# TODO: Improve the way to get a word from a file
		word = ''
		while True:
			c = self.corpus_file.read(1)
			if c.isspace() and word:
				return word
			elif not c:
				return None if not word else word
			word = word + c
		return None if not word else word

def get_similar_tokens(context, word2vec):
	""" Find the most probable words given bag of context words

	context: A list of context words
	word2vec: A fitted word2vec model
	"""
	similar_words = word2vec.predict_output_word(context)
	if not similar_words:
		return []
	return  [w for w, _ in similar_words]

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Generate the set of words and similar words using a raw corpus file')
	parser.add_argument('--corpus',
						type=argparse.FileType('r'),
						required=True,
						help='large raw corpus file')
	parser.add_argument('--output_words_file',
						type=argparse.FileType('w'),
						required=True,
						help='The words of the corpus, one word per line')
	parser.add_argument('--output_similar_words_file',
						type=argparse.FileType('w'),
						required=True,
						help='The set of similar words for the words of the corpus, tab-delimited')
	args = parser.parse_args()
	corpus_file = args.corpus
	output_words_file = args.output_words_file
	output_similar_words_file = args.output_similar_words_file

	word2vec = gensim.models.Word2Vec(Dataset(corpus_file, 2), min_count=1)

	for gram in tqdm.tqdm(Dataset(corpus_file, 2)):
		center_word = gram.pop(len(gram) // 2)
		if not center_word or not all(gram):
			continue
		similar_words = get_similar_tokens(gram, word2vec)
		if not similar_words:
			continue
		output_words_file.write(center_word + '\n')
		output_similar_words_file.write('\t'.join(similar_words) + '\n')
