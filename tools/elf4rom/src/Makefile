#
# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

CC=g++
CFLAGS=-Wall -O3 -g3
CPPFLAGS=-I. -I.. -I$/include/boost
BASE_CFLAGS=
LDFLAGS=-g -L$(MINGW_HOME)/local/lib
LIBS=-lelf -lboost_program_options -lboost_regex -lboost_filesystem
ELF4ROM=elf4rom.exe

OBJS=dwarfabbrevmanager.o
OBJS+=dwarfarangesmanager.o
OBJS+=dwarfframemanager.o
OBJS+=dwarfinfomanager.o
OBJS+=dwarflinemanager.o
OBJS+=dwarflocexpr.o
OBJS+=dwarflocmanager.o
OBJS+=dwarfmanager.o
OBJS+=dwarfnamemanager.o
OBJS+=dwarfrangesmanager.o
OBJS+=dwarfutils.o
OBJS+=e32romimage.o
OBJS+=elfheader.o
OBJS+=elfphdr.o
OBJS+=elfrom.o
OBJS+=elfromerror.o
OBJS+=elfsection.o
OBJS+=elfsectionmanager.o
OBJS+=elfstringtable.o
OBJS+=elfsymboltablemanager.o
OBJS+=filefragment.o
OBJS+=inputfile.o
OBJS+=main.o
OBJS+=outputfile.o
OBJS+=processoptions.o

%.o: %.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) $(BASE_CFLAGS) -c -o $@ $<

$(ELF4ROM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJS) $(ELF4ROM)
