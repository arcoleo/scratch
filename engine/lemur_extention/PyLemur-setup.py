#!/usr/bin/python

import sys, os
from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc
from distutils.util import *

plt = get_platform()

print plt

compile_args = []
user = ''

if ((plt[:6] == 'macosx') or (plt[:6] == 'darwin')):
	compile_args = ['-arch', 'i386']
	goouidir = '/Users/arcoleo/gooui'
	platform_args = ('MACOSX',1)
	user = 'arcoleo'
	libdirs = ['/Users/arcoleo/gooui/server/engine/lemur-inst/lib']
	lemurBaseDir = '/Users/arcoleo/cvs/lemur/'
#	if plt[7:11] == '10.5':
#		print "\n\tCompiling for m64\n"
#		compile_args = ['-arch', 'x86_64', '-m64']
else:
	platform_args = ('LINUX',1)
	if 'arcoleo' == os.getlogin():
		print 'arcoleo'
		user = 'arcoleo'
		#goouidir = '/home/arcoleo/www/gooui'
		goouidir = '/var/repo/hg-gooui/gooui'
		libdirs = ['/home/arcoleo/www/gooui/server/engine/lemur-inst/lib', '/var/repo/hg-gooui/gooui/server/engine/lemur-inst/lib']
	elif 'kylefox2' == os.getlogin():
		print 'kylefox2'
		user = 'kylefox2'
		goouidir = '/home/kylefox2/www/gooui'
		libdirs = ['/home/kylefox2/www/gooui/server/engine/lemur-inst/lib']
	else:
		goouidir = '/var/www/gooui'
		libdirs = ['/home/kylefox2/www/gooui/server/engine/lemur-inst/lib']
	#lemurBaseDir = '/home/' + user + '/cvs/lemur/'
	lemurBaseDir = '/var/repo/hg-gooui/gooui/server/engine/lemur-4.6/'

print "platform_args", platform_args

incdirs = [	'/var/repo/hg-gooui/gooui/server/lemur-inst/include', lemurBaseDir + 'index/include',
			lemurBaseDir + 'utility/include',
			lemurBaseDir + 'parsing/include',
			lemurBaseDir + 'retrieval/include',
			lemurBaseDir + 'cluster/include']

print os.getlogin()

libs = ['pthread', 'lemur', 'm', 'z'] 

setup(	name="pylemur", 
		version="1.0",
		
	ext_modules=[
		Extension("pylemur",  ["PyLemur.cpp"],
			extra_compile_args = compile_args,
			include_dirs=incdirs,
			library_dirs=libdirs,
			libraries=libs,
			define_macros=[	('P_NEEDS_GNU_CXX_NAMESPACE','1'),
							('HAVE_STRFTIME',None),
							platform_args],
			undef_macros=['HAVE_FOO','HAVE_BAR'])
	]
)
