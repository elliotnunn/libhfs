from distutils.core import setup, Extension

c_code = Extension('demo', sources = ['main.c'])

setup (name = 'libhfs',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [c_code])
