#ifndef PLATFORMTYPES_H
#define PLATFORMTYPES_H
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
// TODO: We should have a proper type definition file created common for standalone model libraries
typedef signed __int8         int8_t;
typedef signed __int16        int16_t;
typedef signed __int32        int32_t;
typedef signed __int64        int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int8     u_int8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int16    u_int16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int32    u_int32_t;
typedef unsigned __int64    uint64_t;
typedef unsigned __int64    u_int64_t;
typedef unsigned char byte_t;
#endif


#endif // PLATFORMTYPES_H
