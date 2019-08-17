%module lttoolbox

%include <lttoolbox/fst_processor.h>
%include <lttoolbox/lt_locale.h>


%typemap(in) char ** {
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *py_obj = PyList_GetItem($input, i);
      if (PyUnicode_Check(py_obj)) {
        $1[i] = strdup(PyUnicode_AsUTF8(py_obj));
      }
      else {
        PyErr_SetString(PyExc_TypeError, "list must contain strings");
        free($1);
        return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError, "not a list");
    return NULL;
  }
}

%typemap(freearg) char ** {
  free((char *) $1);
}

%inline%{
#define SWIG_FILE_WITH_INIT
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <getopt.h>

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

  void lt_proc(char argc, char **argv, char *input_path, char *output_path)
  {
    FILE* input = fopen(input_path, "r");
    FILE* output = fopen(output_path, "w");
    int maxAnalyses;
    int maxWeightClasses;
    int cmd = 0;
    int c = 0;
    optind = 1;
    while (true)
    {
      c = getopt(argc, argv, "abcegi:r:lmndopxstzwvCIWN:L:h");
      if (c == -1)
      {
        break;
      }
      switch(c)
      {
        case 'c':
          setCaseSensitiveMode(true);
          break;

        case 'i':
          setIgnoredChars(true);
          parseICX(optarg);
          break;

        case 'r':
          setRestoreChars(true);
          parseRCX(optarg);

        case 'I':
          setUseDefaultIgnoredChars(false);
          break;

        case 'W':
          setDisplayWeightsMode(true);
          break;

        case 'N':
          maxAnalyses = atoi(optarg);
          if (maxAnalyses > 1)
          {
            setMaxAnalysesValue(maxAnalyses);
          }
          break;

        case 'L':
          maxWeightClasses = atoi(optarg);
          if (maxWeightClasses > 1)
          {
            setMaxWeightClassesValue(maxWeightClasses);
          }
          break;

        case 'e':
        case 'a':
        case 'b':
        case 'o':
        case 'l':
        case 'm':
        case 'g':
        case 'n':
        case 'd':
        case 'p':
        case 'x':
        case 't':
        case 's':
        case 'C':
          if(cmd == 0)
          {
            cmd = c;
          }
          break;

        case 'z':
          setNullFlush(true);
          break;

        case 'w':
          setDictionaryCaseMode(true);
          break;
        default:
          break;
      }
    }

    switch(cmd)
    {
      case 'n':
        initGeneration();
        generation(input, output, gm_clean);
        break;

      case 'g':
        initGeneration();
        generation(input, output);
        break;

      case 'd':
        initGeneration();
        generation(input, output, gm_all);
        break;

      case 'l':
        initGeneration();
        generation(input, output, gm_tagged);
        break;

      case 'm':
        initGeneration();
        generation(input, output, gm_tagged_nm);
        break;

      case 'C':
        initGeneration();
        generation(input, output, gm_carefulcase);
        break;

      case 'p':
        initPostgeneration();
        postgeneration(input, output);
        break;

      case 'x':
        initPostgeneration();
        intergeneration(input, output);
        break;

      case 's':
        initAnalysis();
        SAO(input, output);
        break;

      case 't':
        initPostgeneration();
        transliteration(input, output);
        break;

      case 'o':
        initBiltrans();
        setBiltransSurfaceForms(true);
        bilingual(input, output);
        break;

      case 'b':
        initBiltrans();
        bilingual(input, output);
        break;

      case 'e':
        initDecomposition();
        analysis(input, output);
        break;

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
