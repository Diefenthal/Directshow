

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed Jul 07 14:35:01 2010
 */
/* Compiler settings for .\idlparsertypes.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __idlparsertypes_h_h__
#define __idlparsertypes_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_idlparsertypes_0000_0000 */
/* [local] */ 

typedef unsigned short u_short;

typedef struct _teletext_ebu_t
    {
    unsigned char language[ 4 ];
    u_short type;
    BYTE magazine_number;
    BYTE page_number;
    } 	teletext_ebu_t;

typedef struct _subtitling_t
    {
    unsigned char language[ 4 ];
    u_short type;
    } 	subtitling_t;

typedef struct _pesvideodet_t
    {
    LONG width;
    LONG height;
    UINT aspect_x;
    UINT aspect_y;
    LONGLONG timeperframe;
    UINT profile;
    UINT level;
    UINT bitrate;
    } 	pesvideodet_t;

typedef struct _pesaudiodet_t
    {
    u_short channels;
    u_short mpeg_layer;
    u_short mpeg_version;
    UINT sampfreq;
    UINT bitrate;
    } 	pesaudiodet_t;

typedef struct _interop_piddetails_t
    {
    u_short stream_type;
    u_short private_type;
    u_short pid;
    pesvideodet_t pes_vdet;
    pesaudiodet_t pes_adet;
    BOOL pes_supported;
    unsigned char language[ 4 ];
    int subtitling_descriptor_count;
    int teletext_descriptor_count;
    } 	interop_piddetails_t;



extern RPC_IF_HANDLE __MIDL_itf_idlparsertypes_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_idlparsertypes_0000_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


