import re
import sys
import numpy as np
import pandas as pd

#TODO: HANDLE THE REST OF THE SPECIAL CHARACTERS
special_regex_chars = '%,.;!#-—+*:0?[]()~"\''

def clean_tag_patterns(reg):
	whitesace_free_reg = re.sub(' ', '', reg)
	return '%{}%>'.format(whitesace_free_reg[:-1])

def clean_line(line):
	line = line.strip()
	if line.endswith('$"'):
		# ERROR LINE LIKE ^./.<sent>$"
		line = line[:-2]
	line = re.sub(r'(.)', r'\1 ', line)

	for special_char in special_regex_chars:
		line = re.sub('\\{}'.format(special_char), '%{}'.format(special_char), line)
	
	line = re.sub(r'(\<.*?\>)', lambda m: 
		clean_tag_patterns(m.group(0)),line)
	# HANDLE TAGS
	line = line.strip()
	line = '[{}]'.format(line)
	# line = '[?*][{}][?*]'.format(line)
	return line

if __name__ == '__main__':
	FILE_NAME = sys.argv[1]
	with open(FILE_NAME, 'r') as f:
		lines =[clean_line(line) for line in f.readlines() if not line.startswith('*')]

	lines = list(pd.Series(lines).value_counts().reset_index().apply(lambda r: '{}::{}'.format(r['index'].strip(), -np.log(r[0]/len(lines))), axis=1))

	with open(FILE_NAME, 'w') as f:
		for line in lines:
			f.write(line+'\n')