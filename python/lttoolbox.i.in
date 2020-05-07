%module lttoolbox

%include <lttoolbox/fst_processor.h>
%include <lttoolbox/lt_locale.h>


%typemap(in) (int argc, char **argv) {
  if (PyTuple_Check($input)) {
    int i = 0;
    $1 = PyTuple_Size($input);
    $2 = (char **) malloc(($1 + 1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *py_obj = PyTuple_GetItem($input, i);
      if (PyUnicode_Check(py_obj)) {
        $2[i] = strdup(PyUnicode_AsUTF8(py_obj));
      }
      else {
        PyErr_SetString(PyExc_TypeError, "tuple must contain strings");
        free($2);
        return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError, "not a tuple");
    return NULL;
  }
}

%typemap(freearg) (int argc, char **argv) {
  free((char *) $2);
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

  void lt_proc(int argc, char **argv, char *input_path, char *output_path)
  {
    FILE* input = fopen(input_path, "r");
    FILE* output = fopen(output_path, "w");
    int cmd = 0;
    int c = 0;
    optind = 1;
    while (true)
    {
      c = getopt(argc, argv, "bgpw");
      if (c == -1)
      {
        break;
      }
      switch(c)
      {
        case 'b':
        case 'g':
        case 'p':
          if(cmd == 0)
          {
            cmd = c;
          }
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
      case 'b':
        bilingual(input, output);
        break;

      case 'g':
        generation(input, output);
        break;

      case 'p':
        postgeneration(input, output);
        break;

      default:
        analysis(input, output);
        break;
      }

    fclose(input);
    fclose(output);
  }
};

%}
