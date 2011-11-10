#
# Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description: syborg_nvmemorydevice.py
# A simple non volatile memory device nvmemory.dll python wrapper
# Represents a non volatile memory device register interface for quest OS in QEMU Syborg environment.
#
# Gets the following from devicetree (.dtb configuration file) 
#       drive_size - the size of the non volatile memory array to be created if there is no such thing available when the system is started
#       sector_size - the size of the sector for the memory device
#       drive_image_name - the name of the image to be used
#
# Creates an empty image of specified size and name if there is not image available when the system is started
#

import ctypes
import qemu
import sys
import os
import array
import platform
import re

class syborg_nvmemorydevice(qemu.devclass):
    # 256 MB default empty drive size if there is no readymade image available
    DEFAULT_DRIVE_SIZE = 0x10000000
    DEVICE_SECTOR_SIZE = 0x200
    DRIVE_NAME = "qemudrive.img"
    DRIVE_PATH = "nvmemory"
    
    # Memory device registers
    R_NVMEM_ID                                = 0x0000
    R_NVMEM_TRANSACTION_OFFSET                = 0x0004
    R_NVMEM_TRANSACTION_SIZE                  = 0x0008
    R_NVMEM_TRANSACTION_DIRECTION             = 0x000c
    R_NVMEM_TRANSACTION_EXECUTE               = 0x0010
    R_NVMEM_SHARED_MEMORY_BASE                = 0x0014
    R_NVMEM_NV_MEMORY_SIZE                    = 0x0018
    R_NVMEM_SHARED_MEMORY_SIZE                = 0x001c
    R_NVMEM_STATUS                            = 0x0020
    R_NVMEM_ENABLE                            = 0x0024
    R_NVMEM_LASTREG                           = 0x0028  # not a register, address of last register
    
    NVMEM_TRANSACTION_READ              = 1
    NVMEM_TRANSACTION_WRITE             = 2
    # Variables to store the information for current transaction
    shared_memory_base                  = 0
    shared_memory_size                  = 0
    transaction_offset                  = 0
    transaction_size                    = 0
    transaction_direction               = 0
    # Variables to validate transaction
    transaction_offset_set              = 0
    transaction_size_set                = 0
    transaction_direction_set           = 0
    nvmemory_sector_count                = 0
    
    drive_size                          = 0
    sector_size                         = 0
    drive_image_name                    = ""
    host_plat = platform.platform();

    def create(self):
        print "syborg_nvmemorydevice: create\n"
        
        # Get properties
        self.drive_size = self.properties["drive_size"]
        self.sector_size = self.properties["sector_size"]
        self.drive_image_name = self.properties["drive_image_name"]
        
        print "drive size: ", self.drive_size
        print "sector size: ", self.sector_size
        print "drive name: ", self.drive_image_name

        drive_path_and_name = os.path.join(self.DRIVE_PATH, self.drive_image_name)  
        # Save working directory
        self.working_dir = os.getcwd()
        nvmem_lib = ""
        open_mode = 0
        if re.match('^linux',self.host_plat,re.I):
			nvmemlib_name = "libnvmemmory.so"
			open_mode = os.O_RDWR
        else:
			nvmemlib_name = "nvmemmory.dll"
			open_mode = os.O_RDWR|os.O_BINARY			
        
        # Open the nvmemory library
        try:
            self.nvmemlib = ctypes.CDLL(nvmemlib_name)
        except Exception, e:
            print repr(e)
            sys.exit("syborg_nvmemorydevice: nvmemmory load failed")
        
        # Create an instance of non volatile memory handler class
        self.obj = self.nvmemlib.nvmem_create( self.sector_size )
        self.nvmemlib.nvmem_reset( self.obj )
        
        # Create drive image path
        try:
            print "syborg_nvmemorydevice: Check drive image path\n"
            os.mkdir( self.DRIVE_PATH )
        except:
            # Here we could check why we failed - usually because the path already exists \n"
            pass
        try:
            self.filehandle = os.open( drive_path_and_name, open_mode )
            os.close( self.filehandle )
        except:
            print "syborg_nvmemorydevice: drive image not found - create\n"
            self.filehandle = open( drive_path_and_name, "wb" )
            # Initialize file with zeroes. This may take a while
            temparray = array.array("B", [0,0,0,0,0,0,0,0])
            arraylength = temparray.buffer_info()[1] * temparray.itemsize
            multiplier = self.drive_size / arraylength / 128
            temparray = temparray * multiplier
            arraylength = temparray.buffer_info()[1] * temparray.itemsize
            print "array length: ", arraylength
            index = 0
            while index < 128:
                temparray.tofile(self.filehandle)
                index = index+1
        
        # Create path and get handle to the raw memory array
        imagepath = os.path.join(self.working_dir, drive_path_and_name)
        print "imagepath: ", imagepath
        self.nvmemhandle = self.nvmemlib.nvmem_open( self.obj, imagepath )
        if( self.nvmemhandle < 0 ):
            error_msg = "syborg_nvmemorydevice: nvmem_open error: ", self.nvmemhandle
            sys.exit( error_msg )
        
        # Initialize callback and get memory sector count
        self.initialize_nvmem_callback()
        self.nvmemory_sector_count = self.nvmemlib.nvmem_get_sector_count( self.obj, self.nvmemhandle )
        print "syborg_nvmemorydevice: created\n"
            
    def updateIrq(self,new_value):
        self.set_irq_level(0, new_value)

    def nvmem_request_callback(self, result):
        #print "graphics_request_callback: " , result
        self.status_reg = result
        self.updateIrq(1)
        return 0
        
    def initialize_nvmem_callback(self):
        self.CALLBACKFUNC = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)
        self.nvmem_callback = self.CALLBACKFUNC(self.nvmem_request_callback)
        self.nvmemlib.nvmem_set_callback( self.obj, self.nvmem_callback )

    def read_reg(self, offset):
        offset >>= 2
        #print "read register: 0x%x" % (offset) 
        if offset == self.R_NVMEM_ID:
            return 0xDEADBEEF
        elif offset == self.R_NVMEM_TRANSACTION_OFFSET:
            return self.transaction_offset
        elif offset == self.R_NVMEM_TRANSACTION_SIZE:
            return self.transaction_size
        elif offset == self.R_NVMEM_TRANSACTION_DIRECTION:
            return self.transaction_direction
        elif offset == self.R_NVMEM_SHARED_MEMORY_BASE:
            return self.shared_memory_base
        elif offset == self.R_NVMEM_SHARED_MEMORY_SIZE:
            return self.shared_memory_size
        elif offset == self.R_NVMEM_NV_MEMORY_SIZE:
            return self.nvmemory_sector_count
        elif offset == self.R_NVMEM_STATUS:
            self.updateIrq(0)
            return self.status_reg
        else:
            reg_read_error = "syborg_nvmemorydevice: Illegal register read at: ", offset 
            sys.exit( reg_read_error )

    def write_reg(self, offset, value):
        offset >>= 2
        #print "write register: 0x%x value: 0x%x" % (offset, value) 
        if offset == self.R_NVMEM_TRANSACTION_OFFSET:
            self.transaction_offset = value
            self.transaction_offset_set = 1
        elif offset == self.R_NVMEM_TRANSACTION_SIZE:
            self.transaction_size = value
            self.transaction_size_set = 1
        elif offset == self.R_NVMEM_TRANSACTION_DIRECTION:
            self.transaction_direction = value
            self.transaction_direction_set = 1
        elif offset == self.R_NVMEM_TRANSACTION_EXECUTE:
            if( (self.transaction_offset_set == 0) | (self.transaction_size_set == 0) | (self.transaction_direction_set == 0) ):
                error_msg = "syborg_nvmemorydevice: Illegal transaction! All the required parameters are not set" 
                sys.exit( error_msg )
            elif(self.transaction_size == 0 ):
                error_msg = "syborg_nvmemorydevice: Zero size transaction issued!" 
                sys.exit( error_msg )
            else:
                if( self.transaction_direction == self.NVMEM_TRANSACTION_READ ):
                    self.nvmemlib.nvmem_read(  self.obj, self.nvmemory_sharedmemory_host_address, self.nvmemhandle, self.transaction_offset, self.transaction_size )
                elif( self.transaction_direction == self.NVMEM_TRANSACTION_WRITE ):
                    self.nvmemlib.nvmem_write(  self.obj, self.nvmemory_sharedmemory_host_address, self.nvmemhandle, self.transaction_offset, self.transaction_size )
                else:
                    error_msg = "syborg_nvmemorydevice: Transaction direction not set!" 
                    sys.exit( error_msg )
                self.transaction_offset_set = 0
                self.transaction_size_set = 0
                self.transaction_direction_set = 0
        elif offset == self.R_NVMEM_SHARED_MEMORY_BASE:
            self.shared_memory_base = value
        elif offset == self.R_NVMEM_SHARED_MEMORY_SIZE:
            self.shared_memory_size = value
        elif offset == self.R_NVMEM_ENABLE:
            if( value > 0 ):
                self.nvmemory_memregion = qemu.memregion( self.shared_memory_base, self.shared_memory_size )
                self.nvmemory_sharedmemory_host_address = self.nvmemory_memregion.region_host_addr()
                print"syborg_nvmemorydevice: host addr: 0x%08x" % (self.nvmemory_sharedmemory_host_address)
        else:
            reg_write_error = "syborg_nvmemorydevice: Illegal register write to: ", offset 
            sys.exit( reg_write_error )

    # Device class properties
    regions = [qemu.ioregion(0x1000, readl=read_reg, writel=write_reg)]
    irqs = 1
    name = "syborg,nvmemorydevice"
    properties = {"drive_size":DEFAULT_DRIVE_SIZE, "sector_size":DEVICE_SECTOR_SIZE, "drive_image_name":DRIVE_NAME}

qemu.register_device(syborg_nvmemorydevice)
