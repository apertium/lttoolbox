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
   * Imitates functionality of lt-proc using file path
   */
  void lt_proc(char arg, char *dictionary_path, char *input_path, char *output_path);
};


void 
FST::lt_proc(char arg, char *dictionary_path, char *input_path, char *output_path)
{
  FILE *in = fopen(dictionary_path, "rb");
  load(in);
  FILE *input = fopen(input_path, "r"), *output = fopen(output_path, "w");
  switch(arg)
  {
    case 'g':
      initGeneration();
      generation(input, output);
      break;
    case 'b':
      initBiltrans();
      bilingual(input, output);
      break;
    case 'p':
      initPostgeneration();
      intergeneration(input, output);
      break;
    case 'w':
      setDictionaryCaseMode(true);
    case 'a':
    default:
      initAnalysis();
      analysis(input, output);
      break;
  }

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
   * Imitates functionality of lt-proc using file path
   */
  void lt_proc(char arg, char *dictionary_path, char *input_path, char *output_path);
};
