

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sun Aug 30 17:42:09 2015
 */
/* Compiler settings for ifc.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ifc_h_h__
#define __ifc_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IUdpMulticast_FWD_DEFINED__
#define __IUdpMulticast_FWD_DEFINED__
typedef interface IUdpMulticast IUdpMulticast;
#endif 	/* __IUdpMulticast_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_ifc_0000_0000 */
/* [local] */ 

typedef unsigned short u_short;

#define	MAX_DESC_NAME_LEN	( 255 )

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
    u_short composition_page_id;
    u_short ancillary_page_id;
    } 	subtitling_t;

typedef struct _pesvideodet_t
    {
    LONG width;
    LONG height;
    UINT aspect_x;
    UINT aspect_y;
    BYTE afd;
    LONGLONG timeperfieldNom;
    LONGLONG timeperfieldDenom;
    UINT profile;
    UINT level;
    UINT bitrate;
    u_short interlaced_indicator;
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
    u_short pid;
    BOOL pes_supported;
    unsigned char language[ 4 ];
    int subtitling_descriptor_count;
    int teletext_descriptor_count;
    u_short pcr_pid;
    BOOL data_ok;
    } 	interop_piddetails_t;

typedef 
enum _ts_structure_enum_t
    {	TS_STANDARD	= 0,
	TS_TIMECODE	= ( TS_STANDARD + 1 ) ,
	TS_OVERRTP	= ( TS_TIMECODE + 1 ) 
    } 	ts_structure_enum_t;

typedef struct _ts_structure_t
    {
    ts_structure_enum_t type;
    int packet_len;
    } 	ts_structure_t;

typedef struct _network_name_desc_t
    {
    unsigned char network_name[ 255 ];
    } 	network_name_desc_t;

typedef struct _service_list_desc_t
    {
    u_short service_id;
    BYTE service_type;
    } 	service_list_desc_t;

typedef struct _cable_delivery_system_desc_t
    {
    UINT frequency;
    BYTE FEC_outer;
    BYTE modulation;
    UINT symbol_rate;
    BYTE FEC_inner;
    } 	cable_delivery_system_desc_t;

typedef struct _satellite_delivery_system_desc_t
    {
    UINT frequency;
    USHORT orbital_position;
    BYTE west_east_flag;
    BYTE polarization;
    BYTE rolloff;
    BYTE modulation_system;
    BYTE modulation_type;
    UINT symbol_rate;
    BYTE FEC_inner;
    } 	satellite_delivery_system_desc_t;

typedef struct _terrestial_delivery_system_desc_t
    {
    UINT centre_frequency;
    BYTE bandwidth;
    BYTE priority;
    BYTE time_scalling_indicator;
    BYTE MPE_FEC_indicator;
    BYTE constellation;
    BYTE hierarchy_information;
    BYTE code_rate_HP_stream;
    BYTE code_rate_LP_stream;
    BYTE guard_interval;
    BYTE transmission_mode;
    BYTE other_frequency_use;
    } 	terrestial_delivery_system_desc_t;




extern RPC_IF_HANDLE __MIDL_itf_ifc_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ifc_0000_0000_v0_0_s_ifspec;

#ifndef __IUdpMulticast_INTERFACE_DEFINED__
#define __IUdpMulticast_INTERFACE_DEFINED__

/* interface IUdpMulticast */
/* [uuid][object] */ 


EXTERN_C const IID IID_IUdpMulticast;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C7953E93-2C62-428D-A2F0-4F73F07994DC")
    IUdpMulticast : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMulticastIp( 
            /* [out] */ ULONG *plMip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMulticastIp( 
            /* [in] */ ULONG lMip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNicIp( 
            /* [out] */ ULONG *plNip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNicIp( 
            /* [in] */ ULONG lNip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNicIndex( 
            /* [in] */ ULONG lNicIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPort( 
            /* [out] */ u_short *psPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPort( 
            /* [in] */ u_short sPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioPid( 
            /* [out] */ u_short *piApid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioPid( 
            /* [in] */ u_short iApid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoPid( 
            /* [out] */ u_short *piVpid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoPid( 
            /* [in] */ u_short iVpid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubtitlesPid( 
            /* [out] */ u_short *piSpid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSubtitlesPid( 
            /* [in] */ u_short iSpid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRecFile( 
            /* [in] */ TCHAR *p_tcFile_name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSrcFile( 
            /* [in] */ TCHAR *p_tcFile_name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfigureDemultiplexer( 
            /* [in] */ u_short video_pid,
            /* [in] */ u_short audio_pid,
            /* [in] */ u_short subtitle_pid,
            /* [in] */ BYTE flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportRenderingSuccess( 
            /* [in] */ BYTE pin_code) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParsedData( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ interop_piddetails_t *p_det) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProgCount( 
            /* [out] */ int *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPidCount( 
            /* [in] */ int prog_index,
            /* [out] */ int *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubDesc( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [in] */ int sub_index,
            /* [out] */ subtitling_t *subdesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTtxDesc( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [in] */ int ttx_index,
            /* [out] */ teletext_ebu_t *ttxdesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoInfo( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ pesvideodet_t *vidinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioInfo( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ pesaudiodet_t *audinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPATPid( 
            /* [in] */ int prog_index,
            /* [out] */ u_short *prog_num,
            /* [out] */ u_short *pat_pid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNITPid( 
            /* [out] */ u_short *nit_pid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPMTType( 
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ u_short *pid,
            /* [out] */ int *type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMode( 
            /* [in] */ BYTE mode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ServicesCount( 
            /* [out] */ int *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceDescriptor( 
            /* [in] */ int index,
            /* [out] */ unsigned char *provider,
            /* [out] */ unsigned char *name,
            /* [out] */ u_short *prog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentSubtitleImage( 
            /* [out] */ unsigned char *pImage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendPTSToH264Dec( 
            /* [in] */ BOOL set) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IgnorePTSBeforeFirstIFrameForAVSync( 
            /* [in] */ BOOL set) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ContinueOnNextStop( 
            /* [in] */ BOOL set) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ LONGLONG *time) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetTiming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransportStreamInfo( 
            /* [out] */ ts_structure_t *ts_structure) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCutrrentTSBitRate( 
            /* [out] */ UINT *bitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitRateByPidTableCount( 
            /* [out] */ UINT *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitrateByPid( 
            /* [in] */ UINT index,
            /* [out] */ UINT *pid,
            /* [out] */ UINT *bitrate,
            /* [out] */ UINT *complete_bitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetNetworkNameDescriptor( 
            /* [in] */ int tsindex,
            /* [out] */ network_name_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetServiceListDescriptorCount( 
            /* [in] */ int tsindex,
            /* [out] */ int *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetServiceListDescriptor( 
            /* [in] */ int tsindex,
            /* [in] */ int index,
            /* [out] */ service_list_desc_t *descriptor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetCableDeliverySistemDescriptor( 
            /* [in] */ int tsindex,
            /* [out] */ cable_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetSatelliteDeliverySistemDescriptor( 
            /* [in] */ int tsindex,
            /* [out] */ satellite_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetTerrestialDeliverySistemDescriptor( 
            /* [in] */ int tsindex,
            /* [out] */ terrestial_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitTSDescriptorListCount( 
            /* [out] */ int *count) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NitGetTsDescriptorListIds( 
            /* [in] */ int index,
            /* [out] */ u_short *transport_stream_id,
            /* [out] */ u_short *original_network_id) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUdpMulticastVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUdpMulticast * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUdpMulticast * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUdpMulticast * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMulticastIp )( 
            IUdpMulticast * This,
            /* [out] */ ULONG *plMip);
        
        HRESULT ( STDMETHODCALLTYPE *SetMulticastIp )( 
            IUdpMulticast * This,
            /* [in] */ ULONG lMip);
        
        HRESULT ( STDMETHODCALLTYPE *GetNicIp )( 
            IUdpMulticast * This,
            /* [out] */ ULONG *plNip);
        
        HRESULT ( STDMETHODCALLTYPE *SetNicIp )( 
            IUdpMulticast * This,
            /* [in] */ ULONG lNip);
        
        HRESULT ( STDMETHODCALLTYPE *SetNicIndex )( 
            IUdpMulticast * This,
            /* [in] */ ULONG lNicIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetPort )( 
            IUdpMulticast * This,
            /* [out] */ u_short *psPort);
        
        HRESULT ( STDMETHODCALLTYPE *SetPort )( 
            IUdpMulticast * This,
            /* [in] */ u_short sPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioPid )( 
            IUdpMulticast * This,
            /* [out] */ u_short *piApid);
        
        HRESULT ( STDMETHODCALLTYPE *SetAudioPid )( 
            IUdpMulticast * This,
            /* [in] */ u_short iApid);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoPid )( 
            IUdpMulticast * This,
            /* [out] */ u_short *piVpid);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoPid )( 
            IUdpMulticast * This,
            /* [in] */ u_short iVpid);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubtitlesPid )( 
            IUdpMulticast * This,
            /* [out] */ u_short *piSpid);
        
        HRESULT ( STDMETHODCALLTYPE *SetSubtitlesPid )( 
            IUdpMulticast * This,
            /* [in] */ u_short iSpid);
        
        HRESULT ( STDMETHODCALLTYPE *SetRecFile )( 
            IUdpMulticast * This,
            /* [in] */ TCHAR *p_tcFile_name);
        
        HRESULT ( STDMETHODCALLTYPE *SetSrcFile )( 
            IUdpMulticast * This,
            /* [in] */ TCHAR *p_tcFile_name);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureDemultiplexer )( 
            IUdpMulticast * This,
            /* [in] */ u_short video_pid,
            /* [in] */ u_short audio_pid,
            /* [in] */ u_short subtitle_pid,
            /* [in] */ BYTE flags);
        
        HRESULT ( STDMETHODCALLTYPE *ReportRenderingSuccess )( 
            IUdpMulticast * This,
            /* [in] */ BYTE pin_code);
        
        HRESULT ( STDMETHODCALLTYPE *GetParsedData )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ interop_piddetails_t *p_det);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgCount )( 
            IUdpMulticast * This,
            /* [out] */ int *count);
        
        HRESULT ( STDMETHODCALLTYPE *GetPidCount )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [out] */ int *count);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubDesc )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [in] */ int sub_index,
            /* [out] */ subtitling_t *subdesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetTtxDesc )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [in] */ int ttx_index,
            /* [out] */ teletext_ebu_t *ttxdesc);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoInfo )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ pesvideodet_t *vidinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioInfo )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ pesaudiodet_t *audinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetPATPid )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [out] */ u_short *prog_num,
            /* [out] */ u_short *pat_pid);
        
        HRESULT ( STDMETHODCALLTYPE *GetNITPid )( 
            IUdpMulticast * This,
            /* [out] */ u_short *nit_pid);
        
        HRESULT ( STDMETHODCALLTYPE *GetPMTType )( 
            IUdpMulticast * This,
            /* [in] */ int prog_index,
            /* [in] */ int pid_index,
            /* [out] */ u_short *pid,
            /* [out] */ int *type);
        
        HRESULT ( STDMETHODCALLTYPE *SetMode )( 
            IUdpMulticast * This,
            /* [in] */ BYTE mode);
        
        HRESULT ( STDMETHODCALLTYPE *ServicesCount )( 
            IUdpMulticast * This,
            /* [out] */ int *count);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int index,
            /* [out] */ unsigned char *provider,
            /* [out] */ unsigned char *name,
            /* [out] */ u_short *prog);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentSubtitleImage )( 
            IUdpMulticast * This,
            /* [out] */ unsigned char *pImage);
        
        HRESULT ( STDMETHODCALLTYPE *SendPTSToH264Dec )( 
            IUdpMulticast * This,
            /* [in] */ BOOL set);
        
        HRESULT ( STDMETHODCALLTYPE *IgnorePTSBeforeFirstIFrameForAVSync )( 
            IUdpMulticast * This,
            /* [in] */ BOOL set);
        
        HRESULT ( STDMETHODCALLTYPE *ContinueOnNextStop )( 
            IUdpMulticast * This,
            /* [in] */ BOOL set);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IUdpMulticast * This,
            /* [out] */ LONGLONG *time);
        
        HRESULT ( STDMETHODCALLTYPE *ResetTiming )( 
            IUdpMulticast * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransportStreamInfo )( 
            IUdpMulticast * This,
            /* [out] */ ts_structure_t *ts_structure);
        
        HRESULT ( STDMETHODCALLTYPE *GetCutrrentTSBitRate )( 
            IUdpMulticast * This,
            /* [out] */ UINT *bitrate);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitRateByPidTableCount )( 
            IUdpMulticast * This,
            /* [out] */ UINT *count);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitrateByPid )( 
            IUdpMulticast * This,
            /* [in] */ UINT index,
            /* [out] */ UINT *pid,
            /* [out] */ UINT *bitrate,
            /* [out] */ UINT *complete_bitrate);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetNetworkNameDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [out] */ network_name_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetServiceListDescriptorCount )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [out] */ int *count);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetServiceListDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [in] */ int index,
            /* [out] */ service_list_desc_t *descriptor);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetCableDeliverySistemDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [out] */ cable_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetSatelliteDeliverySistemDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [out] */ satellite_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetTerrestialDeliverySistemDescriptor )( 
            IUdpMulticast * This,
            /* [in] */ int tsindex,
            /* [out] */ terrestial_delivery_system_desc_t *descriptor,
            /* [out] */ BOOL *has_descriptor);
        
        HRESULT ( STDMETHODCALLTYPE *NitTSDescriptorListCount )( 
            IUdpMulticast * This,
            /* [out] */ int *count);
        
        HRESULT ( STDMETHODCALLTYPE *NitGetTsDescriptorListIds )( 
            IUdpMulticast * This,
            /* [in] */ int index,
            /* [out] */ u_short *transport_stream_id,
            /* [out] */ u_short *original_network_id);
        
        END_INTERFACE
    } IUdpMulticastVtbl;

    interface IUdpMulticast
    {
        CONST_VTBL struct IUdpMulticastVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUdpMulticast_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IUdpMulticast_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IUdpMulticast_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IUdpMulticast_GetMulticastIp(This,plMip)	\
    ( (This)->lpVtbl -> GetMulticastIp(This,plMip) ) 

#define IUdpMulticast_SetMulticastIp(This,lMip)	\
    ( (This)->lpVtbl -> SetMulticastIp(This,lMip) ) 

#define IUdpMulticast_GetNicIp(This,plNip)	\
    ( (This)->lpVtbl -> GetNicIp(This,plNip) ) 

#define IUdpMulticast_SetNicIp(This,lNip)	\
    ( (This)->lpVtbl -> SetNicIp(This,lNip) ) 

#define IUdpMulticast_SetNicIndex(This,lNicIndex)	\
    ( (This)->lpVtbl -> SetNicIndex(This,lNicIndex) ) 

#define IUdpMulticast_GetPort(This,psPort)	\
    ( (This)->lpVtbl -> GetPort(This,psPort) ) 

#define IUdpMulticast_SetPort(This,sPort)	\
    ( (This)->lpVtbl -> SetPort(This,sPort) ) 

#define IUdpMulticast_GetAudioPid(This,piApid)	\
    ( (This)->lpVtbl -> GetAudioPid(This,piApid) ) 

#define IUdpMulticast_SetAudioPid(This,iApid)	\
    ( (This)->lpVtbl -> SetAudioPid(This,iApid) ) 

#define IUdpMulticast_GetVideoPid(This,piVpid)	\
    ( (This)->lpVtbl -> GetVideoPid(This,piVpid) ) 

#define IUdpMulticast_SetVideoPid(This,iVpid)	\
    ( (This)->lpVtbl -> SetVideoPid(This,iVpid) ) 

#define IUdpMulticast_GetSubtitlesPid(This,piSpid)	\
    ( (This)->lpVtbl -> GetSubtitlesPid(This,piSpid) ) 

#define IUdpMulticast_SetSubtitlesPid(This,iSpid)	\
    ( (This)->lpVtbl -> SetSubtitlesPid(This,iSpid) ) 

#define IUdpMulticast_SetRecFile(This,p_tcFile_name)	\
    ( (This)->lpVtbl -> SetRecFile(This,p_tcFile_name) ) 

#define IUdpMulticast_SetSrcFile(This,p_tcFile_name)	\
    ( (This)->lpVtbl -> SetSrcFile(This,p_tcFile_name) ) 

#define IUdpMulticast_ConfigureDemultiplexer(This,video_pid,audio_pid,subtitle_pid,flags)	\
    ( (This)->lpVtbl -> ConfigureDemultiplexer(This,video_pid,audio_pid,subtitle_pid,flags) ) 

#define IUdpMulticast_ReportRenderingSuccess(This,pin_code)	\
    ( (This)->lpVtbl -> ReportRenderingSuccess(This,pin_code) ) 

#define IUdpMulticast_GetParsedData(This,prog_index,pid_index,p_det)	\
    ( (This)->lpVtbl -> GetParsedData(This,prog_index,pid_index,p_det) ) 

#define IUdpMulticast_GetProgCount(This,count)	\
    ( (This)->lpVtbl -> GetProgCount(This,count) ) 

#define IUdpMulticast_GetPidCount(This,prog_index,count)	\
    ( (This)->lpVtbl -> GetPidCount(This,prog_index,count) ) 

#define IUdpMulticast_GetSubDesc(This,prog_index,pid_index,sub_index,subdesc)	\
    ( (This)->lpVtbl -> GetSubDesc(This,prog_index,pid_index,sub_index,subdesc) ) 

#define IUdpMulticast_GetTtxDesc(This,prog_index,pid_index,ttx_index,ttxdesc)	\
    ( (This)->lpVtbl -> GetTtxDesc(This,prog_index,pid_index,ttx_index,ttxdesc) ) 

#define IUdpMulticast_GetVideoInfo(This,prog_index,pid_index,vidinfo)	\
    ( (This)->lpVtbl -> GetVideoInfo(This,prog_index,pid_index,vidinfo) ) 

#define IUdpMulticast_GetAudioInfo(This,prog_index,pid_index,audinfo)	\
    ( (This)->lpVtbl -> GetAudioInfo(This,prog_index,pid_index,audinfo) ) 

#define IUdpMulticast_GetPATPid(This,prog_index,prog_num,pat_pid)	\
    ( (This)->lpVtbl -> GetPATPid(This,prog_index,prog_num,pat_pid) ) 

#define IUdpMulticast_GetNITPid(This,nit_pid)	\
    ( (This)->lpVtbl -> GetNITPid(This,nit_pid) ) 

#define IUdpMulticast_GetPMTType(This,prog_index,pid_index,pid,type)	\
    ( (This)->lpVtbl -> GetPMTType(This,prog_index,pid_index,pid,type) ) 

#define IUdpMulticast_SetMode(This,mode)	\
    ( (This)->lpVtbl -> SetMode(This,mode) ) 

#define IUdpMulticast_ServicesCount(This,count)	\
    ( (This)->lpVtbl -> ServicesCount(This,count) ) 

#define IUdpMulticast_GetServiceDescriptor(This,index,provider,name,prog)	\
    ( (This)->lpVtbl -> GetServiceDescriptor(This,index,provider,name,prog) ) 

#define IUdpMulticast_GetCurrentSubtitleImage(This,pImage)	\
    ( (This)->lpVtbl -> GetCurrentSubtitleImage(This,pImage) ) 

#define IUdpMulticast_SendPTSToH264Dec(This,set)	\
    ( (This)->lpVtbl -> SendPTSToH264Dec(This,set) ) 

#define IUdpMulticast_IgnorePTSBeforeFirstIFrameForAVSync(This,set)	\
    ( (This)->lpVtbl -> IgnorePTSBeforeFirstIFrameForAVSync(This,set) ) 

#define IUdpMulticast_ContinueOnNextStop(This,set)	\
    ( (This)->lpVtbl -> ContinueOnNextStop(This,set) ) 

#define IUdpMulticast_GetStreamTime(This,time)	\
    ( (This)->lpVtbl -> GetStreamTime(This,time) ) 

#define IUdpMulticast_ResetTiming(This)	\
    ( (This)->lpVtbl -> ResetTiming(This) ) 

#define IUdpMulticast_GetTransportStreamInfo(This,ts_structure)	\
    ( (This)->lpVtbl -> GetTransportStreamInfo(This,ts_structure) ) 

#define IUdpMulticast_GetCutrrentTSBitRate(This,bitrate)	\
    ( (This)->lpVtbl -> GetCutrrentTSBitRate(This,bitrate) ) 

#define IUdpMulticast_GetBitRateByPidTableCount(This,count)	\
    ( (This)->lpVtbl -> GetBitRateByPidTableCount(This,count) ) 

#define IUdpMulticast_GetBitrateByPid(This,index,pid,bitrate,complete_bitrate)	\
    ( (This)->lpVtbl -> GetBitrateByPid(This,index,pid,bitrate,complete_bitrate) ) 

#define IUdpMulticast_NitGetNetworkNameDescriptor(This,tsindex,descriptor,has_descriptor)	\
    ( (This)->lpVtbl -> NitGetNetworkNameDescriptor(This,tsindex,descriptor,has_descriptor) ) 

#define IUdpMulticast_NitGetServiceListDescriptorCount(This,tsindex,count)	\
    ( (This)->lpVtbl -> NitGetServiceListDescriptorCount(This,tsindex,count) ) 

#define IUdpMulticast_NitGetServiceListDescriptor(This,tsindex,index,descriptor)	\
    ( (This)->lpVtbl -> NitGetServiceListDescriptor(This,tsindex,index,descriptor) ) 

#define IUdpMulticast_NitGetCableDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor)	\
    ( (This)->lpVtbl -> NitGetCableDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor) ) 

#define IUdpMulticast_NitGetSatelliteDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor)	\
    ( (This)->lpVtbl -> NitGetSatelliteDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor) ) 

#define IUdpMulticast_NitGetTerrestialDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor)	\
    ( (This)->lpVtbl -> NitGetTerrestialDeliverySistemDescriptor(This,tsindex,descriptor,has_descriptor) ) 

#define IUdpMulticast_NitTSDescriptorListCount(This,count)	\
    ( (This)->lpVtbl -> NitTSDescriptorListCount(This,count) ) 

#define IUdpMulticast_NitGetTsDescriptorListIds(This,index,transport_stream_id,original_network_id)	\
    ( (This)->lpVtbl -> NitGetTsDescriptorListIds(This,index,transport_stream_id,original_network_id) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IUdpMulticast_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ifc_0000_0001 */
/* [local] */ 

#define DECLARE_IUDPMULTICAST();\
virtual STDMETHODIMP(GetMulticastIp)(ULONG*);\
virtual STDMETHODIMP(SetMulticastIp)(ULONG);\
virtual STDMETHODIMP(GetNicIp)(ULONG *);\
virtual STDMETHODIMP(SetNicIp)(ULONG);\
virtual STDMETHODIMP(GetPort)(u_short*);\
virtual STDMETHODIMP(SetPort)(u_short);\
virtual STDMETHODIMP(GetAudioPid)(u_short*);\
virtual STDMETHODIMP(SetAudioPid)(u_short);\
virtual STDMETHODIMP(GetVideoPid)(u_short*);\
virtual STDMETHODIMP(SetVideoPid)(u_short);\
virtual STDMETHODIMP(GetSubtitlesPid)(u_short*);\
virtual STDMETHODIMP(SetSubtitlesPid)(u_short);\
virtual STDMETHODIMP(SetRecFile)(TCHAR*);\
virtual STDMETHODIMP(SetSrcFile)(TCHAR*);\
virtual STDMETHODIMP(ConfigureDemultiplexer)(u_short,u_short,u_short,BYTE);\
virtual STDMETHODIMP(ReportRenderingSuccess)(BYTE);\
virtual STDMETHODIMP(GetParsedData)(int,int,interop_piddetails_t*);\
virtual STDMETHODIMP(GetProgCount)(int*);\
virtual STDMETHODIMP(GetPidCount)(int,int*);\
virtual STDMETHODIMP(GetSubDesc)(int,int,int,subtitling_t*);\
virtual STDMETHODIMP(GetTtxDesc)(int,int,int,teletext_ebu_t*);\
virtual STDMETHODIMP(GetVideoInfo)(int,int,pesvideodet_t*);\
virtual STDMETHODIMP(GetAudioInfo)(int,int,pesaudiodet_t*);\
virtual STDMETHODIMP(GetPATPid)(int,u_short*,u_short*);\
virtual STDMETHODIMP(GetNITPid)(u_short*);\
virtual STDMETHODIMP(GetPMTType)(int,int,u_short*,int*);\
virtual STDMETHODIMP(SetMode)(BYTE);\
virtual STDMETHODIMP(ServiceCount)(int*);\
virtual STDMETHODIMP(GetServiceDescriptor)(int,unsigned char*,unsigned char*,u_short*);\
virtual STDMETHODIMP(GetCurrentSubtitleImage)(unsigned char*);\
virtual STDMETHODIMP(SendPTSToH264Dec)(BOOL);\
virtual STDMETHODIMP(IgnorePTSBeforeFirstIFrameForAVSync)(BOOL);\
virtual STDMETHODIMP(ContinueOnNextStop)(BOOL);\
virtual STDMETHODIMP(GetStreamTime)(LONGLONG*);\
virtual STDMETHODIMP(ResetTiming)();\
virtual STDMETHODIMP(GetTransportStreamInfo)(ts_structure_t*);\
virtual STDMETHODIMP(GetCutrrentTSBitRate)(UINT*);\
virtual STDMETHODIMP(GetBitRateByPidTableCount)(UINT*);\
virtual STDMETHODIMP(GetBitrateByPid)(UINT,UINT*,UINT*,UINT*);\
virtual STDMETHODIMP(NitGetNetworkNameDescriptor)(int,network_name_desc_t*,BOOL*);\
virtual STDMETHODIMP(NitGetServiceListDescriptorCount)(int,int*);\
virtual STDMETHODIMP(NitGetServiceListDescriptor)(int,int,service_list_desc_t*);\
virtual STDMETHODIMP(NitGetCableDeliverySistemDescriptor)(int,cable_delivery_system_desc_t*, BOOL*);\
virtual STDMETHODIMP(NitGetSatelliteDeliverySistemDescriptor)(int,satellite_delivery_system_desc_t*,BOOL*);\
virtual STDMETHODIMP(NitGetTerrestialDeliverySistemDescriptor)(int, terrestial_delivery_system_desc_t*,BOOL*);\
virtual STDMETHODIMP(NitTSDescriptorListCount)(int*);\
virtual STDMETHODIMP(NitGetTsDescriptorListIds)(int,u_short*,u_short* original_network_id);\


extern RPC_IF_HANDLE __MIDL_itf_ifc_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ifc_0000_0001_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


