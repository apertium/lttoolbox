%module analysis

%{
#define SWIG_FILE_WITH_INIT
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

class FST: private FSTProcessor
{
public:
  /**
   * Reads from input_path and stores result at output_path
   */  
  void init_analysis(char *automorf_path, char *input_path, char *output_path);
  bool validity() const;

};

bool 
FST::validity() const
{
  return valid();
}

void 
FST::init_analysis(char *automorf_path, char *input_path, char *output_path)
{
  setDictionaryCaseMode(true);
  LtLocale::tryToSetLocale();
  FILE *in = fopen(automorf_path, "rb");
  load(in);
  initAnalysis();
  FILE *input = fopen(input_path, "r"), *output = fopen(output_path, "w");
  analysis(input, output);
  fclose(in);
  fclose(input);
  fclose(output);
}

%}


#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

class FST: private FSTProcessor
{
public:
  /**
   * Reads from input_path and stores result at output_path
   */  
  void init_analysis(char *automorf_path, char *input_path, char *output_path);
  bool validity() const;
};  
