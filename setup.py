from distutils.core import setup, Extension

setup(
    ext_modules=[Extension('libhfs', ['main.c'])],
)
