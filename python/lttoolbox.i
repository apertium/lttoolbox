%module lttoolbox

%{
#define SWIG_FILE_WITH_INIT
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

class FST: public FSTProcessor
{
public:
  /**
   * Reads from input_path and stores result at output_path
   */  
  void analyze(char *automorf_path, char *input_path, char *output_path);

  /**
   * Reads from input_path and stores result at output_path
   */
  void generate(char *autogen_path, char *input_path, char *output_path);
};


void 
FST::analyze(char *automorf_path, char *input_path, char *output_path)
{
  FILE *in = fopen(automorf_path, "rb");
  load(in);
  initAnalysis();
  FILE *input = fopen(input_path, "r"), *output = fopen(output_path, "w");
  analysis(input, output);
  fclose(in);
  fclose(input);
  fclose(output);
}

void
FST::generate(char *autogen_path, char *input_path, char *output_path)
{
  FILE *in = fopen(autogen_path, "rb");
  load(in);
  initGeneration();
  FILE *input = fopen(input_path, "r"), *output = fopen(output_path, "w");
  generation(input, output);
  fclose(in);
  fclose(input);
  fclose(output);
}

%}


%include <lttoolbox/fst_processor.h>
%include <lttoolbox/lttoolbox_config.h>
%include <lttoolbox/my_stdio.h>
%include <lttoolbox/lt_locale.h>

class FST: public FSTProcessor
{
public:
  /**
   * Reads from input_path and stores result at output_path
   */  
  void analyze(char *automorf_path, char *input_path, char *output_path);

  /**
   * Reads from input_path and stores result at output_path
   */
  void generate(char *autogen_path, char *input_path, char *output_path);
};
