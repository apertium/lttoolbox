#!/usr/bin/env python3

#TODO: Use streamparser?
import re
import sys

def extract_surface(tagged_line):
	"""Extract the surface form from a tagged line

	A tagged line takes the form ^surface/analysis<tag>$
	i.e: returns surface
	"""
	return re.findall(r'\^.*\/', tagged_line)[0][1:-1]

def extract_analysis(tagged_line):
	"""Extract the Analysis form from a tagged line

	A tagged line takes the form ^surface/analysis<tag>$
	i.e.: returns analysis<tag>
	"""
	return re.sub(r'[\t ]|^(\^.*\/)|(\$)$', '', tagged_line)

def extract_tag_from_analysis(tagged_line):
	"""Extract the tags from a tagged line

	A tagged line takes the form ^surface/analysis<tag>$
	i.e.: returns tag
	"""
	matches = re.findall('<.*>', extract_analysis(tagged_line))
	if len(matches) == 0:
		# TODO: HANDLE THE CASE NO TAGS ARE FOUND
		# raise BaseException()
		return None
	return matches[0]

def generate_regex(analysis, match_all_prefixes=False):
	"""Convert an analysis form into XEROX regex"""

	# Add a space after each token "REGEX concatenation"
	analysis = ' '.join(analysis)

	#TODO: HANDLE THE REST OF THE SPECIAL CHARACTERS
	SPECIAL_REGEX_CHARACTERS = '%,.;!#-—+=@&*_:0?[]()~"\'^$°’\\'
	# Escape special characters
	for special_char in SPECIAL_REGEX_CHARACTERS:
		if special_char == '\\':
			analysis = re.sub(r'\\', r'%\\', analysis)
		else:
			analysis = re.sub(r'\{}'.format(special_char), '%{}'.format(special_char), analysis)

	# Fix the multichar tags:
	# - Remove intermediate spaces
	# - Prepend < and > with %
	analysis = re.sub(r'(\<.*?\>)', lambda multichar_tag:
		'%<{}%>'.format((re.sub(' ', '', multichar_tag.group(0)[1:-1]))), analysis)

	analysis = re.sub(r'[^%]([>])', '%>', analysis)
	analysis = re.sub(r'[^%]([<])', '%<', analysis)

	# TODO: Transform the regex into [?*][REGEX][?*]
	# This may be too slow to be feasible
	if match_all_prefixes:
		return '[?* {}]'.format(analysis)

	# Surround regex with []
	analysis = '[{}]'.format(analysis)

	return analysis
