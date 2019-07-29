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
  FST(char *dictionary_path)
  {
    FILE *dictionary = fopen(dictionary_path, "rb");
    load(dictionary);
    fclose(dictionary);
  }
  void lt_proc(char arg, char *input_path, char *output_path)
  {
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

    fclose(input);
    fclose(output);
  }
};

%}


%include <lttoolbox/fst_processor.h>
%include <lttoolbox/lttoolbox_config.h>
%include <lttoolbox/my_stdio.h>
%include <lttoolbox/lt_locale.h>

class FST: public FSTProcessor
{
public:
  FST(char *dictionary_path);
  void lt_proc(char arg, char *input_path, char *output_path);
};
