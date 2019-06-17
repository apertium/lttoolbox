#!/usr/bin/env python3

#TODO: Use streamparser?
import re
import sys

def extract_analysis(tagged_line):
	return re.sub(r'[\t ]|^(\^.*\/)|(\$)$', '', tagged_line)

def generate_regex(analysis):
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

	# Surround regex with []
	analysis = '[{}]'.format(analysis)

	# TODO: Transform the regex into [?*][REGEX][?*]
	# This may be too slow to be feasible

	return analysis
