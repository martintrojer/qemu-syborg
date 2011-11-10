# Copyright (c) 2010 Symbian Foundation.
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Mike Kinghan, mikek@symbian.org, for Symbian Foundation - Initial contribution
# 
# Description:
# Bottom level Makefile to build libnvmemmory.so in the current directory

include ../../../qemu-symbian-svp/config-host.mak

sources := nvmemmory.cpp
objs := nvmemmory.o
libname := libnvmemmory.so
soname := $(libname).1
targ := $(soname).0
 
.phony: all build clean install distclean uninstall

cflags=-DHAVE_STDINT_H -DNVMEMORY_EXPORTS
ifneq ($(DEBUG_LIBS),)
cflags+="-O0 -g"
else
cflags+=-O2
endif

vpath %.cpp ..

all: $(targ)

%.o : %.cpp
	g++ $(cflags) -fPIC -I../../commoninc -c -o $@ $<

$(objs): $(sources)

$(targ): $(objs)
	g++ $(cflags) -shared -Wl,-soname,$(soname) -Wl,-l,stdc++ -o $@ $(objs)

clean:
	rm -f $(objs) $(targ)

install: $(targ)
	if [ ! -d "$(DESTDIR)$(libdir)" ]; then mkdir -p "$(DESTDIR)$(libdir)"; fi && \
	$(INSTALL) -m 755 $(targ) "$(DESTDIR)$(libdir)" && \
	rm -f "$(DESTDIR)$(libdir)/$(libname)" && \
	ln -s "$(DESTDIR)$(libdir)/$(targ)" "$(DESTDIR)$(libdir)/$(libname)"

distclean: clean
#	Nothing for distclean

uninstall:
	rm -f "$(DESTDIR)$(libdir)/$(libname)" "$(DESTDIR)$(libdir)/$(targ)" && \
	files=`ls -A "$(DESTDIR)$(libdir)"` && \
	if [ -z "$$files" ]; then rmdir "$(DESTDIR)$(libdir)"; fi


 
