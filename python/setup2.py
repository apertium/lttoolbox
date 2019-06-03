#!/usr/bin/python

"""
Setup for SWIG Python bindings for lttoolbox
"""

from distutils.core import setup, Extension

# swig -python -c++ -o analysis_wrap.cpp analysis.i

def getenv_list(var_name):
    from os import getenv
    var = getenv(var_name)
    if var:
        return var.split()
    else:
        return []


def get_sources():
    from os import path
    sources = ['analysis_wrap.cpp']
    cc_sources = '\
             att_compiler.cc compiler.cc entry_token.cc expander.cc match_exe.cc \
             match_node.cc match_state.cc pattern_list.cc \
             regexp_compiler.cc sorted_vector.cc transducer.cc tmx_compiler.cc'

    paths = '/home/kawai/Desktop/git_push/lttoolbox/lttoolbox/'
    cc = 'alphabet.cc compression.cc fst_processor.cc lt_locale.cc node.cc state.cc trans_exe.cc xml_parse_util.cc' 
    # for i in cc.split():
    #     sources.append(path + i)
    # sources.extend(cc_sources.split())
    sources.extend([path.join(paths, f) for f in cc.split()])
    return sources

analysis_module = Extension(
        '_analysis',
        # sources=['analysis_wrap.cpp', '/home/kawai/Desktop/git_push/lttoolbox/lttoolbox/fst_processor.cc'],
        sources=get_sources(),
        include_dirs=['/home/kawai/Desktop/git_push/lttoolbox', '/usr/include/libxml2'],
        library_dirs=['/usr/include/libxml2'],
        extra_compile_args=getenv_list('CPPFLAGS') + getenv_list('CXXFLAGS'),
        extra_link_args= (getenv_list('LDFLAGS')) + ['-lxml2'])


setup(name='python-@PACKAGE@',
      version='@PACKAGE_VERSION@',
      description='SWIG interface to @PACKAGE_NAME@',
      long_description="SWIG interface to @PACKAGE_NAME@ for use in apertium-python",
      # TODO: author, maintainer, url
      author_email='@PACKAGE_BUGREPORT@',
      license='GPL-3.0+',
      maintainer_email='@PACKAGE_BUGREPORT@',
      py_modules=['analysis'],
      ext_modules=[analysis_module])
