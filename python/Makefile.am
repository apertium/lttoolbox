SWIG_SOURCES = analysis.i

all: analysis_wrap.cpp

analysis_wrap.cpp: $(SWIG_SOURCES)
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir) -c++ -o $@ $<
	(python setup.py build) 