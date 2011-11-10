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
# Makefile to build all available model libraries.

.phony: all clean install distclean

# Append new model library dirs here.
subdirs := nvmemmory

all clean install distclean:
	for d in $(subdirs); do $(MAKE) -C $$d $@; done

