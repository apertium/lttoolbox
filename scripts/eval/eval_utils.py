#!/usr/bin/env python3

import re
import subprocess
from pathlib import Path

def get_apertium_analyses(X, weighted_bin, base_dir, only_one_analysis=True):
    #TODO: WHY DOES ADDING A DOT WORK AS A SEPARATOR?
    joined_X = ' .\n'.join([x for x in X + ['\n']])

    base_input_as_file = str(Path(base_dir, 'base_input_as_file'))
    with open(base_input_as_file, 'w') as f:
        f.write(joined_X)

    deformatted_input = str(Path(base_dir, 'formatted_input'))
    assert(subprocess.run(['apertium-destxt', '-n', base_input_as_file, deformatted_input]).returncode == 0)

    analysed_output = str(Path(base_dir, 'analysis_output'))
    reformatted_output = str(Path(base_dir, 'reformatted_output'))
    # TODO: Is cleaning the reformatted output file needed?

    processing_command = ['lt-proc', weighted_bin, deformatted_input, analysed_output]
    if only_one_analysis:
    	processing_command.append('-N 1')

    subprocess.run(processing_command)
    subprocess.run(['apertium-retxt', analysed_output, reformatted_output])

    with open(reformatted_output, 'r') as f:
    	analyses = [a.strip() for a in f.readlines() if a.strip()]
    if only_one_analysis:
	    return [analysis[analysis.find('/') + 1: analysis.find('$')]
	            for analysis in analyses]
    else:
        return [analysis.strip('$').split('/')[1:]
                for analysis in analyses]

def split_X_y(file_lines):
    '^With/with<pr>$'
    splitted_lines = [line.strip()[1:-1].split('/') for line in file_lines if line.strip()]

    tokens = [l[0] for l in splitted_lines]
    targets = [l[1] for l in splitted_lines]

    assert(len(tokens)==len(targets)), 'Token and Target vectors size mismatch ({}!={})'.format(len(tokens), len(targets))

    return tokens, targets
