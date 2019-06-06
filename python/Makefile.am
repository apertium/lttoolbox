SWIG_SOURCES = lttoolbox.i

BUILT_SOURCES = lttoolbox_wrap.cpp lttoolbox.py

lttoolbox_wrap.cpp: $(SWIG_SOURCES) setup.py
	$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir) -o $@ $<
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install
