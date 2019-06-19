SWIG_INTERFACE = lttoolbox.i

BUILT_SOURCES = %_wrap.cpp.cpp

%_wrap.cpp.cpp: $(SWIG_INTERFACE) setup.py
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install --prefix=$(DESTDIR)$(prefix)
