SWIG_SOURCES = analysis.i

# TODO: Check libdivvun to remove call to all
all: analysis_wrap.cpp

analysis_wrap.cpp: $(SWIG_SOURCES) setup.py
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir) -o $@ $<
	$(PYTHON) setup.py install
