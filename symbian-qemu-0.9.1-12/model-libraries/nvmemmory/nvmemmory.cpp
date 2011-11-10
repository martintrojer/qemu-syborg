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
* Simple non volatile memory device. nvmemmory.cpp
*
*/

extern "C" 
    {
#include <stdio.h>
    }
#include "nvmemmory.h"

SyborgNVMemory::SyborgNVMemory( uint32_t a_sectorsize ):
    iNVMemSectorSizeInBytes( a_sectorsize )
    {
    for( int32_t index = 0; index < NVMEM_MAX_STREAMHANDLES; index += 1 ) 
        {
        i_filestream[index] = NULL;
        }
    }

SyborgNVMemory::~SyborgNVMemory()
    {
    NVMemReset();
    }

int32_t SyborgNVMemory::NVMemReset( )
    {
    /*Initialize the file array*/
    for( int32_t index = 0; index < NVMEM_MAX_STREAMHANDLES; index += 1 ) 
        {
        if( i_filestream[index] != NULL )
            {
            fclose( i_filestream[index] );
            i_filestream[index] = NULL;
            }
        }
    return 0;
    }

int32_t SyborgNVMemory::NVMemCreateImage( char* a_memoryarrayname, uint32_t a_sectorcount, uint32_t a_sectorsize )
    {
    /* Create a temporary MByte buffer array for image creation purpose */
    const uint32_t tempbufsize = 1024 * 1024;
    uint8_t tempbuf[ tempbufsize ];
    uint32_t index = 0;
    FILE *filestream = NULL;
    char mode1[4] = {"rb"};
    char mode2[4] = {"wb"};
    int32_t ret = NVMEM_ERROR_CREATE;
    uint32_t temparraysectorcount = tempbufsize / a_sectorsize;
    iNVMemSectorSizeInBytes = a_sectorsize;
            
    /* Try to open the specified file. If it exists we do not create a new one */
    filestream = fopen( a_memoryarrayname, &mode1[0] );
    if( filestream == NULL )
        {
        /* Fill MBR with zeroes */
        for( index = 0; index < iNVMemSectorSizeInBytes; index += 1 )
            {
            tempbuf[index] = 0;
            }
        /* Open a temporary file handle. Create the file*/
        filestream = fopen( a_memoryarrayname, &mode2[0] );

        if( ret != NULL )
            {
            ret = NVMEM_OK;
            for( index = 0; (index < a_sectorcount) && (ret == NVMEM_OK); index += temparraysectorcount )
                {
                /* Print one array of zeroes to our temporary buffer */
                if( fwrite( tempbuf, iNVMemSectorSizeInBytes, temparraysectorcount, filestream ) < 0 )
                    {
                    ret = NVMEM_ERROR_FWRITE;
                    }
                }
            fclose( filestream );
            }
        else
            {
            ret = NVMEM_ERROR_FOPEN;
            }
        }
    return ret;
    }

int32_t SyborgNVMemory::NVMemOpen( char* a_memoryarrayname )
    {
    char mode[4] = {"rb+"};
    int32_t handle = NVMEM_ERROR_OUT_OF_FREE_STREAMHANDLES;
    /* Search for a free handle position and assign if found */
    int32_t index = 0;
    for( ; index < NVMEM_MAX_STREAMHANDLES; index += 1 ) 
        {
        if( i_filestream[index] == NULL )
            {
            i_filestream[index] = fopen( a_memoryarrayname, &mode[0] );
            if( i_filestream[index] != NULL )
                {
                handle = index;
                printf("handle created: %d\n", index);
                }
            else
                {
                handle = NVMEM_ERROR_FOPEN;
                }
            break;
            }
        }
    return handle;
    }

int32_t SyborgNVMemory::NVMemClose( int32_t a_memoryarrayhandle )
    {
    int32_t result = NVMEM_ERROR_FCLOSE;
    if( fclose( i_filestream[a_memoryarrayhandle] ) == 0 )
        {
        result = NVMEM_OK;
        }
    return result;
    }

int32_t SyborgNVMemory::NVMemFlush( int32_t a_memoryarrayhandle )
    {
    int32_t result = NVMEM_ERROR_FFLUSH;
    if( fflush( i_filestream[a_memoryarrayhandle] ) == 0 )
        {
        result = NVMEM_OK;
        }
    return result;
    }

void SyborgNVMemory::NVMemRead( uint32_t *a_client_targetmemoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarraysectoroffset, uint32_t a_sectorcount )
    {
    //printf("SyborgNVMemory::NVMemRead: sectorpos:%d sectorcount:%d hostaddr: 0x%08x handle: %d\n", a_memoryarraysectoroffset, a_sectorcount, a_client_targetmemoryaddr, a_memoryarrayhandle );
    uint32_t items_read = 0;
    long streamoffset = a_memoryarraysectoroffset * iNVMemSectorSizeInBytes;
    int32_t result = fseek( i_filestream[a_memoryarrayhandle], streamoffset, SEEK_SET );
    if( result == 0 )   
        {
        items_read = fread( a_client_targetmemoryaddr, iNVMemSectorSizeInBytes, a_sectorcount, i_filestream[a_memoryarrayhandle] );
        /*Check that everything is read*/
        if( items_read != a_sectorcount )
            {
            result = NVMEM_ERROR_FREAD;
            }
        else
            {
            result = items_read;
            }
        }
    else /*error*/
        {
        result = NVMEM_ERROR_FSEEK;
        }
    i_NVMemCallBack( result );
    }

void SyborgNVMemory::NVMemWrite( uint32_t *a_client_sourcememoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarraysectoroffset, uint32_t a_sectorcount )
    {
    //printf("SyborgNVMemory::NVMemWrite: sectorpos:%d sectorcount:%d hostaddr: 0x%08x\n", a_memoryarraysectoroffset, a_sectorcount, a_client_sourcememoryaddr );
    uint32_t items_written = 0;
    long streamoffset = a_memoryarraysectoroffset * iNVMemSectorSizeInBytes;
    int32_t result = fseek( i_filestream[a_memoryarrayhandle], streamoffset, SEEK_SET );
    if( result == 0 )   
        {
        items_written = fwrite( a_client_sourcememoryaddr, iNVMemSectorSizeInBytes, a_sectorcount, i_filestream[a_memoryarrayhandle] );
        /*Check that everything is written*/
        if( items_written != a_sectorcount )
            {
            result = NVMEM_ERROR_FWRITE;
            }
        else
            {
            result = items_written;
            }
        }
    else /*error*/
        {
        result = NVMEM_ERROR_FSEEK;
        }
    i_NVMemCallBack( result );
    }

uint32_t SyborgNVMemory::NVMemGetSectorCount( int32_t a_memoryarrayhandle )
    {
    long byte_size = 0;
    uint32_t sector_count = 0;
    printf("use handle: %d\n", a_memoryarrayhandle);
    fseek( i_filestream[a_memoryarrayhandle], 0, SEEK_END );
    byte_size = ftell( i_filestream[a_memoryarrayhandle] );
    sector_count = byte_size / iNVMemSectorSizeInBytes;
    return sector_count;
    }

int32_t  SyborgNVMemory::NVMemSetCallback( int (*aNVMemCallBack) (int) )
    {
    i_NVMemCallBack = aNVMemCallBack;
    return 0;
    }


extern "C"
    {
    NVMEMORY_API SyborgNVMemory * nvmem_create( uint32_t a_sectorsize )
        {
        return new SyborgNVMemory( a_sectorsize );
        }

    NVMEMORY_API int32_t nvmem_reset( SyborgNVMemory* a_syborg_nvmemory ) 
        {
        return a_syborg_nvmemory->NVMemReset();
        }

    NVMEMORY_API int32_t nvmem_create_image( SyborgNVMemory* a_syborg_nvmemory, char* a_memoryarrayname, uint32_t a_sectorcount, uint32_t a_sectorsize )
        {
        return a_syborg_nvmemory->NVMemCreateImage( a_memoryarrayname, a_sectorcount, a_sectorsize );
        }

    NVMEMORY_API int32_t nvmem_open( SyborgNVMemory* a_syborg_nvmemory, char* a_memoryarrayname ) 
        {
        return a_syborg_nvmemory->NVMemOpen( a_memoryarrayname );
        }

    NVMEMORY_API int32_t nvmem_close( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle ) 
        {
        return a_syborg_nvmemory->NVMemClose( a_memoryarrayhandle );
        }

    NVMEMORY_API int32_t nvmem_flush( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle ) 
        {
        return a_syborg_nvmemory->NVMemFlush( a_memoryarrayhandle );
        }

    NVMEMORY_API void nvmem_read( SyborgNVMemory* a_syborg_nvmemory, uint32_t *a_client_targetmemoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarraysectoroffset, uint32_t a_sectorcount ) 
        {
        a_syborg_nvmemory->NVMemRead( a_client_targetmemoryaddr, a_memoryarrayhandle, a_memoryarraysectoroffset, a_sectorcount );
        }

    NVMEMORY_API void nvmem_write( SyborgNVMemory* a_syborg_nvmemory, uint32_t *a_client_sourcememoryaddr, int32_t a_memoryarrayhandle, uint32_t a_memoryarraysectoroffset, uint32_t a_sectorcount ) 
        {
        a_syborg_nvmemory->NVMemWrite( a_client_sourcememoryaddr, a_memoryarrayhandle, a_memoryarraysectoroffset, a_sectorcount );
        }

    NVMEMORY_API uint32_t nvmem_get_sector_count( SyborgNVMemory* a_syborg_nvmemory, int32_t a_memoryarrayhandle )
        {
        return a_syborg_nvmemory->NVMemGetSectorCount( a_memoryarrayhandle );
        }
    
    NVMEMORY_API int32_t nvmem_set_callback( SyborgNVMemory* a_syborg_nvmemory, int (*aGraphicsCallBack) (int) )
        {
        return a_syborg_nvmemory->NVMemSetCallback( aGraphicsCallBack );
        }
    }
