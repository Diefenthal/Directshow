#ifndef H_UDPFILTER
#define H_UDPFILTER

#include "proj.h"
#include <tchar.h>
#include "receiver.h"
#include <iostream>
#include <fstream>
#include <strsafe.h>
#include "udpguids.h"
#include "parser.h"
#include "dvbsubdecoder.h"

using namespace std;


#define g_wszUdpFilterName     L"Multicast udp source filter (MulticastTV)\0"
#define g_wszPropPageName      L"Net Receive Properties\0"

#define Dump1(tsz, arg)                         \
    { TCHAR dbgsup_tszDump[1024];               \
      HRESULT hrT = StringCchPrintf(dbgsup_tszDump, 1024, (tsz), (arg));   \
      OutputDebugString(dbgsup_tszDump); }

static const GUID MEDIASUBTYPE_AVC =
  		{ 0x31435641, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

#define RUNNING  0
#define STOPPED  1
#define PAUSED	 2

#define MAX_ES_LEN 512000
#define MAX_DVB_PES 64000
#define MAX_h264_FRAME_SIZE 160000

#define DVBDEC_SHOW_PREV 0x1
#define DVBDEC_SHOW_NEW 0x2
#define DVBDEC_CLEAR 0x3 
#define DVBDEC_MS_PERFRAME 20
#define DVBDEC_TIMEPERFRAME 2000000  //5fps
#define DVBDEC_SKIP 0x4


//#define TO_DS_TIME_FACTOR 1000/9  // dt = 300/f *dPTS ; f=27000000Hz

#define PTS_1SEC 90000
#define PTS_2SEC 2*PTS_1SEC
#define PTS_3SEC 3*PTS_1SEC
#define PTS_4SEC 4*PTS_1SEC
#define PTS_5SEC 5*PTS_1SEC
#define PTS_6SEC 6*PTS_1SEC
#define PTS_7SEC 7*PTS_1SEC
#define PTS_8SEC 8*PTS_1SEC
#define PTS_9SEC 9*PTS_1SEC
#define PTS_10SEC 10*PTS_1SEC  // We use this value for PTS jump detection. 
                                 // TODO: Find Key frame interval and report it to player !!!!!!!!!!!!!!!!!
#define DSPTS_1SEC 10000000
#define DSPTS_2SEC 2*DSPTS_1SEC
#define DSPTS_3SEC 3*DSPTS_1SEC
#define DSPTS_4SEC 4*DSPTS_1SEC
#define DSPTS_5SEC 5*DSPTS_1SEC
#define DSPTS_6SEC 6*DSPTS_1SEC
#define DSPTS_7SEC 7*DSPTS_1SEC
#define DSPTS_8SEC 8*DSPTS_1SEC
#define DSPTS_9SEC 9*DSPTS_1SEC
#define DSPTS_10SEC 10*DSPTS_1SEC 


// DELAYS 10000 = 1ms 
//Set constant delay to prevent samples comming to late to renderer
#define GLOBAL_VIDEO_DELAY 2000000 //200ms
#define GLOBAL_AUDIO_DELAY 2000000 //200ms   
#define GLOBAL_SUB_DELAY 2000000   //200ms

#define MAX_PTS 8589934591
#define DS_MAX_PTS MAX_PTS*1000/9
#define NUM_PTS_COLLECT 150							// How many PTS values to collect for AV resync

#define ALL_PINS_SUCCESS 0x2
#define VIDEO_PIN_SUCCESS 0x4
#define AUDIO_PIN_SUCCESS 0x8
#define SUB_PIN_SUCCESS 0x10
#define TTX_PIN_SUCCESS 0x20

#define DEMUX_FLAGS_POLL_SUBTITLES 0x1
#define DEMUX_FLAGS_AYUV_SUBTITLES 0x2
#define DEMUX_FLAGS_MEASURE_BITRATE 0x4
#define DEMUX_FLAGS_MEASURE_PID_BITRATE 0x8
#define DEMUX_FLAGS_MONITOR_SELECTED_PMT 0x10  // Monitor PMT for changes (selected program)

const int MAX_DEMUX_FIFO_SIZE_FOR_FILE_SOURCE = 240000/RECV_BUFFER; 
const int MAX_ES_AUDIOFIFO_SIZE_FOR_FILE_SOURCE = 4;
const int MIN_ES_AUDIOFIFO_SIZE_FOR_FILE_SOURCE = 3;
const int MAX_ES_VIDEOFIFO_SIZE_FOR_FILE_SOURCE = 4;
const int MIN_ES_VIDEOFIFO_SIZE_FOR_FILE_SOURCE = 3;

#define DEMUXER_FIRST_PUSH_BUFFER_COUNT  240000/RECV_BUFFER  // How many buffers to collect before demuxer loop starts. Collect for about 12 video frames at 4Mbit/s. TODO: dynamic

#define AUDIO_PID_CHANGE_NO_CHANGES_NEEDED 0
#define AUDIO_PID_CHANGE_FORMAT_CHANGE_NEEDED 1
#define AUDIO_PID_CHANGE_RECONNECTION_NEEDED 2

#define ACM_MPEG_LAYER1             (0x0001)
#define ACM_MPEG_LAYER2             (0x0002)
#define ACM_MPEG_LAYER3             (0x0004)

#define MAX_ES_FIFO_SIZE 15000  // PREVENT to accumulate to much data if for some reason flow of data is blocked somewhere.
                                // This effects timeshift to RAM ! TODO: Drop all packets if any of ES fifo reaches this value, 
                                // otherwise audio and video can't be synchronised.

#define AUDIO_BUFFERS 16
#define VIDEO_BUFFERS 16

struct timeperframe_t
{
	LONGLONG nominator;
	LONGLONG denominator;
};

//===========================================================
// PINS
//===========================================================

// Video output pin =========================================
class CPushPinUdpVideo : public CSourceStream//, public IAMPushSource
{
		private:

       CPushSourceUdp* m_pSrcFilter;

		bool m_delay_set;
		bool m_h264_fps_found;
		LONGLONG m_pts_loop_count;
		LONGLONG m_first_pts;
		LONGLONG m_first_dspts;
		LONGLONG m_prev_pts;
		LONGLONG m_prev_dspts;
		UINT m_time;
		LONGLONG m_discont_pts_diff;
		LONGLONG m_last_jump_pts;
		
		int m_prev_sample_field_count;
		UINT m_prev_buf_size;
		LONGLONG m_first_pts_array[NUM_PTS_COLLECT];	// Used to find out h.264 fps and AV resync if samples are not in presentation order.
		BYTE m_first_pts_array_count;
		LONGLONG m_field_count;
		bool m_h264_timeperhalfframe_estimated;
		bool m_discont_recovered;
		bool m_check_audio_pin_for_pts_jump;

		void estimate_h264_tpf(LONGLONG first_pts, LONGLONG second_pts);
		void getTPHF();								//Find time per half frame 
		LONGLONG getLPTS();							//Find lowest PTS code from the beginning of the stream - for audio/video sync.
		static void shellsort(LONGLONG a[],int n);

	public:
    
		CPushPinUdpVideo(HRESULT *phr, CSource *pFilter );
		~CPushPinUdpVideo();

		BOOL m_ignore_PTS_before_first_I_frame;

		HANDLE m_ExitFillBuffer;
		BYTE m_source; // Net or File

		UINT m_arx, m_ary, m_width, m_height;
		BYTE m_afd;
		timeperframe_t m_time_per_field;

		HRESULT GetMediaType (CMediaType*) ;
		HRESULT CheckMediaType (const CMediaType* );
		HRESULT DecideBufferSize(IMemAllocator*, ALLOCATOR_PROPERTIES*);
		HRESULT FillBuffer(IMediaSample*);

		void resetTiming();

		STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q) // Quality control
		{
			return E_FAIL;
		}

	 /*   DECLARE_IUNKNOWN;

		STDMETHODIMP GetPushSourceFlags(ULONG* pFlags)
		{
			(* pFlags) = 0 ;
			return S_OK;
		}
		STDMETHODIMP GetLatency(REFERENCE_TIME *ptrLatency)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetPushSourceFlags(ULONG Flags)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetStreamOffset(REFERENCE_TIME rtOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetStreamOffset(REFERENCE_TIME* ptrOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetMaxStreamOffset(REFERENCE_TIME* ptrMaxOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
		{
			if (riid == IID_IAMPushSource)
			{
				return GetInterface(
				   static_cast<IAMPushSource*>(this),
				   ppv);
			}
			else if (riid == IID_IAMLatency)
			{
				return GetInterface(static_cast<IAMLatency*>(this), ppv);
			}
			else return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
		}*/

};

//===========================================================

// Audio output pin =========================================
class CPushPinUdpAudio : public CSourceStream, IPinFlowControl//, IAMPushSource
{
	
	private:
		CPushSourceUdp* m_pSrcFilter;
		LONGLONG m_pts_loop_count;
		LONGLONG m_first_pts;
		LONGLONG m_first_dspts;
		LONGLONG m_prev_pts;
		LONGLONG m_prev_dspts;
		UINT m_time;
		LONGLONG m_delay;
		LONGLONG m_first_video_pts;
		BOOL m_was_PTS_jump_detected;
		BOOL m_video_pin_reported_jump;
		BOOL m_has_video_pin_ref_time;
		BOOL m_has_this_pin_ref_time;
		LONGLONG m_last_sample_length;
		LONGLONG m_last_jump_dspts;

	public:
		CPushPinUdpAudio(HRESULT *phr, CSource *pFilter );
		~CPushPinUdpAudio();

		bool m_rendered;
		bool m_video_has_data;

		HANDLE m_ExitFillBuffer;
		CRITICAL_SECTION m_CritSec;
		BYTE m_source;
		USHORT m_channels;
		UINT m_samp_freq;
		BOOL m_dyn_recon_in_progress;

		void SetVideoStartPts(LONGLONG pts);
		BOOL wasPTSJumpDetected(LONGLONG* ref_pts, LONGLONG* ref_ds, LONGLONG vid_pts);
		BOOL hasRefTime();
		void reportPTSJumpinVideoPin();

		HRESULT GetMediaType (CMediaType*) ;
		HRESULT CheckMediaType (const CMediaType* );
		HRESULT DecideBufferSize(IMemAllocator*, ALLOCATOR_PROPERTIES*);
		HRESULT FillBuffer(IMediaSample*);

		void resetTiming();
   
		// Quality control
		STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
		{
			return E_FAIL;
		}

		DECLARE_IUNKNOWN;

		// Only synchronous.
		STDMETHODIMP Block(/* [in] */ DWORD dwBlockFlags, 
                           /* [in] */ HANDLE hEvent);
		

		/*STDMETHODIMP GetPushSourceFlags(ULONG* pFlags)
		{
			(* pFlags) = 0 ;
			return S_OK;
		}
		STDMETHODIMP GetLatency(REFERENCE_TIME *ptrLatency)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetPushSourceFlags(ULONG Flags)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetStreamOffset(REFERENCE_TIME rtOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetStreamOffset(REFERENCE_TIME* ptrOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetMaxStreamOffset(REFERENCE_TIME* ptrMaxOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
		{
			return E_NOTIMPL;
		}
		*/
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
		{
			//if (riid == IID_IAMPushSource)
			//{
			//	return GetInterface(
			//	   static_cast<IAMPushSource*>(this),
			//	   ppv);
			//}
			//else if (riid == IID_IAMLatency)
			//{
			//	return GetInterface(static_cast<IAMLatency*>(this), ppv);
			//}
			if (riid == IID_IPinFlowControl)
			{
			    return GetInterface(static_cast<IPinFlowControl*>(this),ppv);
			}
			else return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
		}
};


// Subtitles output pin =========================================
class CPushPinUdpSubs : public CSourceStream//, IAMPushSource
{
	 private:
		
		 AM_MEDIA_TYPE m_MediaType;
		 _32BIT_PIXEL* m_empty_img;

		CPushSourceUdp* m_pSrcFilter;
		LONGLONG m_pts_loop_count;
		LONGLONG m_prev_pts;
		LONGLONG m_prev_dspts;
		LONGLONG m_last_sample_time;
		LONGLONG m_loop_start_time;
		LONGLONG m_ds_ref_time;
		LONGLONG m_ds_approx_wrap_time;
		LONGLONG m_ds_current_time;
		DWORD m_time_out;
		bool m_last_page_valid;
		bool m_graph_start_time_found;
		bool m_first_call;

	
	public:
		CPushPinUdpSubs(HRESULT *phr, CSource *pFilter );
		~CPushPinUdpSubs();

		CRITICAL_SECTION m_CritSec;
 
		CDvbSub* m_dvb_decoder;

		LONGLONG GetStartPts(LONGLONG* ref_time);
		void SetStartPts(LONGLONG pts, LONGLONG dstime);		
		LONGLONG GetPtsCounter();
		void SetPtsWrap(LONGLONG factor, LONGLONG approx_time);

		LONGLONG m_first_pts;
		LONGLONG m_first_dspts;

		HANDLE m_ExitFillBuffer;
		BYTE m_source;
		u_short m_page_id;
		u_short m_ancillary_id;

		HRESULT GetMediaType (CMediaType*) ;
		void SetMediaTypeParams(BYTE colorspace, DWORD width, DWORD height, RECT src, RECT trg,UINT arx, UINT ary);
		HRESULT CheckMediaType (const CMediaType* );
		HRESULT DecideBufferSize(IMemAllocator*, ALLOCATOR_PROPERTIES*);
		HRESULT FillBuffer(IMediaSample*);
		//HRESULT GetBuffer(  IMediaSample **ppBuffer,  REFERENCE_TIME *pStartTime,  REFERENCE_TIME *pEndTime, DWORD dwFlags);
		//HRESULT DoBufferProcessingLoop(void);
   
		// Quality control
		STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
		{
			return E_FAIL;
		}

		/*DECLARE_IUNKNOWN;

		STDMETHODIMP GetPushSourceFlags(ULONG* pFlags)
		{
			(* pFlags) = 0 ;
			return S_OK;
		}
		STDMETHODIMP GetLatency(REFERENCE_TIME *ptrLatency)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetPushSourceFlags(ULONG Flags)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetStreamOffset(REFERENCE_TIME rtOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetStreamOffset(REFERENCE_TIME* ptrOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP GetMaxStreamOffset(REFERENCE_TIME* ptrMaxOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
		{
			return E_NOTIMPL;
		}
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
		{
			if (riid == IID_IAMPushSource)
			{
				return GetInterface(
				   static_cast<IAMPushSource*>(this),
				   ppv);
			}
			else if (riid == IID_IAMLatency)
			{
				return GetInterface(static_cast<IAMLatency*>(this), ppv);
			}
			else return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
		}*/
};



// ================================================================
//FILTER
// ================================================================
class CPushSourceUdp : public CSource,  public IUdpMulticast, public ISpecifyPropertyPages//, public IAMFilterMiscFlags//, public IReferenceClock
{

private:

    // Constructor is private because we have to use CreateInstance
    CPushSourceUdp(IUnknown*, HRESULT*);
    ~CPushSourceUdp();

	struct payload_t
	{
		int len;
		u_short indicated_len;
		LONGLONG present_time;
		bool has_pts;
		u_short last_continuity_count;
		bool discontinuity;
		u_short stream_type;
		char buffer[MAX_ES_LEN];				

	};

//======================================
	struct pidbitrate_t
	{
		int PID;
		ULONG bitratetimer;
		UINT packetcounter;
		UINT tsbitrate;
	};

	struct find_pid : std::unary_function<pidbitrate_t, bool> {
		DWORD pid;
		find_pid(int pid):pid(pid) { }
		bool operator()(pidbitrate_t const& p) const {
			return p.PID == pid;
		}
	};

//============================================

	payload_t m_completeVideoES;					// Video ES packet builder
	payload_t m_completeAudioES;					// Audio ES packet builder
	payload_t m_completeSubES;					// Subtitles ES packet builder


	mpeg2_ext_info_t m_mpeg2_ext_info;
	h264_ext_info_t m_h264_ext_info;

	ULONG m_Mip, m_Nip;
	u_short m_Port;
	u_short m_Apid, m_Vpid, m_Spid, m_PCRpid, m_VpidType, m_ApidType, m_PMT_PID; // SHOULD be in the struct !!!
	bool m_resetAudES, m_resetSubES, m_resetVidES;
	bool m_reciver_started;
	BYTE m_dvb_sub_status;
	bool m_audio_pid_format_change;
	
	ULONG m_bitratetimer;
	int m_packetcounter;
	UINT m_tsbitrate;

	vector<pidbitrate_t> m_PidSet;

	ULONG m_nic_index;

	void justParse();
	void ParseAndPush();
	void Demux();
	void processPacket(char* buf, int pos, bool*, bool*,bool*); //scarry function
	void DVBSubDecodeThread();
	BYTE DVBSubDecode(bufferES_t* bt, u_short page_id, u_short ancillary_id, BYTE* time_out);
	void extractPayload(char* buffer, int ts_start, payload_t*, CFifoES* esfifo , bool*);
	void pushFullESPacket(payload_t*,CFifoES*);

	HRESULT setPrimaryAudioPinMediaType(piddetails_t* audio_det, u_short audio_pid);

	// threads:
	HANDLE m_ParseThread;
	HANDLE m_ParseDemuxThread;
	HANDLE m_SubtitleDecoderThread;				// VMR-9's stream mixer has in some cases trouble mixing rgba32 video. That is why application can get directly decoded image from filter and use bitmap mixer instead.

	// signals:
	HANDLE m_ExitParseThreadEvent;				// Used for Parse&Push thread termination
	HANDLE m_ExitDemuxThreadEvent;				// Used for Demux thread termination
	HANDLE m_DemuxerConfiguredEvent;			// Signals Demux thread that Apllication configured demuxer
	HANDLE m_DemuxFifoDataEvent;				// Used to notify Demux thread that data is available in demux fifo.
	HANDLE m_DemuxFillingEnded;					// Used only in PARSING MODE - ensures that parsing ended before filter is stopped


	static DWORD WINAPI CPushSourceUdp::ParseThreadStaticEntryPoint(void * pThis) 
    {
     	CPushSourceUdp * pthX = (CPushSourceUdp*)pThis;
        pthX->ParseAndPush();   
        return EXIT_SUCCESS;  
    }

	static DWORD WINAPI CPushSourceUdp::ParseAndDemuxThreadStaticEntryPoint(void * pThis)
    {
     	CPushSourceUdp * pthX = (CPushSourceUdp*)pThis;  
        pthX->Demux();    
        return EXIT_SUCCESS;     
    }

		static DWORD WINAPI CPushSourceUdp::DVBDecodingThreadStaticEntryPoint(void * pThis) 
    {
     	CPushSourceUdp * pthX = (CPushSourceUdp*)pThis;
        pthX->DVBSubDecodeThread();   
        return EXIT_SUCCESS;  
    }

public:

    DECLARE_IUNKNOWN;

	TCHAR m_rec_file[MAX_FILENAME_LEN];
	TCHAR m_src_file[MAX_FILENAME_LEN];

	BYTE m_mode;
	BYTE m_source;
	UINT m_flags;

	BOOL m_dont_leave_multicast;
	BOOL m_pause_second_time;
	bool m_stream_has_video;
	bool m_stream_has_audio;
	bool m_stream_has_subtitles;

	// Application MUST notify filter if a particular pin is sucessfully rendered ! 
	bool m_video_pin_rendered;
	bool m_audio_pin_rendered;
	bool m_sub_pin_rendered;
	bool m_subtitle_polling;

	BOOL m_PTStoDec;

	LONGLONG m_first_vid_pts;
	LONGLONG m_first_aud_pts;
	LONGLONG m_first_sub_pts;

	HANDLE m_PushLimitEvent;

	CPushPinUdpSubs *m_pPinSubtitles;			//This pin is public, because subtitles must be synced from video pin.
    CPushPinUdpAudio *m_pPinAudio;              //This pin is public, because subtitles must be synced from video pin.
	CPushPinUdpVideo *m_pPinVideo;              //
    
	CCritSec m_pLock;
    CCritSec m_pLockSub;

	CReceiver* m_pReceiver;
	CParser* m_pParser;

	CFifo* m_pDemuxFifo;
	CFifoES* m_pVideoFifo;
	CFifoES* m_pAudioFifo;
	CFifoES* m_pSubsFifo;

	AM_MEDIA_TYPE m_MediaTypeVideo;
	AM_MEDIA_TYPE m_MediaTypeAudio;

	void Resync(LONGLONG video_i_frimePTS);

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
    STDMETHODIMP Run(REFERENCE_TIME);
    CBasePin * GetPin (IN  int ) ;

	HRESULT StreamTime(CRefTime& rtStream);

    int GetPinCount () { return 3 ; }

	STDMETHODIMP FindPin(LPCWSTR Id, IPin ** ppPin) // Default CSource FindPin doesn't work as expected
    {
    CheckPointer(ppPin,E_POINTER);
    ValidateReadWritePtr(ppPin,sizeof(IPin *));
    //  We're going to search the pin list so maintain integrity
    CAutoLock lck(&m_pLock);
    int iCount = GetPinCount();
    for (int i = 0; i < iCount; i++) {
        CBasePin *pPin = GetPin(i);
        ASSERT(pPin != NULL);

        if (0 == lstrcmpW(pPin->Name(), Id)) {
            //  Found one that matches
            //
            //  AddRef() and return it
            *ppPin = pPin;
            pPin->AddRef();
            return S_OK;
        }
    }
    *ppPin = NULL;
    return VFW_E_NOT_FOUND;
    }

    static CUnknown * WINAPI CreateInstance(IUnknown *, HRESULT *);  


//===========================================================
// Basic filter settings
    STDMETHODIMP CPushSourceUdp::GetMulticastIp(ULONG *plMip)
	{
		CAutoLock cAutoLock(&m_pLock);
		*plMip = CPushSourceUdp::m_Mip;
		return S_OK;
	}

    STDMETHODIMP CPushSourceUdp::SetMulticastIp(ULONG lMip)
	{
		CAutoLock cAutoLock(&m_pLock);
		CPushSourceUdp::m_Mip=lMip;
		OutputDebugString(TEXT("Multicast ip address set. \n"));
        return S_OK;
	}
    
	STDMETHODIMP CPushSourceUdp::GetNicIp(ULONG *plNip)
	{
		CAutoLock cAutoLock(&m_pLock);
        *plNip=CPushSourceUdp::m_Nip;
        return S_OK;
	}

    STDMETHODIMP CPushSourceUdp::SetNicIp(ULONG lNip)
	{
		CAutoLock cAutoLock(&m_pLock);
		CPushSourceUdp::m_Nip=lNip;
		OutputDebugString(TEXT("NIC ip address set. \n"));
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::SetNicIndex(ULONG index)
	{
		CAutoLock cAutoLock(&m_pLock);
		CPushSourceUdp::m_nic_index=index;
		OutputDebugString(TEXT("NIC index address set. \n"));
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetPort(u_short *psPort)
	{
		CAutoLock cAutoLock(&m_pLock);
		*psPort = CPushSourceUdp::m_Port;
		return S_OK;
	}

    STDMETHODIMP CPushSourceUdp::SetPort(u_short sPort)
	{
		CAutoLock cAutoLock(&m_pLock);
		CPushSourceUdp::m_Port = sPort;
		OutputDebugString(TEXT("Port set. \n"));
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetAudioPid(u_short *piApid)
	{
		CAutoLock cAutoLock(&m_pLock);
		*piApid = CPushSourceUdp::m_Apid;
		return S_OK;
	}
    STDMETHODIMP CPushSourceUdp::SetAudioPid(u_short iApid);


	STDMETHODIMP CPushSourceUdp::GetVideoPid(u_short *piVpid)
	{
		CAutoLock cAutoLock(&m_pLock);
		*piVpid = CPushSourceUdp::m_Vpid;
		return S_OK;
	}
    STDMETHODIMP CPushSourceUdp::SetVideoPid(u_short iVpid);


	STDMETHODIMP CPushSourceUdp::GetSubtitlesPid(u_short *piSpid)
	{
		CAutoLock cAutoLock(&m_pLock);
		*piSpid = CPushSourceUdp::m_Spid;
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::SetSubtitlesPid(u_short iSpid);

	
	STDMETHODIMP CPushSourceUdp::SetRecFile(TCHAR* file_name)
	{
		CAutoLock cAutoLock(&m_pLock);
		return _tcscpy_s(m_rec_file,_tcsclen(file_name),file_name);
		
	}

	STDMETHODIMP CPushSourceUdp::SetSrcFile(TCHAR* file_name)
	{
		HRESULT hr=E_FAIL;
		CAutoLock cAutoLock(&m_pLock);
		if (_tcsclen(file_name)>0)
		{
			hr = _tcscpy_s(m_src_file,_countof(m_src_file),file_name);
		}
			m_source = SOURCE_FILE;
		return hr;
	}

	STDMETHODIMP ConfigureDemultiplexer(u_short video_pid, u_short audio_pid, u_short subtitle_pid, BYTE flags);

	// OBSOLETE:.
	STDMETHODIMP ReportRenderingSuccess(BYTE code)
	{
		CAutoLock cAutoLock(&m_pLock);
		
		if (code == ALL_PINS_SUCCESS)
		{
			m_video_pin_rendered=true;
			m_audio_pin_rendered=true;
			m_sub_pin_rendered=true;
		}
		else
		{
			if ( code & VIDEO_PIN_SUCCESS)
				m_video_pin_rendered =true;
			if (code & AUDIO_PIN_SUCCESS)
				m_audio_pin_rendered =true;
			if (code & SUB_PIN_SUCCESS)
				m_sub_pin_rendered=true;
		}

		if (m_audio_pin_rendered==true)
			m_pPinAudio->m_rendered=true;

		if (m_video_pin_rendered && m_stream_has_video)
			m_pPinAudio->m_video_has_data=true;

		SetEvent(m_DemuxerConfiguredEvent); // Signal to demux thread that demuxer can start
		return S_OK;
	}

	//STDMETHODIMP ConfigureParserManualVideo(AM_MEDIA_TYPE* mt);
	//STDMETHODIMP ConfigureParserManualAudio(AM_MEDIA_TYPE* mt);

//===========================================================
//Parsing results
	STDMETHODIMP CPushSourceUdp::GetParsedData(int prog_index,int pid_index ,interop_piddetails_t* p_det)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->parsed_data(prog_index,pid_index,p_det);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetProgCount(int* count)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->programesCount(count);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetPidCount(int prog_index, int* count)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->pidCount(prog_index,count);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetSubDesc(int prog_index,int pid_index, int sub_index, subtitling_t* subdesc)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->get_sub_desc(prog_index,pid_index,sub_index,subdesc);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetTtxDesc(int prog_index,int pid_index, int ttx_index, teletext_ebu_t* ttxdesc)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->get_ttx_desc(prog_index, pid_index, ttx_index, ttxdesc);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetVideoInfo(int prog_index, int pid_index, pesvideodet_t* vidinfo)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->get_video_det(prog_index,pid_index,vidinfo);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetAudioInfo(int prog_index, int pid_index, pesaudiodet_t* audinfo)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pParser->get_audio_det(prog_index,pid_index,audinfo);
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetPATPid(int prog_index, u_short* prog_num , u_short* pat_pid);
	
	STDMETHODIMP CPushSourceUdp::GetNITPid(u_short* nit_pid);

	STDMETHODIMP CPushSourceUdp::GetPMTType(int prog_index, int pid_index, u_short* pid, int* type);

	STDMETHODIMP CPushSourceUdp::SetMode(BYTE mode)
	{
		if (mode == PARSING_MODE)
			m_mode = PARSING_MODE; // else default playback mode
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::ServicesCount(int* count)
	{
		*count = m_pParser->services_count();
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetServiceDescriptor(int index,  unsigned char* provider, unsigned char* name, u_short* prog)
	{
		m_pParser->get_service_descriptor(index,provider,name,prog);
		return S_OK;
	}

// ==========================================================
// Subtitle polling 
// Aplication must poll fast enaugh. If it is to slow, subtitles may have a delay or may be missed.
	STDMETHODIMP CPushSourceUdp::GetCurrentSubtitleImage(unsigned char* pImage)
	{
		BYTE dvb_sub_status = m_dvb_sub_status;
		{
		    CAutoLock cAutoLock(&m_pLockSub);
			dvb_sub_status = m_dvb_sub_status;		
		}

		if (m_dvb_sub_status == DVBDEC_SHOW_NEW)
		{
			m_pPinSubtitles->m_dvb_decoder->GetCurrentPageClone((_32BIT_PIXEL*)pImage);
			CAutoLock cAutoLock(&m_pLockSub);		
			m_dvb_sub_status = DVBDEC_SHOW_PREV;	//reset
		}

		return dvb_sub_status;
		//DVBDEC_SHOW_PREV
		//DVBDEC_SHOW_NEW
		//DVBDEC_CLEAR
	}

// ==========================================================

// other ====================================================

	STDMETHODIMP CPushSourceUdp::SendPTSToH264Dec(BOOL set)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_PTStoDec=set;
		return S_OK;
	}

	// Must be set before graph is running.
	STDMETHODIMP CPushSourceUdp::IgnorePTSBeforeFirstIFrameForAVSync(BOOL set)
	{
		CAutoLock cAutoLock(&m_pLock);
		m_pPinVideo->m_ignore_PTS_before_first_I_frame = set;
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::ContinueOnNextStop(BOOL set)
	{
	    CAutoLock cAutoLock(&m_pLock);
		m_dont_leave_multicast=set;
	    return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetStreamTime(LONGLONG *time)
	{
		CAutoLock cAutoLock(&m_pLock);
		CRefTime tm;
	    StreamTime(tm);
		*time = tm.m_time;
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::ResetTiming()
	{
        CAutoLock cAutoLock(&m_pLock);
		m_pPinAudio->resetTiming();		
		m_pPinVideo->resetTiming();		
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetTransportStreamInfo(ts_structure_t* ts_structure)
	{
        //CAutoLock cAutoLock(&m_pLock);
		ts_structure->type= m_pParser->m_ts_structure.type;
		ts_structure->packet_len = m_pParser->m_ts_structure.packet_len;
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetCutrrentTSBitRate(UINT * bitrate)
	{
	    //CAutoLock cAutoLock(&m_pLock);
		*bitrate =m_tsbitrate;
		return S_OK;
	}

	STDMETHODIMP CPushSourceUdp::GetBitRateByPidTableCount(UINT* count)
	{
	    CAutoLock cAutoLock(&m_pLock);	
		*count = m_PidSet.size();
		return S_OK;
	}

	STDMETHODIMP GetBitrateByPid(UINT index, UINT* pid, UINT* bitrate, UINT* complete_bitrate)
	{
	    CAutoLock cAutoLock(&m_pLock);
		*pid = m_PidSet[index].PID;
		*bitrate = m_PidSet[index].tsbitrate;
		*complete_bitrate = m_tsbitrate;
		return S_OK;
	}

	///========

	STDMETHODIMP NitGetNetworkNameDescriptor(int tsindex, network_name_desc_t* descriptor, BOOL* has_descriptor)
	{
	   CAutoLock cAutoLock(&m_pLock);
	   
	   if (tsindex==-1)
	   {
		   if (m_pParser->m_nit_net_descs.has_network_name_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_net_descs.network_name_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;
	   }
	   else
	   {
	   		if (m_pParser->m_nit_ts_descs[tsindex].has_network_name_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_ts_descs[tsindex].network_name_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;
	   }

	   return S_OK;
	}

	STDMETHODIMP NitGetServiceListDescriptorCount(int tsindex,int* count)
	{
	   CAutoLock cAutoLock(&m_pLock);	   
	   
	   if (tsindex==-1)
	   {
	      * count = m_pParser->m_nit_net_descs.service_list_descriptors.size();
	   }
	   else
	   {
	   	  * count = m_pParser->m_nit_ts_descs[tsindex].service_list_descriptors.size();
	   }
		
	   return S_OK;
	}

	STDMETHODIMP NitGetServiceListDescriptor(int tsindex,int index, service_list_desc_t* descriptor)
	{
	   CAutoLock cAutoLock(&m_pLock);
	   
	   if (tsindex==-1)
	   {
	      *descriptor = m_pParser->m_nit_net_descs.service_list_descriptors[index];
	   }
	   else
	   {
	      *descriptor = m_pParser->m_nit_ts_descs[tsindex].service_list_descriptors[index];	   
	   }
	  
	   return S_OK;
	}

	STDMETHODIMP NitGetCableDeliverySistemDescriptor(int tsindex,cable_delivery_system_desc_t* descriptor, BOOL* has_descriptor)
	{
	   CAutoLock cAutoLock(&m_pLock);

	   if (tsindex==-1)
	   {
		   if (m_pParser->m_nit_net_descs.has_cable_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_net_descs.cable_delivery_system_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;
	   }
	   else
	   {
		   if (m_pParser->m_nit_ts_descs[tsindex].has_cable_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_ts_descs[tsindex].cable_delivery_system_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;	   
	   }

	   return S_OK;
	}

	STDMETHODIMP NitGetSatelliteDeliverySistemDescriptor(int tsindex,satellite_delivery_system_desc_t* descriptor, BOOL* has_descriptor)
	{
	   CAutoLock cAutoLock(&m_pLock);	
	   
	   if (tsindex==-1)
	   {
		   if (m_pParser->m_nit_net_descs.has_satellite_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_net_descs.satellite_delivery_system_descriptor;
			   *has_descriptor=true;	
		   }
		   else
			   *has_descriptor=false;
	   }
	   else
	   {
		   if (m_pParser->m_nit_ts_descs[tsindex].has_satellite_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_ts_descs[tsindex].satellite_delivery_system_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;	   
	   }

	   return S_OK;
	}

	STDMETHODIMP NitGetTerrestialDeliverySistemDescriptor(int tsindex,terrestial_delivery_system_desc_t* descriptor, BOOL* has_descriptor)
	{
	   CAutoLock cAutoLock(&m_pLock);	
	   
	   if (tsindex==-1)
	   {
		   if (m_pParser->m_nit_net_descs.has_terrestial_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_net_descs.terrestial_delivery_system_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;
	   }
	   else
	   {
		   if (m_pParser->m_nit_ts_descs[tsindex].has_terrestial_delivery_system_descriptor)
		   {
			   *descriptor = m_pParser->m_nit_ts_descs[tsindex].terrestial_delivery_system_descriptor;
			   *has_descriptor=true;
		   }
		   else
			   *has_descriptor=false;	   
	   }

	   return S_OK;
	}

	STDMETHODIMP NitTSDescriptorListCount(int* count)
	{	
		CAutoLock cAutoLock(&m_pLock);	
		*count = m_pParser->m_nit_ts_descs.size();
	    return S_OK;
	}

	STDMETHODIMP NitGetTsDescriptorListIds(int index, u_short* transport_stream_id, u_short* original_network_id )
	{
		CAutoLock cAutoLock(&m_pLock);
		if (index>=0)
		{
		   * transport_stream_id = m_pParser->m_nit_ts_descs[index].transport_stream_id;
		   * original_network_id = m_pParser->m_nit_ts_descs[index].original_network_id;
		}
		else
			* transport_stream_id = m_pParser->m_nit_net_descs.network_id;
	   
		return S_OK;
	}

// ==========================================================

//===========================================================
// Tell graph manager that this filter has Reference clock
//ULONG STDMETHODCALLTYPE CPushSourceUdp::GetMiscFlags()
//{
//	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
//}


//===========================================================
// Property pages:
    STDMETHODIMP CPushSourceUdp::GetPages(CAUUID *pPages)
    {
        if (pPages == NULL) return E_POINTER;
        pPages->cElems = 1;
        pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
        if (pPages->pElems == NULL) 
        {
            return E_OUTOFMEMORY;
        }
        pPages->pElems[0] = CLSID_UdpSourceFilterPropPage;
        return S_OK;
    }

//*************************************************************

//***************************************************************

	STDMETHODIMP CPushSourceUdp::NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_ISpecifyPropertyPages)
		{
			return GetInterface(
			   static_cast<ISpecifyPropertyPages*>(this),
			   ppv);
		}
		else if (riid == IID_IUdpMulticast)
		{
			return GetInterface(static_cast<IUdpMulticast*>(this), ppv);
		}

		//else if (riid == __uuidof(IReferenceClock))
		//{
		//	return GetInterface((IReferenceClock *) this, ppv);
		//}

		//else if (riid == __uuidof(IAMFilterMiscFlags))
		//{
		//	return GetInterface((IAMFilterMiscFlags *) this, ppv);
		//}
		else
		{
			// Call the parent class.
			return CSource::NonDelegatingQueryInterface(riid, ppv);
		}
	}
};

#endif