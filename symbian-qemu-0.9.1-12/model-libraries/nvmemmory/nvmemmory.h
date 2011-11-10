/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
* Simple non volatile memory device. nvmemmory.h
*
*/

#ifndef _NVMEMORY_H
#define _NVMEMORY_H

#pragma once

/*! \brief 
NVMEMORY.DLL 
- A simple non volatile sector addressed memory device
- An example of a portable high abstraction model architecture. 
  This DLL is created to serve in multiple different modeling 
  environments and tools. Model portability should be taken into 
  account when further developing and maintaining this device.

USAGE       
nvmemory.dll Example usage provided by syborg_nvmemorydevice.py.
1 - Create an instance of SyborgNVMemory class. 
    This can be performed either by instantiating the C++ class directly 
    by calling the constructor "new SyborgNVMemory( a_sectorsize );".
    Or by using the C API to create an instance "nvmem_create( sector_size )".
    As you can see you need to set the sector size for your device when you create it.
    
    In fact each of the API functions are provided in form of both C and C++ API functions.
    From this on only the C API is explained.

2 - Reset the device by calling "nvmem_reset( self.obj )". 
    This function clears any information stored by the device instance except the sector size.
    Note that for the C API you always need to specify the object which you wish to command.

3 - Create handle to your image file by calling "nvmem_open( self.obj, imagepath )".
    Image is opened in binary mode. Store the handle.
    
    Note that you need to have an image stored to a path on your PC before you can call this function. 
    You must provide the image name and path when calling.
    
    Note also that there is a service provided by this DLL which can create an image for you.
    NVMEMORY_API int32_t nvmem_create_image( SyborgNVMemory* a_syborg_nvmemory, char* a_memoryarrayname, uint32_t a_sectorcount, uint32_t a_sectorsize = NVMEM_DEFAULT_SECTORSIZE_IN_BYTES );
    nvmem_create_image function probably needs further development. You may also create image in your wrapper as done in example usage file.
    
    You may get your memory device size by calling nvmem_get_sector_count( self.obj, nvmemhandle ).
    nvmemhandle is the handle you got when calling nvmem_open.
    nvmem_get_sector_count is handy in cases where you have provided a readymade image for the device.
    In this case you don't need to go and hardcode the image size each time in your wrapper.
    
4 - Initialize callback. Provide a callback function for the device by calling "nvmem_set_callback( self.obj, nvmem_callback )".
    Where the callback is a function pointer of a type "int (*i_NVMemCallBack)(int);".
    Callback is called by DLL when read and write operations are finished. Parameter is the amount of sectors succesfully read or written.
    
5 - Start using your device.
    nvmem_read(  self.obj, nvmemory_sharedmemory_host_address, nvmemhandle, transaction_offset, transaction_size )
    nvmem_write(  self.obj, nvmemory_sharedmemory_host_address, nvmemhandle, transaction_offset, transaction_size )
    
    See syborg_nvmemorydevice.py to learn more about device usage. 
*/

#include "platformtypes.h"

#ifdef WIN32
#ifdef NVMEMORY_EXPORTS
#define NVMEMORY_API __declspec(dllexport)
#else
#define NVMEMORY_API __declspec(dllimport)
#endif
#else
#define NVMEMORY_API
#endif

class NVMEMORY_API SyborgNVMemory;

extern "C"
    {
    const int32_t NVMEM_DEFAULT_SECTORSIZE_IN_BYTES = 512;
    const int32_t NVMEM_MAX_STREAMHANDLES = 16;

    /*Nvmemory error codes*/
    const int32_t NVMEM_OK =                                 0;
    const int32_t NVMEM_ERROR_OUT_OF_FREE_STREAMHANDLES =   -1;
    const int32_t NVMEM_ERROR_FOPEN =                       -2;
    const int32_t NVMEM_ERROR_FCLOSE =                      -3;
    const int32_t NVMEM_ERROR_FFLUSH =                      -4;
    const int32_t NVMEM_ERROR_FSEEK =                       -5;
    const int32_t NVMEM_ERROR_FREAD =                       -6;
    const int32_t NVMEM_ERROR_FWRITE =                      -7;
    const int32_t NVMEM_ERROR_SETVBUF =                     -8;
    const int32_t NVMEM_ERROR_CREATE =                      -9;
    const int32_t NVMEM_ERROR_FPRINTF =                     -10;

    /**
    * Reset the device
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @return TODO an error if fails
    */
    NVMEMORY_API int32_t nvmem_reset( SyborgNVMemory* a_syborg_nvmemory );
    
    /**
    * Create a non volatile memory array object.
    *
    * @param a_sectorsize Size for the sector.
    * @return An object of class SyborgNVMemory.
    */
    NVMEMORY_API SyborgNVMemory * nvmem_create( uint32_t a_sectorsize = NVMEM_DEFAULT_SECTORSIZE_IN_BYTES);

    /**
    * Create a non volatile memory array. A raw image will be created to host filesystem which will 
    * act as a non volatile memory device in client point of view.
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_memoryarrayname Name for the image to be created.
    * @param a_sectorcount Image size in sectors.
    * @param a_sectorsize size for the sector.
    * @return An error code.
    */
    NVMEMORY_API int32_t nvmem_create_image( SyborgNVMemory* a_syborg_nvmemory, char* a_memoryarrayname, uint32_t a_sectorcount, uint32_t a_sectorsize = NVMEM_DEFAULT_SECTORSIZE_IN_BYTES );
    
    /**
    * Create handle to an image
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_memoryarrayname Name and path for the image to be opened.
    * @return Handle to a non volatile memory array.
    */
    NVMEMORY_API int32_t nvmem_open( SyborgNVMemory* a_syborg_nvmemory, char* a_memoryarrayname );

    /**
    * Close handle to a non volatile memory array
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_memoryarrayhandle Handle to be closed.
    * @return Error code.
    */
    NVMEMORY_API int32_t nvmem_close( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle );
    
    /**
    * Flush possible cached content of a non volatile memory array
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_memoryarrayhandle Handle pointing to stream which is flushed to a file.
    * @return Error code.
    */
    NVMEMORY_API int32_t nvmem_flush( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle );

    /**
    * Read from a non volatile memory array to a guest os application memory space
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_client_targetmemoryaddr Host OS address pointing to a place to where the data read should be returned.
    * @param a_memoryarrayhandle Handle to the device image.
    * @param a_memoryarrayoffset Sector offset to a position from where the data is to be read.
    * @param a_sectorcount Amount of sectors to be read.
    * @return Error code.
    */
    NVMEMORY_API void nvmem_read( SyborgNVMemory* a_syborg_nvmemory, uint32_t *a_client_targetmemoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarrayoffset, uint32_t a_sectorcount );

    /**
    * Write to a non volatile memory array from a guest os application memory space
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_client_sourcememoryaddr Host OS address pointing to a place to where to get the data for write operation.
    * @param a_memoryarrayhandle Handle to the device image.
    * @param a_memoryarrayoffset Sector offset to a position to where the data is to be written.
    * @param a_sectorcount Amount of sectors to be written.
    * @return Error code.
    */
    NVMEMORY_API void nvmem_write( SyborgNVMemory* a_syborg_nvmemory, uint32_t *a_client_sourcememoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarrayoffset, uint32_t a_sectorcount );

    /**
    * Get the size of a non volatile memory array
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param a_memoryarrayhandle Handle to the device image.
    * @return Total amount of sectors of the device.
    */
    NVMEMORY_API uint32_t nvmem_get_sector_count( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle );

    /**
    * Store callback
    *
    * @param a_syborg_nvmemory an object of class SyborgNVMemory.
    * @param aGraphicsCallBack pointer to a callback function.
    * @return TODO an error if fails.
    */
    NVMEMORY_API int32_t nvmem_set_callback( SyborgNVMemory* a_syborg_nvmemory, int (*aGraphicsCallBack) (int) );
    }

class NVMEMORY_API SyborgNVMemory
    {
    public:

        SyborgNVMemory( uint32_t a_sectorsize = NVMEM_DEFAULT_SECTORSIZE_IN_BYTES );
        ~SyborgNVMemory( );

        /**
        * Reset the device
        */
        int32_t NVMemReset( );
        /**
        * Create a non volatile memory array. A raw image will be created to host filesystem which will 
        * act as a non volatile memory device in client point of view.
        */
        int32_t NVMemCreateImage( char* a_memoryarrayname, uint32_t a_sectorcount, uint32_t a_sectorsize = NVMEM_DEFAULT_SECTORSIZE_IN_BYTES );
        /**
        * Return handle to a non volatile memory array
        */
        int32_t NVMemOpen( char* a_memoryarrayname );
        /**
        * Close handle to a non volatile memory array
        */
        int32_t NVMemClose( int32_t a_memoryarrayhandle );
        /**
        * Flush possible cached content of a non volatile memory array
        */
        int32_t NVMemFlush( int32_t a_memoryarrayhandle );
        /**
        * Read from a non volatile memory array to a guest os application memory space
        */
        void NVMemRead( uint32_t *a_client_targetmemoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarrayoffset, uint32_t a_sectorcount );
        /**
        * Write to a non volatile memory array from a guest os application memory space
        */
        void NVMemWrite( uint32_t *a_client_sourcememoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarrayoffset, uint32_t a_sectorcount );
        /**
        * Get the size of a non volatile memory array
        */
        uint32_t NVMemGetSectorCount( int32_t a_memoryarrayhandle );
        /**
        * Store callback
        */
        int32_t NVMemSetCallback( int (*aNVMemCallBack) (int) );

    private:
        FILE *i_filestream[NVMEM_MAX_STREAMHANDLES];
        uint32_t iNVMemSectorSizeInBytes;
        int (*i_NVMemCallBack)(int); 
    };

#endif // _NVMEMORY_H
