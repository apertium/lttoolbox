<!-- -*- mode: markdown -*- -->

lttoolbox
===============================================================================

lttoolbox contains finite state tools for lexical processing,
morphological analysis and generation of words. Analysis is the
process of splitting a word like `cats` into its lemma `cat` and the
grammatical information `<n><pl>`. Generation is the opposite process.

The three main programs are lt-comp, the compiler, lt-proc, the
processor, and lt-expand, which generates all possible mappings
between surface forms and lexical forms in the dictionary.

Executables built by this package:

* `lt-comp`: compiler, execute without parameters to show usage
  instructions.

* `lt-proc`: processor, typical options are -a (lexical analyser,
  default option), -g (lexical generator) and -p (lexical
  post-generator). Using -h will show all flags.

* `lt-expand`: generates all the pairs of transductions of a given
  dictionary. Execute without parameters to show the instructions of
  use.

* `lt-trim`: trims a compiled analyser to only contain entries which
  would pass through a compiled bidix, creating a new compiled and
  trimmed analyser.

* `lt-compose`: composes two compiled transducers (applying output of
  the first to input of the second), with support for flipping labels
  and allowing incomplete matches.

* `lt-print`: prints the arcs of a transducer in [ATT format][3].

* `lt-append`: merges two compiled dictionaries.

* `lt-paradigm`: extracts all paths from a compiled dictionary
  matching an input pattern.

* `lsx-comp`: an alias of `lt-comp`.

There is also a C++ API that you can link to (see how [apertium][1] or
[apertium-lex-tools][2] do this).

See https://wiki.apertium.org/wiki/Lttoolbox for usage examples and
more information.

Installation
===============================================================================

There are binaries available for Debian, Ubuntu, Fedora, CentOS, OpenSUSE,
Windows, and macOS. We package both nightly builds and releases.
See https://wiki.apertium.org/wiki/Installation for more information.
Only build from source if you either want to change this tool's behavior,
or are on a platform we don't yet package for.

Requirements:

* A C++ compiler capable of C++17
* CMake >= 3.12
* libxml2 >= 2.6.17
* ICU
* utfcpp

Building & installing:

* ./cmake.sh
* make
* make install

[1]: https://github.com/apertium/apertium
[2]: https://github.com/apertium/apertium-lex-tools
[3]: https://wiki.apertium.org/wiki/ATT_format
