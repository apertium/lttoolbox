SWIG_SOURCES = analysis.i

BUILT_SOURCES = analysis_wrap.cpp analysis.py

analysis_wrap.cpp: $(SWIG_SOURCES) setup.py
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir) -o $@ $<
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install
