SWIG_INTERFACE = lttoolbox.i

BUILT_SOURCES = lttoolbox_wrap.cpp

lttoolbox_wrap.cpp: $(SWIG_INTERFACE) setup.py
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install
