#include <unicode/uchar.h>
#include <unicode/ustdio.h>
#include <unicode/normalizer2.h>
#include <getopt.h>
#include <iostream>
#include <libgen.h>
#include <unicode/ustream.h>

using namespace std;
using namespace icu;

void endProgram(char* name)
{
  if(name != NULL) {
    cout << basename(name) << " v" << PACKAGE_VERSION << ": normalize a text stream" << endl;
    cout << "USAGE: " << basename(name) << " [-cth] [input_file [output_file]]" << endl;
    cout << "   -c, --nfc:    use Normal Form canonical Composition normalizer" << endl;
    cout << "   -t, --nft:    use Normal Form for Translation normalizer (default)" << endl;
    cout << "   -h, --help:   print this message and exit" << endl;
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
  bool nfc = false;
  UFILE* input = u_finit(stdin, NULL, NULL);
  UFILE* output = u_finit(stdout, NULL, NULL);

#if HAVE_GETOPT_LONG
  int option_index = 0;
#endif
  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
      {
       {"nfc",   no_argument, 0, 'c'},
       {"nft",   no_argument, 0, 't'},
       {"help",  no_argument, 0, 'h'},
       {0, 0, 0, 0}
      };
    int cnt = getopt_long(argc, argv, "cth", long_options, &option_index);
#else
    int cnt = getopt(argc, argv, "cth");
#endif
    if (cnt == -1) {
      break;
    }
    switch (cnt) {
    case 'c':
      nfc = true;
      break;

    case 't':
      nfc = false;
      break;

    case 'h':
    default:
      endProgram(argv[0]);
      break;
    }
  }
  string infile, outfile;
  switch (argc - optind) {
  case 0:
    break;

  case 1:
    infile = argv[argc-1];
    break;

  case 2:
    infile = argv[argc-2];
    outfile = argv[argc-1];
    break;

  default:
    endProgram(argv[0]);
    break;
  }

  if(infile != "" && infile != "-") {
    input = u_fopen(infile.c_str(), "rb", NULL, NULL);
    if(!input) {
      cerr << "Error: Cannot open file '" << infile << "' for reading." << endl;
      exit(EXIT_FAILURE);
    }
  }

  if(outfile != "" && outfile != "-")
  {
    output = u_fopen(outfile.c_str(), "wb", NULL, NULL);
    if(!output)
    {
      cerr << "Error: Cannot open file '" << outfile << "' for writing." << endl;
      exit(EXIT_FAILURE);
    }
  }

  UErrorCode err;
  const Normalizer2* norm;
  if(nfc) {
    norm = Normalizer2::getNFCInstance(err);
  } else {
    norm = Normalizer2::getInstance("/home/daniel/lttoolbox/lttoolbox/nft.nrm", "nft", UNORM2_COMPOSE, err);
  }
  if (U_FAILURE(err)) {
    cerr << "Error: Unable to load normalizer." << endl;
    exit(EXIT_FAILURE);
  }

  UnicodeString buff;
  while (true) {
    UChar32 c = u_fgetcx(input);
    // TODO: skip blanks
    if (!buff.isEmpty() &&
        (c == U_EOF || c == '\0' || norm->hasBoundaryBefore(c))) {
      u_fprintf(output, "%S", norm->normalize(buff, err).getTerminatedBuffer());
      buff = "";
    }
    if (c == U_EOF) {
      break;
    } else if (c == '\0') {
      u_fputc('\0', output);
    } else {
      buff += c;
    }
  }
  return 0;
}
