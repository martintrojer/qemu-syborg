# Copyright (C) 2008 CodeSourcery
# All rights reserved.

## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = 1.9 foreign

WARNING_CFLAGS = -Wall
PIC_CFLAGS = -fPIC

AM_CFLAGS = $(WARNING_CFLAGS) $(CDEFINES)
AM_CCASFLAGS = $(AM_CFLAGS)

bin_PROGRAMS = dtc ftdump

AM_CPPFLAGS = -I$(srcdir)/libfdt
AM_YFLAGS = -d

dtc_SOURCES = dtc.c flattree.c fstree.c data.c livetree.c \
	treesource.c srcpos.c checks.c dtc-parser.y dtc-lexer.l

ftdump_SOURCES = ftdump.c

BUILT_SOURCES = version_gen.h dtc-parser.h
version_gen.h: Makefile
	echo "#define DTC_VERSION \"DTC @PACKAGE_VERSION@ @PKGVERSION@\"" > $@
	echo "#define BUG_URL \"@BUGURL@\"" >> $@
