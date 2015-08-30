#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "udpfilter.h"
#include <Dvdmedia.h>
#include "Mmreg.h"
#include "mathfunc.h"


// Video Output Pin =================================================

CPushPinUdpVideo::CPushPinUdpVideo(HRESULT *phr, CSource *pFilter ):
             CSourceStream(NAME("Udp Push Source Video"),phr, pFilter, L"Video")
{
	m_pSrcFilter=(CPushSourceUdp*)pFilter; // backpointer
	m_ExitFillBuffer = CreateEvent (NULL, TRUE, FALSE, NULL) ; //  non signaled
	m_source=SOURCE_NET;
	
	m_first_pts=-1;
	m_first_dspts=-1;
	m_prev_pts=0;
	m_prev_dspts=0;
	m_prev_sample_field_count=0;
	m_prev_buf_size=0;
	m_pts_loop_count=0;
	m_time=0;
	m_discont_pts_diff=0;
	m_arx=1;
	m_ary=1;
	m_width=0;
	m_height=0;
	m_afd=0;
	m_delay_set=false;
	m_h264_fps_found=false;
	m_first_pts_array_count=0;
	m_field_count=0;
	m_h264_timeperhalfframe_estimated=false;
	m_time_per_field.nominator=200000;
	m_time_per_field.denominator=1;
	m_discont_recovered=true;
	m_check_audio_pin_for_pts_jump=false;
	m_ignore_PTS_before_first_I_frame = TRUE;
	m_last_jump_pts=0;
}

CPushPinUdpVideo::~CPushPinUdpVideo()
{
	Stop();
	CloseHandle(m_ExitFillBuffer);
}

HRESULT CPushPinUdpVideo::GetMediaType(CMediaType *pmt)
{
 	CAutoLock cAutoLock(m_pFilter->pStateLock());
	
    CheckPointer(pmt, E_POINTER);
 
	pmt->Set(m_pSrcFilter->m_MediaTypeVideo );
  
    return S_OK;
}


HRESULT CPushPinUdpVideo::CheckMediaType (
    IN  const CMediaType * pMediaType
    )
{
    HRESULT hr ;

    if (pMediaType -> majortype  == MEDIATYPE_Video &&
        (pMediaType -> subtype  == MEDIASUBTYPE_H264 ||
		 pMediaType -> subtype  == MEDIASUBTYPE_AVC  || 
		 pMediaType -> subtype  == MEDIASUBTYPE_MPEG2_VIDEO ||
		 pMediaType -> subtype  == MEDIASUBTYPE_MPEG1Video 
		))
	{
		OutputDebugString(TEXT("Checking Media Type for video: OK\n"));
        hr = S_OK ;
    }
    else {
        OutputDebugString(TEXT("Checking Media Type for video: Failed\n"));
        hr = S_FALSE ;
    }

    return hr ;
}


HRESULT CPushPinUdpVideo::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProposed)
{
	HRESULT hr;

    OutputDebugString(TEXT("Decide buffer size for Video Output:\n"));

    CAutoLock cAutoLock(m_pFilter->pStateLock()); // Lock parent of this stream (streaming thread lock)

    CheckPointer(pAlloc, E_POINTER);
    CheckPointer(pProposed, E_POINTER);

    if (pProposed->cBuffers < VIDEO_BUFFERS)			// Ensure a minimum number of buffers
    {
        pProposed->cBuffers = VIDEO_BUFFERS;
    }

    pProposed->cbBuffer = MAX_ES_LEN;		// This is probably way to much. Actual buffer size is set in fill buffer function.

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProposed, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}

	Dump1(TEXT("Actual buffer size %d \n"),Actual.cbBuffer);

	if (Actual.cbBuffer < pProposed->cbBuffer) 
	{
		return E_FAIL;
	}

    OutputDebugString(TEXT("Video buffer size set."));
	return S_OK;

}

// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinUdpVideo::FillBuffer(IMediaSample * pSample)
{
    long cbData;
	bool bEnaugh=FALSE;
	BYTE* pData=NULL;

	LONGLONG diff=0;
	LONGLONG resync_delay=0;
    REFERENCE_TIME start=0;  
	
	CheckPointer(pSample, E_POINTER);

	while (m_pSrcFilter->m_pVideoFifo->items()==0)				// Check if data is available
	{
		if (WaitForSingleObject(m_ExitFillBuffer, 0)!=WAIT_TIMEOUT)
			return S_FALSE;
		SleepEx(1,TRUE);
	}

    HRESULT hr = pSample->GetPointer(&pData);					// Access the sample's data buffer
    if (FAILED(hr))
	{
		OutputDebugString(TEXT("Video pin: GetPointer error.\n"));
	    return hr;
	}
    cbData = pSample->GetSize();

	unitES_t* ut = m_pSrcFilter->m_pVideoFifo->front();
	bufferES_t* bt = ut->buffer;
	int bsize = bt->length;
	
	m_pSrcFilter->m_pVideoFifo->pop();
	pSample->SetActualDataLength(bsize);
	
	memcpy(pData,bt->buf,bsize);								// Fill buffer

	if (bt->video_arx==0)
		bt->video_arx=m_arx;
	if (bt->video_ary==0)
		bt->video_ary=m_ary;
	if (bt->video_width==0)
		bt->video_width=m_width;
	if (bt->video_height==0)
		bt->video_height = m_height;

	bool sane=true;
	// ======================================
	// Check for changes of video properties:
	if ( m_arx != bt->video_arx || m_ary!=bt->video_ary || m_width != bt->video_width || m_height != bt->video_height )
	{
		if ( bt->video_ary == 0 ||  bt->video_ary==0 || bt->video_width==0 ||  bt->video_height==0 || bt->video_width>2400 || bt->video_height > 1200 )
			sane=false;
		else if (bt->video_arx/bt->video_ary >4 || bt->video_width/bt->video_height >4)
			sane = false;
		else if (bt->video_ary/bt->video_arx >3 || bt->video_height/bt->video_width >3)
			sane =false;

        if (sane)
		{
			IPin* video_in;
			if (bt->stream_type == 0x1b)								// h.264 (parsers stores par into ar fields !!!)
			{
				VIDEOINFOHEADER2 *pMVIH = (VIDEOINFOHEADER2*)m_pSrcFilter->m_MediaTypeVideo.pbFormat;
				if (bt->video_ary!=0)  //calculate ar from par
				{
					UINT arx = bt->video_width * bt->video_arx / bt->video_ary;
					UINT ary = bt->video_height;
					if (arx!=0 && ary!=0)
						ReduceFraction(arx,ary);
					pMVIH->dwPictAspectRatioX=arx;
					pMVIH->dwPictAspectRatioY=ary;
				}
				else
				{
					pMVIH->dwPictAspectRatioX = bt->video_arx;	
					pMVIH->dwPictAspectRatioY = bt->video_ary;	
				}

				pMVIH->bmiHeader.biWidth = bt->video_width;
				pMVIH->bmiHeader.biHeight = bt->video_height;
			}
			else if (bt->stream_type == 0x01 || bt->stream_type == 0x02) //mpeg1, mpeg2
			{
				MPEG2VIDEOINFO* mpeg2vi =(MPEG2VIDEOINFO*) m_pSrcFilter->m_MediaTypeVideo.pbFormat;
				mpeg2vi->hdr.dwPictAspectRatioX= bt->video_arx;
				mpeg2vi->hdr.dwPictAspectRatioY = bt->video_ary;
				mpeg2vi->hdr.bmiHeader.biWidth = bt->video_width;
				mpeg2vi->hdr.bmiHeader.biHeight = bt->video_height;
			}
		
			if (this->ConnectedTo(& video_in)==S_OK) // Is protection needed????
			{
                if (video_in->QueryAccept(&(m_pSrcFilter->m_MediaTypeVideo))==S_OK)
					pSample->SetMediaType(&m_pSrcFilter->m_MediaTypeVideo);
				video_in->Release();
			}

			if (  m_arx != bt->video_arx || m_ary!=bt->video_ary )
				m_pSrcFilter->NotifyEvent(EC_MSG_AspectRatioChanged,(LONG_PTR)bt->video_arx,(LONG_PTR)bt->video_ary);
			if ( m_width != bt->video_width || m_height != bt->video_height)
				m_pSrcFilter->NotifyEvent(EC_MSG_VideoDimensionChanged,(LONG_PTR)bt->video_width,(LONG_PTR)bt->video_height);
		}
	}

	if (bt->video_afd >1 && m_afd != bt->video_afd)
	{
	    m_pSrcFilter->NotifyEvent(EC_MSG_VideoADFChanged,(LONG_PTR)bt->video_afd,0);
		m_afd = bt->video_afd; //store last afd
	}

	if (sane)
	{
		// Store picture dimensions:
		m_arx=bt->video_arx;
		m_ary=bt->video_ary;
		m_width = bt->video_width;
		m_height = bt->video_height;
	}
	//========================================

	// Packets without PTS.
	// Note that first sample will allways have PTS.
	if (!bt->has_pts)
	{
		//interpolate
		bt->present_time = m_prev_pts + ( m_prev_sample_field_count*(m_time_per_field.nominator*9 / (m_time_per_field.denominator*1000) ) );
	}

	// Start collecting PTS codes for audio resync - we need it to sync audio/video in a case if we are sending DTS codes, we can also calculate fps;  
	if (m_first_pts_array_count<NUM_PTS_COLLECT)
	{
		m_first_pts_array[m_first_pts_array_count]=bt->present_time;
		m_first_pts_array_count+=1;
	}
	
	if(m_first_pts==-1)										        // First video sample
	{
		m_time = GetTickCount();
		start = GLOBAL_VIDEO_DELAY; 
		m_first_pts= bt->present_time;						        // Store first pts (PTS from TS)
		m_first_dspts=start;								        // Store first DSpts (Dspts = Stream time)
		pSample->SetDiscontinuity(TRUE);
		m_pSrcFilter->NotifyEvent(EC_MSG_FirstVideoPacket,(bt->present_time&0xffffffffffffffff)>>32,bt->present_time&0xffffffff);
		
		m_pSrcFilter->m_pPinSubtitles->SetStartPts(m_first_pts,0);	// Set reference time for subtitles pin
		m_pSrcFilter->m_pPinAudio->SetVideoStartPts(m_first_pts);   // Set reference time for audio pin
	}
	else
	{
		m_field_count+=bt->video_frames;

		if (!m_ignore_PTS_before_first_I_frame && !m_delay_set && m_first_pts_array_count==NUM_PTS_COLLECT)// Correct A/V delay after we collect enaugh frames 
		{
			LONGLONG first_pts = getLPTS();
			LONGLONG pts_diff = m_first_pts - first_pts ;
			if (pts_diff != 0)
			{
				m_first_pts += pts_diff;
				m_pSrcFilter->m_pPinSubtitles->SetStartPts(first_pts,0);// Correct delay for subtites
				
				//m_first_dspts+=(pts_diff*1000/9);
				//m_pSrcFilter->m_pPinAudio->SetVideoStartPts(first_pts);
			}
			m_delay_set =true;
		}

		LONGLONG prev_diff = bt->present_time - m_prev_pts;
		
		// =====================================================================================================
		if (bt->present_time< PTS_10SEC && prev_diff < -(MAX_PTS - PTS_10SEC))						// On every 26 hours
		{
				m_pts_loop_count+=1;
				m_pSrcFilter->m_pPinSubtitles->SetPtsWrap(m_pts_loop_count, m_prev_dspts); // This will happen a little later than actual wrap in subtitle FillBuffer loop, but subtitle loop will simply discard to late subtitles. Not a big deal - or is it ?
				m_pSrcFilter->NotifyEvent(EC_MSG_TS_PTS_WRAP,(LONG_PTR)EC_TS_VIDEO_PIN,0);
		}
		else if ( bt->present_time > MAX_PTS-PTS_10SEC && prev_diff > MAX_PTS-PTS_10SEC  ) // Frames may come with non linear timings
		{
			if (m_pts_loop_count>=1)
			    m_pts_loop_count-=1;
			m_pSrcFilter->m_pPinSubtitles->SetPtsWrap(m_pts_loop_count, m_prev_dspts);
		}
		else if (abs(prev_diff) >PTS_10SEC || m_check_audio_pin_for_pts_jump) // Video samples might have negative jumps up to 10 seconds in NORMAL playback.
		{
			if (abs(prev_diff) >PTS_10SEC)   //more than 10s jump
			{
			    m_pSrcFilter->NotifyEvent(EC_MSG_TS_PTS_JUMP,(LONG_PTR)EC_TS_VIDEO_PIN,abs(abs(prev_diff))/90);
				m_last_jump_pts	= bt->present_time;		
			}

			//PTS jump may happen in stream itself or as a result of RTSP command (seek forward/backward) or as a result of dropped packets.

			pSample->SetDiscontinuity(TRUE);
			m_pSrcFilter->m_pPinAudio->reportPTSJumpinVideoPin();
			m_field_count=0;

			// Check audio pin first, because only video frames might be dropped.
			LONGLONG ref_aud_pts=0;
			LONGLONG ref_aud_ds=0;
			if ( m_pSrcFilter->m_pPinAudio->wasPTSJumpDetected(&ref_aud_pts, &ref_aud_ds, m_last_jump_pts))
			{
				m_first_dspts=ref_aud_ds;  
				m_first_pts=ref_aud_pts; 
			    m_check_audio_pin_for_pts_jump=false;
				m_pSrcFilter->m_pPinSubtitles->SetStartPts(m_first_pts,ref_aud_ds); // Notify Subtitle loop for pts jump
			}
			else if (!m_pSrcFilter->m_pPinAudio->hasRefTime()) // Performance, locking ???
			{
			    // No audio in in stream at all.
				LONGLONG curtime=0;
				m_pSrcFilter->GetStreamTime(&curtime) ;
				LONGLONG time_between_samples=max(0,GLOBAL_VIDEO_DELAY+ curtime - m_prev_dspts);
				m_first_dspts= m_prev_dspts + ( m_time_per_field.nominator * m_field_count / (m_time_per_field.denominator) ) + time_between_samples;			
				m_first_pts=bt->present_time+ (m_pts_loop_count*MAX_PTS);
				
				m_pSrcFilter->m_pPinSubtitles->SetStartPts(m_first_pts, m_first_dspts ); // Notify Subtitle loop for pts jump	
			}
			else
			{
				// Jump might not be yet visible to audio pin or only video packets were dropped.
			    m_first_dspts=m_prev_dspts ;
				m_first_pts=bt->present_time + (m_pts_loop_count*MAX_PTS);
			   // Video will go out of sync at this point.
			   // If jump in audio was less than 10seconds, but jump in video was more than 10 seconds, AV sync will never recover !

				if ( abs(bt->present_time - m_last_jump_pts) > PTS_10SEC)
				{
                    m_check_audio_pin_for_pts_jump=false;                  // Stop checking audio pin.
				    m_pSrcFilter->m_pPinSubtitles->SetStartPts(m_first_pts,  m_first_dspts); // Notify Subtitle loop for pts jump		
			    }
				else
			        m_check_audio_pin_for_pts_jump=true;                   // Check for jump next time.
			}
		}
		// ======================================================================================================

        diff = bt->present_time+(m_pts_loop_count*MAX_PTS) - m_first_pts;

		if (bt->starts_with_i_frame || bt->has_i_frame)
		{
			pSample->SetSyncPoint(TRUE);
		}
		
		if (bt->stream_type == 0x1b && !m_h264_timeperhalfframe_estimated)
			estimate_h264_tpf(m_prev_pts,bt->present_time);

		if(bt->discontinuity)
		{
			// LAV Video Decoder Note:
			// It seems that in some cases LAV decoder doesn't send  our PTS to renderer. In a case of dropped packets (frames) it seems that
			// it calculates them on its own based on fps. AV sync goes way off and eventually playback isn't smooth anymore.
			// It is interesting that this happens only on some streams (BBC knowledge, Disney channel).
			// Microsoft's DTD decoder doesn't seem to have this problem.

			pSample->SetDiscontinuity(TRUE);

			if (!bt->has_i_frame)
			{
			    pSample->SetPreroll(TRUE);
				m_discont_recovered =false;
			}

			if ( !(m_pSrcFilter->m_PTStoDec) )			//Sync DTS against PTS
			{
				start = m_first_dspts + ( (diff*1000)/9 );
				m_first_dspts=start;
				m_first_pts=bt->present_time +(m_pts_loop_count*MAX_PTS);
				m_field_count=0;
			}
			else
			{
				start = m_first_dspts + ( (diff*1000)/9 ) ;
			}
		}
		else
		{
			if (!m_discont_recovered)
			{
				if (bt->has_i_frame)
					m_discont_recovered =true;
				else
					pSample->SetPreroll(TRUE);
			}

			// H.264: we need to find out FPS from PTS to send correct DTS codes. Samples are NOT allways in presentation order.
			if (bt->stream_type == 0x1b && !m_h264_fps_found && m_first_pts_array_count==NUM_PTS_COLLECT)
			{
				getTPHF();
				m_h264_fps_found=true;
			}
			
			if ( !(m_pSrcFilter->m_PTStoDec) )
			{
				start = m_first_dspts+( m_time_per_field.nominator * m_field_count / (m_time_per_field.denominator) );
			}
			else 
				start = m_first_dspts + ( (diff*1000)/9 ) ;
		}
	}

	REFERENCE_TIME stop;	
	stop=start+( m_time_per_field.nominator * bt->video_frames  / (m_time_per_field.denominator) );
	stop = max (0,stop);
	stop=0;
	pSample->SetTime(&start,&stop);

	//if (start <0)
	//	pSample->SetPreroll(TRUE);
    //pSample->SetMediaTime(max(0,&start),max(0,&stop));

	m_prev_dspts=start;
	m_prev_pts=bt->present_time;
	m_prev_sample_field_count = bt->video_frames;
	m_prev_buf_size=bsize;

	if (m_source==SOURCE_FILE)
	{
		if ( m_pSrcFilter->m_pVideoFifo->items()<=MIN_ES_VIDEOFIFO_SIZE_FOR_FILE_SOURCE )
			SetEvent(m_pSrcFilter->m_PushLimitEvent); 
		else if (m_pSrcFilter->m_pVideoFifo->items()>MAX_ES_VIDEOFIFO_SIZE_FOR_FILE_SOURCE  )
			ResetEvent(m_pSrcFilter->m_PushLimitEvent);
	}
	
	delete[] bt->buf;
	delete bt;

	return S_OK;
}

// Must be called only when filter is stopped.
void CPushPinUdpVideo::resetTiming()
{
	//m_first_dspts= GLOBAL_VIDEO_DELAY ;
	//m_first_pts = m_prev_pts;
	m_first_pts=-1;
	m_pSrcFilter->m_pPinSubtitles->SetStartPts(-1,-1);
}


// DS units
void CPushPinUdpVideo::estimate_h264_tpf(LONGLONG first_pts, LONGLONG second_pts)
{
	LONGLONG diff = _abs64(first_pts-second_pts);
	timeperframe_t answer;
	answer.nominator=200000;
	answer.denominator=1;

	if(diff%1800 == 0 || diff==1800)					//25fps
	{

	}
	else if (diff%1875 == 0 || diff==1875)				//24fps
	{
		answer.nominator=625000;
		answer.denominator=3;
	}
	else if ((diff*10)%15015==0 || (diff*10)==15015)	//29.97fps - Not verified !
	{
		answer.nominator=500500;
		answer.denominator=3;
	}
	else if (diff==1876 || diff==1877 )					//23.976fps - Not verified !
	{
		answer.nominator=625625;
		answer.denominator=3;
	}

	m_h264_timeperhalfframe_estimated=true;
	m_time_per_field = answer;
}

// This function is used to find out h.264 FPS - needed to send correct DTS codes - ds units
void CPushPinUdpVideo::getTPHF()
{

	LONGLONG lowest_time_per_halfframe = (m_time_per_field.nominator / m_time_per_field.denominator)-1;
	bool found_lower=false;
	for (int i=1; i<NUM_PTS_COLLECT; i++)				// Find  lowest time per halfframe
	{
		LONGLONG tphf = m_first_pts_array[i] - m_first_pts_array[i-1];
		if(tphf < lowest_time_per_halfframe)
		{
			found_lower=true;
			lowest_time_per_halfframe=tphf;
		}
	}

	if (found_lower)
	{
		// 1st Check for standard double tpf
		if (lowest_time_per_halfframe == 900) // PAL double
		{
			m_time_per_field.nominator=100000;
			m_time_per_field.denominator=1;
		}
		else if (lowest_time_per_halfframe == 750 || lowest_time_per_halfframe == 751 ) //NTSC double - Not verified !
		{
			m_time_per_field.nominator= 250250;
			m_time_per_field.denominator= 3;
		}
		else
		{
			// This might not be true !
			m_time_per_field.nominator=lowest_time_per_halfframe*1000;
			m_time_per_field.denominator=9;
		}
	}
}

// Video frames are not allways delivered in presentation order. Audio and video are at first roughly synchronised by simply subtracting PTS of first received video
// frame from PTS of first received audio PTS. After enaugh PTS codes are collected, we can find a PTS code with lowest value than PTS of first received video frame.
// This function should be called before getTPHF !!!
LONGLONG CPushPinUdpVideo::getLPTS()
{
	shellsort(m_first_pts_array,NUM_PTS_COLLECT); // Sort array
	return m_first_pts_array[0];
}

// Audio output pin ====================================================

CPushPinUdpAudio::CPushPinUdpAudio(HRESULT *phr, CSource *pFilter ):
             CSourceStream(NAME("Udp Push Source Audio"),phr, pFilter, L"Audio")
{
	m_pSrcFilter=(CPushSourceUdp*)pFilter;
	m_ExitFillBuffer = CreateEvent (NULL, TRUE, FALSE, NULL) ; //  non signaled
	m_source=SOURCE_NET;
	m_first_pts=-1;
	m_first_dspts=-1;
	m_prev_pts=0;
	m_prev_dspts=0;
	m_first_video_pts=-1;	
	m_pts_loop_count=0;
	m_channels=0;
	m_samp_freq=0;
	m_time=0;
	m_delay=0;
	m_last_sample_length=0;
	m_rendered=false;
	m_video_has_data=false;
	m_dyn_recon_in_progress=FALSE;
	m_was_PTS_jump_detected=FALSE;
	m_video_pin_reported_jump=FALSE;
	m_has_video_pin_ref_time=FALSE;
	m_has_this_pin_ref_time=FALSE;
	m_last_jump_dspts=0;
	InitializeCriticalSection(&m_CritSec);
}

CPushPinUdpAudio::~CPushPinUdpAudio()
{
	Stop();
	CloseHandle(m_ExitFillBuffer);
	DeleteCriticalSection(&m_CritSec);
}

HRESULT CPushPinUdpAudio::GetMediaType(CMediaType *pmt)
{
 	CAutoLock cAutoLock(m_pFilter->pStateLock());
	
    CheckPointer(pmt, E_POINTER);
 
	pmt->Set( m_pSrcFilter->m_MediaTypeAudio );
 
    return S_OK;
}


HRESULT CPushPinUdpAudio::CheckMediaType (
    IN  const CMediaType * pMediaType
    )
{
    HRESULT hr ;

    if (pMediaType -> majortype  == MEDIATYPE_Audio &&
        (pMediaType -> subtype  == MEDIASUBTYPE_MPEG2_AUDIO ||
		pMediaType -> subtype  == MEDIASUBTYPE_DOLBY_AC3 ||
		pMediaType -> subtype  == MEDIASUBTYPE_MPEG1Audio ||
		pMediaType -> subtype == MEDIASUBTYPE_LATM_AAC ||
		pMediaType->subtype == MEDIASUBTYPE_MPEG1AudioPayload)
	    )
	{
		OutputDebugString(TEXT("Checking Media Type for Audio: OK\n"));
        hr = S_OK ;
    }
    else {
        OutputDebugString(TEXT("Checking Media Type for Audio: Failed\n"));
        hr = S_FALSE ;
    }

    return hr ;
}


HRESULT CPushPinUdpAudio::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProposed)
{
	HRESULT hr;

    OutputDebugString(TEXT("Decide buffer size for Audio Output:\n"));

    CAutoLock cAutoLock(m_pFilter->pStateLock());	//Lock parent of this stream (streaming thread lock)

    CheckPointer(pAlloc, E_POINTER);
    CheckPointer(pProposed, E_POINTER);
    
    if (pProposed->cBuffers < AUDIO_BUFFERS)					// Ensure a minimum number of buffers
    {
        pProposed->cBuffers = AUDIO_BUFFERS;
    }

    pProposed->cbBuffer = MAX_ES_LEN;				// This is probably way to much. Actual buffer size is set in fill buffer function.

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProposed, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}

	Dump1(TEXT("Actual buffer size %d \n"),Actual.cbBuffer);

	if (Actual.cbBuffer < pProposed->cbBuffer) 
	{
		return E_FAIL;
	}

    OutputDebugString(TEXT("Audio buffer size set."));
	return S_OK;
}

// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinUdpAudio::FillBuffer(IMediaSample * pSample)
{	
	start:

	if (!m_rendered)
			return S_FALSE;

	CheckPointer(pSample, E_POINTER);

	long cbData;
	BYTE* pData=NULL;

	LONGLONG diff;
	REFERENCE_TIME start=0;
   
	while (m_pSrcFilter->m_pAudioFifo->items()== 0)		// Check if data is available
	{
		if (WaitForSingleObject(m_ExitFillBuffer, 0)!=WAIT_TIMEOUT)
			return S_FALSE;
		
		SleepEx(1,TRUE);

		//if (m_first_pts==-1)
		//{
		//	pSample->SetActualDataLength(0);			// This should prevent blocking of the graph if no sound samples ara available.
		//	return S_OK;
		//}
	}

	EnterCriticalSection(&m_CritSec); // Protected because of dynamic reconnection.

	//  TODO: Find better solution for this problem !
	if (m_dyn_recon_in_progress)
	{
		LeaveCriticalSection(&m_CritSec);
		SleepEx(1,TRUE);
		
		goto start;	
	}

    HRESULT hr = pSample->GetPointer(&pData);			// Access the sample's data buffer
    if (FAILED(hr))
	{
		OutputDebugString(TEXT("Audio pin: GetPointer error.\n"));
	    return hr;
	}
    cbData = pSample->GetSize();

			
	unitES_t* ut = m_pSrcFilter->m_pAudioFifo->front();	// Fill buffer
	bufferES_t* bt = ut->buffer;           
	int bsize = bt->length;

	m_pSrcFilter->m_pAudioFifo->pop();

	pSample->SetActualDataLength(bsize);
	
	memcpy(pData,bt->buf,bsize);


	if(m_first_pts==-1)								       // First sample
	{
		m_time=GetTickCount();
		pSample->SetDiscontinuity(TRUE);

		if (m_video_has_data)
		{
			if (m_first_video_pts!=-1 )						// We have video - get reference time so we can calculate correct directshow time.
			{
				m_delay = bt->present_time - m_first_video_pts;
				start = GLOBAL_AUDIO_DELAY   + ( (m_delay*1000) / 9 ); 
				m_pSrcFilter->NotifyEvent(EC_MSG_Delay,(LONG_PTR)(start/1000),0);
				m_first_pts = bt->present_time;			//store first pts
				m_first_dspts = start;					//store first dspts
				pSample->SetDiscontinuity(TRUE);
				m_has_this_pin_ref_time=TRUE;
				m_pSrcFilter->NotifyEvent(EC_MSG_FirstAudioPacket,0,0);
			}
			else
			{
			    SleepEx(1,true);   // First pts of video not known yet
				goto start;
			}
		}
		else
		{
			m_first_pts= bt->present_time;    // No video at all, only audio in stream.
			start = GLOBAL_AUDIO_DELAY ;
			m_first_dspts = start;
		}
	}
	else 
	{
		if(bt->discontinuity)
			pSample->SetDiscontinuity(TRUE);
		
		LONGLONG prev_diff = bt->present_time - m_prev_pts;
		
		// ==============================================================================================
		if (bt->present_time<PTS_10SEC && prev_diff < -(MAX_PTS - PTS_10SEC))				// on every 26 hours
		{
			m_pts_loop_count+=1;
			m_pSrcFilter->NotifyEvent(EC_MSG_TS_PTS_WRAP,(LONG_PTR)EC_TS_AUDIO_PIN,0);
		}
		else if ( bt->present_time > MAX_PTS - PTS_10SEC && prev_diff > MAX_PTS - PTS_10SEC  ) // This probably never occoures.
		{
			if (m_pts_loop_count>=1)
			    m_pts_loop_count-=1;
		}
		else if ( prev_diff < 0 || prev_diff > PTS_10SEC ) //Possible pts jump.
		{
			pSample->SetDiscontinuity(TRUE);
			LONGLONG curtime=0;
			LONGLONG ds_prev_diff = prev_diff*1000/9;
			m_pSrcFilter->GetStreamTime(&curtime) ;
			m_last_jump_dspts=curtime;
			m_was_PTS_jump_detected=true; // Set for real jump or missing packets.
			m_pSrcFilter->NotifyEvent(EC_MSG_TS_PTS_JUMP,(LONG_PTR)EC_TS_AUDIO_PIN,abs(prev_diff/90));
			
			// Check for jump in video pin. No jump in video means that jump in audio is result of missing audio packets.
			LONGLONG time_between_samples=max(0,GLOBAL_AUDIO_DELAY+ curtime - m_prev_dspts);//Time that has passed between the last SCHEDULED sample and current sample
			
			if (  (time_between_samples - DSPTS_5SEC < abs(ds_prev_diff) && time_between_samples + DSPTS_5SEC < abs(ds_prev_diff) ) || m_video_pin_reported_jump ||  !m_has_video_pin_ref_time  )
			{
				//Real jump.
				//Recalculate ds times. Video pin will use this value to resync.
				m_first_dspts=m_prev_dspts + m_last_sample_length + time_between_samples ;
				m_first_pts=bt->present_time + (m_pts_loop_count*MAX_PTS);
				m_video_pin_reported_jump=FALSE; //reset
			}
		}
		// ==============================================================================================

		// Calculate directshow timestamps:
		diff =bt->present_time+(m_pts_loop_count*MAX_PTS) - m_first_pts;
	    start = m_first_dspts + ( (diff * 1000)/9 );

		//Reset some stuff after some time:
		if (m_was_PTS_jump_detected && start- m_last_jump_dspts > DSPTS_5SEC )
            m_was_PTS_jump_detected=FALSE;
		if (m_video_pin_reported_jump && start- m_last_jump_dspts > DSPTS_5SEC )
            m_video_pin_reported_jump=FALSE;
	}

	m_prev_pts=bt->present_time;

	REFERENCE_TIME stop=start;
    m_last_sample_length =start - m_prev_dspts;
	m_prev_dspts =start;

	pSample->SetTime(&start,&stop);
	
	if (bt->pid_format_change)
	{
		pSample->SetMediaType(&m_pSrcFilter->m_MediaTypeAudio);
	}
	
	// Here we limit file reading speed.
	// TODO limit in video pin if ts has no audio !!!!!!!!!!!!!!!!!!!
	if (m_source==SOURCE_FILE)
	{
		if (m_pSrcFilter->m_pAudioFifo->items() <= MIN_ES_AUDIOFIFO_SIZE_FOR_FILE_SOURCE  )
			SetEvent(m_pSrcFilter->m_PushLimitEvent); 
		else if (m_pSrcFilter->m_pAudioFifo->items() > MAX_ES_AUDIOFIFO_SIZE_FOR_FILE_SOURCE  )
			ResetEvent(m_pSrcFilter->m_PushLimitEvent);
	}

    delete[] bt->buf;
	delete bt;	

	LeaveCriticalSection(&m_CritSec);

	return S_OK;
}

void CPushPinUdpAudio::SetVideoStartPts(LONGLONG pts)
{
	EnterCriticalSection(&m_CritSec);
	m_first_video_pts = pts;
	m_has_video_pin_ref_time=TRUE;
	LeaveCriticalSection(&m_CritSec);
}


void CPushPinUdpAudio::resetTiming()
{
	EnterCriticalSection(&m_CritSec);
    m_first_pts=-1;
    LeaveCriticalSection(&m_CritSec);
}

BOOL CPushPinUdpAudio::wasPTSJumpDetected(LONGLONG* ref_pts, LONGLONG* ref_ds, LONGLONG vid_pts)
{
	BOOL answ ;
	EnterCriticalSection(&m_CritSec);
	answ = m_was_PTS_jump_detected;
	if (answ)
	{
	    m_was_PTS_jump_detected=false;
		if (vid_pts < m_first_pts)               //Prevent video samples comming to late at the renderer after AV resync.
		{
		    m_first_dspts+= m_first_pts-vid_pts;
		}
	}
	*ref_pts=m_first_pts;
	*ref_ds = m_first_dspts;

    LeaveCriticalSection(&m_CritSec);
	return answ;
}

void CPushPinUdpAudio::reportPTSJumpinVideoPin()
{	
	EnterCriticalSection(&m_CritSec);
	m_video_pin_reported_jump=TRUE;
    LeaveCriticalSection(&m_CritSec);
}

BOOL CPushPinUdpAudio::hasRefTime()
{	
	BOOL answ;
	EnterCriticalSection(&m_CritSec);
	answ = m_has_this_pin_ref_time;
    LeaveCriticalSection(&m_CritSec);
	return answ;
}


STDMETHODIMP CPushPinUdpAudio::Block(/* [in] */ DWORD dwBlockFlags, 
                           /* [in] */ HANDLE hEvent)
{
		    
	if (dwBlockFlags & AM_PIN_FLOW_CONTROL_BLOCK)
	{
		// Block fill buffer
		EnterCriticalSection(&m_CritSec);
		m_dyn_recon_in_progress=true;
		LeaveCriticalSection(&m_CritSec);
	}
	else
	{
		EnterCriticalSection(&m_CritSec);
		if (m_dyn_recon_in_progress)
		{
			//m_pSrcFilter->m_pAudioFifo->empty();  // Empty everything just before we let data flow from pin.
			m_dyn_recon_in_progress=false;
		}
		LeaveCriticalSection(&m_CritSec);
	}
	return S_OK;
}

#define BI_ALPHABITFIELDS  6L
// Subtitles output pin  ====================================================
CPushPinUdpSubs::CPushPinUdpSubs(HRESULT *phr, CSource *pFilter ):
             CSourceStream(NAME("Udp Push Source Subtitles"),phr, pFilter, L"DVB-Subtitles")
{
	m_pSrcFilter=(CPushSourceUdp*)pFilter;
	m_ExitFillBuffer = CreateEvent (NULL, TRUE, FALSE, NULL) ; //  non signaled
	m_source=SOURCE_NET;

	m_first_pts=-1;
	m_first_dspts=-1;
	m_prev_pts=0;
	m_prev_dspts=0;
	m_pts_loop_count=0;
	m_last_sample_time=0;
	m_ds_ref_time =0;
	m_ds_approx_wrap_time=0;
	m_ds_current_time=0;
	m_time_out =50000000;
	m_last_page_valid=false;
	m_first_call=true;
	m_page_id =0;
	m_ancillary_id=0;
	m_dvb_decoder = new CDvbSub;
	
	ZeroMemory(&m_MediaType, sizeof(AM_MEDIA_TYPE));
	m_MediaType.majortype=MEDIATYPE_Video;
	m_MediaType.subtype=MEDIASUBTYPE_ARGB32;
	m_MediaType.formattype = FORMAT_VideoInfo2;
	m_MediaType.bFixedSizeSamples=true;
	m_MediaType.bTemporalCompression =false;
    m_MediaType.cbFormat = sizeof(VIDEOINFOHEADER2) ;
	m_MediaType.pbFormat = (BYTE*)CoTaskMemAlloc(m_MediaType.cbFormat);
	ZeroMemory(m_MediaType.pbFormat, m_MediaType.cbFormat);

    VIDEOINFOHEADER2 *pMVIH = (VIDEOINFOHEADER2*)m_MediaType.pbFormat;

	// 720x576
	SetRectEmpty(&(pMVIH->rcSource));
    SetRectEmpty(&(pMVIH->rcTarget));
	//pMVIH->rcSource.left=0;
	//pMVIH->rcSource.right = MIN_SUB_WIDTH; 
	//pMVIH->rcSource.top = 0;
 //   pMVIH->rcSource.bottom = MIN_SUB_HEIGHT;
	//pMVIH->rcTarget.left=0;
	//pMVIH->rcTarget.right = SUB_WIDTH;
	//pMVIH->rcTarget.top=0;
 //   pMVIH->rcTarget.bottom =  SUB_HEIGHT;

	pMVIH->bmiHeader.biSizeImage =MIN_SUB_WIDTH*MIN_SUB_HEIGHT*4;
	pMVIH->bmiHeader.biCompression = BI_RGB;
	pMVIH->bmiHeader.biBitCount = 32;
	pMVIH->bmiHeader.biWidth = MIN_SUB_WIDTH;
	pMVIH->bmiHeader.biHeight =  MIN_SUB_HEIGHT;
	pMVIH->dwPictAspectRatioX=4;
	pMVIH->dwPictAspectRatioY=3;
	pMVIH->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pMVIH->bmiHeader.biPlanes=1;
	pMVIH->bmiHeader.biClrUsed=0;
	pMVIH->bmiHeader.biClrImportant=0;
	pMVIH->AvgTimePerFrame=2000000; //default 5 fps
	pMVIH->dwInterlaceFlags = 0;

	m_empty_img=NULL;

	InitializeCriticalSection(&m_CritSec);
	
}

CPushPinUdpSubs::~CPushPinUdpSubs()
{
	Stop();
	if (m_empty_img != NULL)
		delete[] m_empty_img;
	DeleteCriticalSection(&m_CritSec);
	CloseHandle(m_ExitFillBuffer);
	delete m_dvb_decoder;
}

HRESULT CPushPinUdpSubs::GetMediaType(CMediaType *pmt)
{
 	CAutoLock cAutoLock(m_pFilter->pStateLock());
	
    CheckPointer(pmt, E_POINTER);

 	pmt->Set( m_MediaType);
 
    return S_OK;
}

void CPushPinUdpSubs::SetMediaTypeParams(BYTE colorspace, DWORD width, DWORD height, RECT src, RECT trg, UINT arx, UINT ary)
{
	VIDEOINFOHEADER2 *pMVIH = (VIDEOINFOHEADER2*)m_MediaType.pbFormat;

	//if (src.right!=0)
	//    pMVIH->rcSource = src;
	if (trg.right !=0)
		pMVIH->rcTarget = trg;
	if (width != 0 && height !=0)
	{
	    pMVIH->bmiHeader.biSizeImage = width*height*4;	
	    pMVIH->bmiHeader.biWidth = width;
	    pMVIH->bmiHeader.biHeight =  height;
	}

	if (colorspace != 0)
	{
		if ( colorspace == COLORSPACE_AYUV)
		{
			m_MediaType.subtype=MEDIASUBTYPE_AYUV;
			pMVIH->bmiHeader.biCompression = MEDIASUBTYPE_AYUV.Data1;
			m_dvb_decoder->set_output_color_space(COLORSPACE_AYUV);
		}
		else if (colorspace == COLORSPACE_ARGB)
		{
			m_MediaType.subtype=MEDIASUBTYPE_ARGB32;
			pMVIH->bmiHeader.biCompression = BI_RGB;
            m_dvb_decoder->set_output_color_space(COLORSPACE_ARGB);
		}
	}

	if (arx != 0 && ary != 0)
	{
		pMVIH->dwPictAspectRatioX=arx;
	    pMVIH->dwPictAspectRatioY=ary;
	}
}

HRESULT CPushPinUdpSubs::CheckMediaType (
    IN  const CMediaType * pMediaType
    )
{
    HRESULT hr =S_FALSE;

    if (pMediaType -> majortype  == MEDIATYPE_Video &&
          pMediaType->formattype == FORMAT_VideoInfo2 &&
		( (pMediaType->subtype == MEDIASUBTYPE_AYUV  || pMediaType->subtype == MEDIASUBTYPE_ARGB32 ) 
		)
		)
	{
		VIDEOINFOHEADER2 *pMVIH = (VIDEOINFOHEADER2*)pMediaType->pbFormat;
		if (abs(pMVIH->bmiHeader.biHeight)<MIN_SUB_HEIGHT || abs(pMVIH->bmiHeader.biWidth)<MIN_SUB_WIDTH)
		{
			OutputDebugString(TEXT("Checking Media Type for DVB Subtitles: FAILED\n"));
			hr =E_FAIL;
		}
		else
		{
		    OutputDebugString(TEXT("Checking Media Type for DVB Subtitles: OK\n"));
			bool center = true;
			if (abs(pMVIH->bmiHeader.biHeight) != abs(pMVIH->rcSource.bottom-pMVIH->rcSource.top) || abs(pMVIH->bmiHeader.biWidth) != abs(pMVIH->rcSource.left-pMVIH->rcSource.right) )
				center = false;

		    m_dvb_decoder->report_preferred_image_size(abs(pMVIH->bmiHeader.biWidth),abs(pMVIH->bmiHeader.biHeight),1,1,center);
			// Update our media type
			VIDEOINFOHEADER2 *pMVIHDecided = (VIDEOINFOHEADER2*)m_MediaType.pbFormat;
			pMVIHDecided->bmiHeader.biWidth=pMVIH->bmiHeader.biWidth;
			pMVIHDecided->bmiHeader.biHeight=pMVIH->bmiHeader.biHeight;
			pMVIHDecided->rcSource.left=0;
			pMVIHDecided->rcSource.top=0;
			pMVIHDecided->rcSource.right = abs(pMVIH->bmiHeader.biWidth);
			pMVIHDecided->rcSource.bottom= abs(pMVIH->bmiHeader.biHeight);
			pMVIHDecided->bmiHeader.biSize = pMVIHDecided->bmiHeader.biWidth * pMVIHDecided->bmiHeader.biHeight * 4 ;
			pMVIHDecided->dwPictAspectRatioX = pMVIH->dwPictAspectRatioX;
			pMVIHDecided->dwPictAspectRatioY = pMVIH->dwPictAspectRatioY;
            hr = S_OK ;
		}
    }
    else {
        OutputDebugString(TEXT("Checking Media Type for DVB Subtitles: Failed\n"));
        hr = S_FALSE ;
    }

    return hr ;
}

HRESULT CPushPinUdpSubs::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProposed)
{
	HRESULT hr;

    OutputDebugString(TEXT("Deciding buffer size for Subtitle Output:\n"));

    CAutoLock cAutoLock(m_pFilter->pStateLock());	// Lock parent of this stream (streaming thread lock)

    CheckPointer(pAlloc, E_POINTER);
    CheckPointer(pProposed, E_POINTER);

    if (pProposed->cBuffers == 0)					// Ensure a minimum number of buffers
    {
        pProposed->cBuffers = 1;
    }
	
    pProposed->cbBuffer = m_dvb_decoder->get_image_size()*4;				

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProposed, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}

	Dump1(TEXT("Actual buffer size %d \n"),Actual.cbBuffer);

	if (Actual.cbBuffer < pProposed->cbBuffer) 
	{
		return E_FAIL;
	}

    OutputDebugString(TEXT("Subtitle buffer size set."));
	return S_OK;
}

// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinUdpSubs::FillBuffer(IMediaSample * pSample)
{
	CheckPointer(pSample, E_POINTER);

    long cbData;
	BYTE* pData=NULL;

	BYTE action;
	bool no_data=true;

	CRefTime ctime;

	if (WaitForSingleObject(m_ExitFillBuffer, 0)!=WAIT_TIMEOUT)
		return S_FALSE;
	
	EnterCriticalSection(&m_CritSec);                // TODO: Use Autolock
	                                                 // Needed because on PTS jump, fifo must be cleared.
	
	if (m_pSrcFilter->m_pSubsFifo->items()>0)	     // Check if data is available
		no_data=false;
	//else
	//	Sleep(1);
	
    HRESULT hr = pSample->GetPointer(&pData);		// Access the sample's data buffer
    if (FAILED(hr))
	{
		OutputDebugString(TEXT("Subtitles pin: GetPointer error.\n"));
	    
		LeaveCriticalSection(&m_CritSec);	
		return hr;
	}
    cbData = pSample->GetSize();
    _32BIT_PIXEL* pImage=NULL;

	if (m_first_call)
	{ 
		IPin* video_in;
		if (this->ConnectedTo(& video_in)==S_OK)
		{
			if (video_in->QueryAccept(&m_MediaType)==S_OK)
		       pSample->SetMediaType(&m_MediaType);
		}
		m_first_call=false;
	}


	m_pSrcFilter->StreamTime(ctime);
	m_ds_current_time = ctime.m_time;	
	
	// Get time since last subtitle started.
	// If time is larger than timeout, clear screen.
	// If new sample is available and time of new sample is lower then current pts, show next subtitle.
	// If new sample is available but time of that sample is larger than current pts, clear screen.
	
	unitES_t* ut=NULL;
	bufferES_t* bt=NULL;
	int bsize=0;
	LONGLONG start_pts = -1;
    LONGLONG sample_ds_start;
	LONGLONG ds_ref_time=0;
	start_pts = GetStartPts(&ds_ref_time);
	LONGLONG vid_pts_wrap_time=0;

	if (no_data)
	{
		if( m_last_sample_time!=0 && ( ((m_ds_current_time - m_last_sample_time) > m_time_out) ) )
		{
			if (m_last_page_valid)
			{
				m_dvb_decoder->clear_img();
				m_last_page_valid=false;
			}
			action = DVBDEC_CLEAR;
		}
		else
			action = DVBDEC_SHOW_PREV;
	}
	else
	{
		ut = m_pSrcFilter->m_pSubsFifo->front();
		bt = ut->buffer;
		bsize = bt->length;

		if (start_pts != -1)
		{
			// We can't simply detect PTS wrap in subtitles, because they are not continous.
		
			sample_ds_start = ( (bt->present_time + (GetPtsCounter()*MAX_PTS) - start_pts)*1000/9 ) +  GLOBAL_SUB_DELAY +  m_ds_ref_time;	// Calculate Ds time.
			
			check:

			LONGLONG sample_ds_start_minus_timeperframe = sample_ds_start - (DVBDEC_TIMEPERFRAME/2) ;

			if( sample_ds_start_minus_timeperframe  > m_ds_current_time)				         // Subtitle not yet to be shown.
			{													
				if (sample_ds_start_minus_timeperframe > m_ds_current_time + (6* DSPTS_10SEC) )
				{
					action = DVBDEC_SKIP; //Something is wrong
				}
				else if( m_last_sample_time!=0 && ((m_ds_current_time - m_last_sample_time) > m_time_out)) // Should be previous sample still on screen ?
				{
					if (m_last_page_valid)
					{
						m_dvb_decoder->clear_img();
						m_last_page_valid=false;
					}
					action = DVBDEC_CLEAR;
				}
				else
					action = DVBDEC_SHOW_PREV;
			}
			else                                // Time to show this subtitle, but check first if sample is not to late.
			{		
				if ( m_ds_current_time - sample_ds_start_minus_timeperframe > m_time_out)
				{
					 // Sample to late, or PTS WRAP wasn't applied yet (10 seconds before wrap).
					if (  (sample_ds_start_minus_timeperframe + DS_MAX_PTS)-DSPTS_10SEC < m_ds_current_time ) //It must be wrong pts counter. Subtitle was received before video pin reported pts wrap.
					{
						sample_ds_start+=DS_MAX_PTS; // Correct time.
						goto check;
					}
					else
					    action = DVBDEC_SKIP;    // Sample is really to late, skip it.
				}
				else
					action = DVBDEC_SHOW_NEW;   // Time to decode and show subtitle.
			}
		}
		else
			action = DVBDEC_CLEAR; // Presentation time can't be calculated yet
	}

	if (m_empty_img==NULL) //Image size is defenetly decided at this time. Create an empty image to show it when no subtitles should be shown.
	{
	    int img_size = m_dvb_decoder->get_image_size(); 
		m_empty_img = new _32BIT_PIXEL[img_size];
		ZeroMemory(m_empty_img,img_size*4);
	}

	//int width,height;
	//m_dvb_decoder->get_image_size(&width,&height);
	//// Update our media type
	//VIDEOINFOHEADER2 *pMVIHDecided = (VIDEOINFOHEADER2*)m_MediaType.pbFormat;
	//if ( abs(pMVIHDecided->bmiHeader.biWidth) != width || abs(pMVIHDecided->bmiHeader.biHeight) != height)
	//{
	//    if (pMVIHDecided->bmiHeader.biWidth>0)
 //          pMVIHDecided->bmiHeader.biWidth=width;
	//	else
 //          pMVIHDecided->bmiHeader.biWidth=-width;

	//    if (pMVIHDecided->bmiHeader.biHeight>0)
 //          pMVIHDecided->bmiHeader.biHeight=height;
	//	else
 //          pMVIHDecided->bmiHeader.biHeight=-height;

	//	pSample->SetMediaType(&m_MediaType);
	//}
	//pMVIHDecided->rcSource.left=0;
	//pMVIHDecided->rcSource.top=0;
	//pMVIHDecided->rcSource.right = abs(pMVIH->bmiHeader.biWidth);
	//pMVIHDecided->rcSource.bottom= abs(pMVIH->bmiHeader.biHeight);
	//pMVIHDecided->bmiHeader.biSize = pMVIHDecided->bmiHeader.biWidth * pMVIHDecided->bmiHeader.biHeight * 4 ;
	//pMVIHDecided->dwPictAspectRatioX = pMVIH->dwPictAspectRatioX;
	//pMVIHDecided->dwPictAspectRatioY = pMVIH->dwPictAspectRatioY;

	if (action == DVBDEC_SHOW_PREV)
	{
		m_dvb_decoder->GetCurrentPage(&(pImage));
		pSample->SetActualDataLength(m_dvb_decoder->get_image_size()*4);
		memcpy(pData,pImage,m_dvb_decoder->get_image_size()*4);
		
		LeaveCriticalSection(&m_CritSec);	
		return S_OK;
	}
	else if (action == DVBDEC_CLEAR)
	{
		pSample->SetActualDataLength(m_dvb_decoder->get_image_size()*4);
		memcpy(pData,m_empty_img,m_dvb_decoder->get_image_size()*4);
		
		LeaveCriticalSection(&m_CritSec);	
		return S_OK;
	}
	else if (action == DVBDEC_SKIP)
	{
		pSample->SetActualDataLength(m_dvb_decoder->get_image_size()*4);
		memcpy(pData,m_empty_img,m_dvb_decoder->get_image_size()*4);
		m_pSrcFilter->m_pSubsFifo->pop();
		delete[] bt->buf;
	    delete bt;	
		
		LeaveCriticalSection(&m_CritSec);	
		return S_OK;
	}

	m_last_page_valid=true;

	// decode ==============================
	
	BYTE time_out; 
	BYTE result =0;
	BYTE counter =0;
	
	m_last_sample_time=m_ds_current_time;
	m_prev_pts=bt->present_time;
	do 
	{
		result = m_dvb_decoder->PreProcessor(bt->buf,bt->length, m_page_id,m_ancillary_id,&(pImage),&time_out);
		m_pSrcFilter->m_pSubsFifo->pop();
		memcpy(pData,pImage,m_dvb_decoder->get_image_size()*4);
		delete[] bt->buf;
	    delete bt;
		if (result == COMPLETE_DISPLAY_SET)
			break;
		if (counter > 10)
			break;
		if (m_pSrcFilter->m_pSubsFifo->items()==0)
			break;
		
		ut = m_pSrcFilter->m_pSubsFifo->front();
		bt = ut->buffer;
		bsize = bt->length;

		if (counter > 0)
			Sleep(1);
        
		counter+=1;
	}
	while(TRUE);

	pSample->SetActualDataLength(m_dvb_decoder->get_image_size()*4);

	if (time_out > 5)
		m_time_out=50000000;
	else
		m_time_out = time_out*10000000;

	m_prev_dspts=m_ds_current_time;
	
	LeaveCriticalSection(&m_CritSec);		
	return S_OK;
}

void CPushPinUdpSubs::SetStartPts(LONGLONG pts, LONGLONG ds)
{
	EnterCriticalSection(&m_CritSec);
	m_first_pts = pts;
	if (ds > -1)
	    m_ds_ref_time = ds;
	if (ds > 0) // PTS jump. Empty previous subtitles
		m_pSrcFilter->m_pSubsFifo->empty(); 
	LeaveCriticalSection(&m_CritSec);
}

LONGLONG CPushPinUdpSubs::GetStartPts(LONGLONG* ds_ref_time)
{
	LONGLONG answer;
	EnterCriticalSection(&m_CritSec);
	answer = m_first_pts;
	*ds_ref_time = m_ds_ref_time;
	LeaveCriticalSection(&m_CritSec);
	return answer;
}

void CPushPinUdpSubs::SetPtsWrap(LONGLONG factor, LONGLONG approx_ds_time)
{
	EnterCriticalSection(&m_CritSec);
	m_pts_loop_count=factor;
	m_ds_approx_wrap_time= approx_ds_time;
	LeaveCriticalSection(&m_CritSec);
}

LONGLONG CPushPinUdpSubs::GetPtsCounter()
{
	LONGLONG answer;
	EnterCriticalSection(&m_CritSec);
	if ( m_ds_current_time < m_ds_approx_wrap_time)   // Due the timeshift, video pin can report PTS wrap way to early.
	    answer = max(0, m_pts_loop_count-1);
	else
	    answer = m_pts_loop_count;
	LeaveCriticalSection(&m_CritSec);
	return answer;
}


//HRESULT CPushPinUdpSubs::DoBufferProcessingLoop(void)
//{
//	Command com;
//
//    OnThreadStartPlay();
//
//    do {
//	while (!CheckRequest(&com)) {
//
//	    IMediaSample *pSample;
//
//	    HRESULT hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
//	    if (FAILED(hr)) {
//                Sleep(1);
//		continue;	// go round again. Perhaps the error will go away
//			    // or the allocator is decommited & we will be asked to
//			    // exit soon.
//	    }
//
//	    // Virtual function user will override.
//	    hr = FillBuffer(pSample);
//
//	    if (hr == S_OK) {
//		hr = Deliver(pSample);
//                pSample->Release();
//
//                // downstream filter returns S_FALSE if it wants us to
//                // stop or an error if it's reporting an error.
//                if(hr != S_OK)
//                {
//                  DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
//                  return S_OK;
//                }
//
//	    } 
//		else if (hr==99)
//		{
//			pSample->Release();
//		}
//		
//		else if (hr == S_FALSE) {
//                // derived class wants us to stop pushing data
//		pSample->Release();
//		DeliverEndOfStream();
//		return S_OK;
//	    } else {
//                // derived class encountered an error
//                pSample->Release();
//		DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
//                DeliverEndOfStream();
//                m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
//                return hr;
//	    }
//
//            // all paths release the sample
//	}
//
//        // For all commands sent to us there must be a Reply call!
//	if (com == CMD_RUN || com == CMD_PAUSE) {
//	    Reply(NOERROR);
//	} else if (com != CMD_STOP) {
//	    Reply((DWORD) E_UNEXPECTED);
//	    DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
//	}
//    } while (com != CMD_STOP);
//
//    return S_FALSE;
//
//}

//========================================================================
//Filter==================================================================

CPushSourceUdp::CPushSourceUdp(IUnknown *pUnk, HRESULT *phr)
           : CSource(NAME("Udp Source"), pUnk, CLSID_UdpSourceFilter)
{
	//_CrtSetBreakAlloc(51);

	m_bitratetimer=GetTickCount();
	m_packetcounter=0;
	m_tsbitrate=0;

	m_mode = PLAYBACK_MODE;
	m_source = SOURCE_NET;
	ZeroMemory(m_src_file,sizeof(m_src_file));
	
	m_ParseThread=NULL;
	m_ParseDemuxThread=NULL;
	m_SubtitleDecoderThread=NULL;
	
	m_pReceiver = new CReceiver(this); 
	m_pParser = new CParser(RECV_BUFFER);

	m_pDemuxFifo=new CFifo(true);
	m_pVideoFifo=new CFifoES();
	m_pAudioFifo=new CFifoES();
	m_pSubsFifo=new  CFifoES();

	ZeroMemory(&m_MediaTypeVideo, sizeof(AM_MEDIA_TYPE));
   	ZeroMemory(&m_MediaTypeAudio, sizeof(AM_MEDIA_TYPE));

	m_DemuxerConfiguredEvent =  CreateEvent (NULL, TRUE, FALSE, NULL) ; //non signaled
	m_DemuxFifoDataEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;		//non signaled
	m_DemuxFillingEnded = CreateEvent (NULL, TRUE, TRUE, NULL);			//  signaled
	m_PushLimitEvent = CreateEvent (NULL, TRUE, TRUE, NULL);			//  signaled

	m_Apid=0;
	m_Vpid=0;
	m_Spid=0;
	m_PMT_PID=0;
	m_VpidType=0;
	m_ApidType=0;

	m_PCRpid=0;
	m_resetAudES=false;
	m_resetSubES=false;
	m_resetVidES=false;
	m_PTStoDec=true;
	m_dont_leave_multicast=FALSE;
	m_pause_second_time = FALSE;
	m_stream_has_video=false;
	m_stream_has_audio=false;
	m_stream_has_subtitles=false;
	m_reciver_started=false;

	m_video_pin_rendered=false;
	m_audio_pin_rendered=false;
	m_sub_pin_rendered=false;
	m_subtitle_polling=false;
	m_audio_pid_format_change=false;

	ZeroMemory(&m_mpeg2_ext_info,sizeof(mpeg2_ext_info_t));
	ZeroMemory(&m_h264_ext_info,sizeof(h264_ext_info_t));

	m_Mip=ADDR_ANY;
	m_Nip=ADDR_ANY;
	m_nic_index=0xFFFFFFFF;
	
	// Stream syhronisation:
	m_first_vid_pts=-1;
	m_first_aud_pts=-1;
	m_first_sub_pts=-1;

	m_dvb_sub_status=DVBDEC_CLEAR;

	// Pin magically adds itself to our pin array:
   	m_pPinVideo = new CPushPinUdpVideo(phr,this);
	m_pPinAudio = new CPushPinUdpAudio(phr,this);
	m_pPinSubtitles = new CPushPinUdpSubs(phr,this);


	if (phr)
	{
		if ( m_pPinVideo==NULL || m_pPinAudio == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
}

CPushSourceUdp::~CPushSourceUdp()
{
	// If graph was never started, Stop function is never called, so we must call it from descructor.
	// This happens when receiver doesn't get any data and therefore application never calls stop.
	
	//FILTER_STATE state;
	//CPushSourceUdp::GetState(1000,&state);
	//if (state == State_Running)
	m_dont_leave_multicast=false;
	CPushSourceUdp::Stop();
	
	// Make sure that parsing/pushing and demux threads finished.
	WaitForSingleObject(m_ParseThread,10000);
	WaitForSingleObject(m_ParseDemuxThread,10000);
    WaitForSingleObject(m_SubtitleDecoderThread,10000);
	
	delete m_pReceiver;
	m_pReceiver=NULL;
	delete m_pDemuxFifo;
	m_pDemuxFifo=NULL;
	delete m_pVideoFifo;
	m_pVideoFifo=NULL;
	delete m_pAudioFifo;
	m_pAudioFifo=NULL;
	delete m_pSubsFifo;
	m_pSubsFifo=NULL;
	delete m_pParser;
	m_pParser=NULL;
	delete m_pPinVideo;
	m_pPinVideo=NULL;
	delete m_pPinAudio;
	m_pPinAudio=NULL;
	delete m_pPinSubtitles;
	m_pPinSubtitles = NULL;

	if (m_MediaTypeAudio.majortype != GUID_NULL)
		FreeMediaType(m_MediaTypeAudio);
	if (m_MediaTypeVideo.majortype != GUID_NULL)
		FreeMediaType(m_MediaTypeVideo);

	if (m_ParseThread!=NULL)
		CloseHandle(m_ParseThread);
	if (m_ParseDemuxThread!=NULL)
		CloseHandle(m_ParseDemuxThread);
	if (m_SubtitleDecoderThread!=NULL)
		CloseHandle(m_SubtitleDecoderThread);

	CloseHandle(m_DemuxFifoDataEvent);
	
	_CrtDumpMemoryLeaks();
}

CBasePin * CPushSourceUdp::GetPin (IN  int Index )
{
   CBasePin *  pPin ;

	CAutoLock cAutoLock(&m_pLock);
   //OutputDebugString(TEXT("Get pin called \n"));

	if (Index == 0)
	{
		pPin = m_pPinVideo;
	}
	else if (Index == 1)
	{
		pPin=m_pPinAudio;
	}
	else if (Index==2)
	{
		pPin = m_pPinSubtitles;
	}
    else {
        pPin = NULL ;
    }

    return pPin ;
}

// This function is used only in parsing mode. Application can wait for additional informations like SDT table and MUST stop parser manually.
void CPushSourceUdp::justParse()
{
	DWORD dwResult=0;
	bool exit=false;
	bool first_sample=true;
	bool main_data_prsd_notf=false;
	bool sdt_table_prsd_notf=false;

	NotifyEvent(EC_PARSER_STARTED,0,0);

	do
	{
		dwResult = WaitForSingleObject(m_ExitParseThreadEvent, 0);	// Do we have to exit ?
		if(dwResult != WAIT_TIMEOUT)
			return;

		dwResult = WaitForSingleObject(m_pReceiver->data->m_data_rcvd_event,100); // Wait until we receive something

		int count;
		unit_t* ut = m_pReceiver->data->front(&count);

		for (int i=0; i<count; i++)
		{
			if (i>0) 
			{
				int dummy;
				ut = m_pReceiver->data->front(&dummy);
			}
			char* temp = new char[ut->buffer->length];
			memcpy(temp,ut->buffer->buf,ut->buffer->length);
			m_pReceiver->data->pop();									// Free receiving buffer
			dwResult = WaitForSingleObject(m_ExitParseThreadEvent,0);	// Check again - if application stops filter before basic data is available.
			
			if(dwResult == WAIT_TIMEOUT)								// Skip parsing if stop was called.
				m_pParser->transport_stream(temp,ut->buffer->length);
			
			delete[] temp;

			if (first_sample)
			{
				NotifyEvent(EC_MSG_ParserRecvdFirstSample,0,0);
				first_sample=false;
			}
		}

		// Notifications:
		if (!main_data_prsd_notf)
		{
			if (m_pParser->m_all_data_parsed)			
			{
				NotifyEvent(EC_PARSER_COMPLETE,0,0);
				main_data_prsd_notf=true;
			}
		}
		if (!sdt_table_prsd_notf)
		{
			if (m_pParser->m_sdt.complete)			// Aplication can now acces global service list while this loop is still running.
			{
				NotifyEvent(EC_MSG_SERVICE_LIST_RECEIVED,0,0);
				sdt_table_prsd_notf=true;
			}
		}
	}
	while (TRUE);
}

// At the beginning this loop sends samples to initial parser and pushes them to demux fifo.
// After all relavant data is parsed it only pushes sample to demux fifo.
void CPushSourceUdp::ParseAndPush()
{
	m_ExitParseThreadEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;			//  non signaled

	if (m_mode == PARSING_MODE)
	{
		justParse();
		SetEvent(m_DemuxFillingEnded);
		return;
	}
	
	DWORD dwResult=0;
	bool exit=false;
	bool valid_ts=true;
	bool start_notified=false;
	ResetEvent(m_DemuxFillingEnded); 

	do
	{
		dwResult = WaitForSingleObject(m_ExitParseThreadEvent, 0);				// Do we have to exit ?
		if(dwResult != WAIT_TIMEOUT)
		{
			SetEvent(m_DemuxFillingEnded);
			return;
		}
		
		dwResult = WaitForSingleObject(m_pReceiver->data->m_data_rcvd_event,500); // Wait until we receive something.

		if (!start_notified && dwResult != WAIT_TIMEOUT)
		{
            start_notified=true;
	        NotifyEvent(EC_PARSER_STARTED,0,0);
		}

		int count;
		unit_t* ut = m_pReceiver->data->front(&count);

		for (int i=0; i<count; i++)
		{
			if (i>0) 
			{
				int dummy;
				ut = m_pReceiver->data->front(&dummy);
			}
			
			char* temp = new char[ut->buffer->length];						// Copy buffer.
			memcpy(temp,ut->buffer->buf,ut->buffer->length);
			m_pReceiver->data->pop();										// Free receiving buffer.

			valid_ts = m_pParser->transport_stream(temp,ut->buffer->length);
			
			buffer_t* bt_temp = new buffer_t;				// Data is deallocated deleted when demuxed. 
			bt_temp->buf=temp;
			bt_temp->index=0;								// Not needed here.
			bt_temp->length=ut->buffer->length;
			m_pDemuxFifo->push(bt_temp);					// Push data pointers to demux fifo.
			//SetEvent(m_DemuxFifoDataEvent);					// Signal demux thread that we have data in demux fifo.
		}
	}
	while(!m_pParser->m_all_data_parsed && !m_pParser->m_enaugh_data_parsed );

	if(!valid_ts)
	{	
		NotifyEvent(EC_PARSER_NOT_MPEGTS,0,0);
		
		int count;											// Delete everything in fifo and return
		unit_t*	ut = m_pDemuxFifo->front(&count);
		for (int p=0; p<count; p++)
		{
			if (p>0) 
			{
				int dummy;
				ut = m_pDemuxFifo->front(&dummy);
			}
			delete[] ut->buffer->buf;
			delete ut->buffer;
			m_pDemuxFifo->pop();
		}
		SetEvent(m_DemuxFillingEnded);
		return;
	}

	//Detect half done work.
	// Some tables may not be complete. Begin from the start in demux thread.
	if (!m_pParser->m_sdt.complete && m_pParser->m_sdt.first_part_received)
	{
		m_pParser->m_sdt.first_part_received=false;
		m_pParser->m_sdt.section_position=0;
	}
	if (!m_pParser->m_nit.complete && m_pParser->m_nit.first_part_received )
	{
	    m_pParser->m_nit.first_part_received =false;
        m_pParser->m_nit.section_position =0;
	}

	m_dont_leave_multicast=true;							//	Next time application calls stop, filter won't leave multicast group.	
	NotifyEvent(EC_PARSER_COMPLETE,0,0);					// Signal the aplication and continue copying to demux fifo.
	if (m_pParser->global_service_list_parsed)
		NotifyEvent(EC_MSG_SERVICE_LIST_RECEIVED,0,0);

	int count;
	int demux_items=0;
	bool first_push=true;
	LONG brtime =0;
	UINT brdatalen=0;

	do
	{
		dwResult = WaitForSingleObject(m_ExitParseThreadEvent, 0);
		if(dwResult != WAIT_TIMEOUT)
			break;

		dwResult = WaitForSingleObject(m_pReceiver->data->m_data_rcvd_event,500);

		unit_t* ut = m_pReceiver->data->front(&count);


		if (m_source == SOURCE_FILE)						// Limit fifo sizes in FILE mode.
		{
			WaitForSingleObject(m_PushLimitEvent,INFINITE);
			if (m_pDemuxFifo->items() > MAX_DEMUX_FIFO_SIZE_FOR_FILE_SOURCE) 
			{
				SetEvent(m_DemuxFifoDataEvent);				// Signal demux thread that we have (TO  MUCH) data in demux fifo.
				SleepEx(1,TRUE);
				continue;
			}
		}

		for (int i=0; i< count; i++)
		{
			if (i>0)
			{
				int dummy;
				ut = m_pReceiver->data->front(&dummy);
			}
			
			char* temp = new char[ut->buffer->length];
			memcpy(temp,ut->buffer->buf,ut->buffer->length);
			m_pReceiver->data->pop();						// Free receiving buffer
					
			buffer_t* bt_temp = new buffer_t;				// Data is deleted when demuxed
			bt_temp->buf=temp;
			bt_temp->index=0;								// Not needed here
			bt_temp->length=ut->buffer->length;
			brdatalen += bt_temp->length;
			m_pDemuxFifo->push(bt_temp);					// Push data pointers to demux fifo.
		}
			
		if (first_push)
		{
			// Accumulate enaugh data
			if (m_pDemuxFifo->items()>DEMUXER_FIRST_PUSH_BUFFER_COUNT )  // Enaugh data
			{
				SetEvent(m_DemuxFifoDataEvent);				
				first_push = false;
			}	       
			else // Check if bitrate is to low and abandon accumulation.
			{
				if (brtime ==0)
			        brtime = GetTickCount(); // Save time for bitrate meausure.
				else
				{
					int tmp = GetTickCount() - brtime; // Measure bitrate
					if (  tmp != 0 &&  brdatalen/tmp < 64) // Don't acumulate to much for low bitrate channels (radio).
					{
				 		SetEvent(m_DemuxFifoDataEvent);	
						first_push = false;
					}
				}
			}
		}
		else
			SetEvent(m_DemuxFifoDataEvent);					// Signal demux thread that we have data in demux fifo

	}
	while(TRUE);

	SetEvent(m_DemuxFillingEnded);
}


void CPushSourceUdp::Demux()
{
	DWORD dwResult=0;
	m_ExitDemuxThreadEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

    char comp_ts_packet[MAX_PACKET_SIZE*8]; // TS packet builder.
	int ts_split_pos=0;					    // TS packet splice position.
	int buf_len;                            // Buffer length
	int part_packet_pos;                    // Partial packet building variable  1
	int part_packet_stop;                   // Partial packet building variable  2
	ts_structure ts_struct;                 // Structure of ts packets (length offset, timecode, rtp header ...), determined by parser

	bool vid_pes_start_found=false;      // Was video elementary start found
	bool aud_pes_start_found=false;      // Was audio elementary start found
	bool sub_pes_start_found=false;      // Was subtitling elementary start found
	bool exit=false;
	ZeroMemory(comp_ts_packet,MAX_PACKET_SIZE*8);
	ZeroMemory(&m_completeVideoES, 250);
	ZeroMemory(&m_completeAudioES, 250);
	ZeroMemory(&m_completeSubES,250);

	WaitForSingleObject(m_DemuxerConfiguredEvent,INFINITE);	//Wait until demuxer is configured.

	// Check if pins are connected. 
	// This makes function ReportRenderingSuccess(BYTE code) obsolete.
	IPin *pTmp = NULL;
	
	if (m_pPinVideo->ConnectedTo(&pTmp)==S_OK)
	{		
		if (m_stream_has_video)
			m_pPinAudio->m_video_has_data=true;

	    m_video_pin_rendered=true;
	}

	if (pTmp!=NULL)
	{
		 pTmp->Release();
		 pTmp = NULL;
	}
	
	if (m_pPinAudio->ConnectedTo(&pTmp)==S_OK)
	{
	    m_audio_pin_rendered=true;
		m_pPinAudio->m_rendered=true;
	}
	if (pTmp!=NULL)
	{
		 pTmp->Release();
		 pTmp = NULL;
	}

	if (m_pPinSubtitles->ConnectedTo(&pTmp)==S_OK)
	    m_sub_pin_rendered=true;
	
	if (pTmp!=NULL)
	{
		 pTmp->Release();
		 pTmp = NULL;
	}

	//It is now safe to access parser's variables !

	m_completeVideoES.stream_type=m_VpidType;
	m_completeAudioES.stream_type=m_ApidType;
	m_completeSubES.stream_type=PRIVATE_DVB_SUBS;

	ts_struct = m_pParser->m_ts_structure;	
	part_packet_pos=ts_struct.start_ofs;  // Start processing at correct address
	part_packet_stop=0;

	//if (ts_struct.type == TS_OVERRTP)
	//	NotifyEvent(EC_MSG_RTSP_PROTOCOL,0,0);

	#ifdef _DEBUG
		LONGLONG loop_time=0;
	#endif
	do

	{   
        #ifdef _DEBUG
			loop_time = GetTickCount();
        #endif

		dwResult = WaitForSingleObject(m_ExitDemuxThreadEvent, 0); // Do we have to exit?
		if(dwResult != WAIT_TIMEOUT)
		{
			exit=true;										// Process all remaining buffers and exit.
			SetEvent(m_DemuxFifoDataEvent);					// Ensure that event is signaled.
		}

		dwResult = WaitForSingleObject(m_DemuxFifoDataEvent,INFINITE);

		int count;
		unit_t* ut = m_pDemuxFifo->front(&count);
		
		for (int p=0; p<count; p++)
		{
			if (p>0) 
			{
				int dummy;
				ut = m_pDemuxFifo->front(&dummy);
			}
			
			buf_len=ut->buffer->length;

	        int t = 0; // support for packets with 4 byte timecode header
			if ( ts_struct.type == TS_TIMECODE)
				t=4;

        	// This will only happen if source is a file. In Net source, packets are aligned based on MTU.
			// We can't just choose appropriate buffer for a file source because of a RTP with variable length support
			// *******************************************************************************************
			if (  part_packet_stop != 0 ) 
			{
				if (ts_struct.type != TS_OVERRTP)
				{
					memcpy(&comp_ts_packet[part_packet_pos],ut->buffer->buf,part_packet_stop); // Copy reminding part
					processPacket(comp_ts_packet,t,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
					part_packet_pos = part_packet_stop; //Align
				}
				else
				{
					//RTP can either start with rtp header or ts
					if ( part_packet_stop == 0x47 )
					{
						// 
						memcpy(&comp_ts_packet[part_packet_pos],ut->buffer->buf,ts_struct.packet_len); // Copy reminding part of a ts packet
						processPacket(comp_ts_packet,0,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
						part_packet_pos = ts_struct.packet_len; //Align			
					}
					else
					{
						rtp_header rhdr;
						m_pParser->parse_rtp(comp_ts_packet,PART_PACKET_SIZE,&rhdr);
						int i = rhdr.header_length;
						processPacket(&comp_ts_packet[i],0,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
						part_packet_pos = i + ts_struct.packet_len; //Align	
					}
				}
			}
			// ****************************************************************************************************************************

			int j = part_packet_stop+t; // packet start + timecodeoffset

			if ( ts_struct.type == TS_STANDARD || ts_struct.type == TS_TIMECODE )
			{
				int ps=0; //Packet start (processed)

				for( ; j<buf_len; j+=ts_struct.packet_len+t)		
				{
					do// Make sure that there is nothing in between the packets.
					{
						if (ut->buffer->buf[j] == 0x47) 
						{
							ps=j;
							if (j + ts_struct.packet_len<= buf_len )
					            processPacket(ut->buffer->buf,j,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
							break;
						}
						else
							j+=1;
					}while (j<buf_len);
				}
				if ( buf_len-ps >= ts_struct.packet_len  ) // Full packet with or without any additional header.
				{
					part_packet_pos=0;
					part_packet_stop=0;
				}
				else
					part_packet_pos = (buf_len - ps ) +t ;
				
				j = ps;
			}
			else  // rtp packets
			{
				// RTP payload is not constant. It must only have an integer number of ts packets.
				// Header length may not be constant, so it must be allways parsed.
				// If there is an error in transmission, RTP will suffer the most. TODO: Create recovery mechanism for RTP !!!!
				rtp_header rhdr;
				int h =0; // Position of last processed ts packet
				int tmp;

				// buffer with offset has either ts or rtp start
				if (ut->buffer->buf[j] != 0x47)
				{
					m_pParser->parse_rtp(&ut->buffer->buf[j],buf_len-j,&rhdr);
					j+= rhdr.header_length; //Jump to ts start
				}

				h=j;
				j += ts_struct.packet_len; // Jump to next ts or next rtp start. Buffer will never be smaller than ts length.
				for ( ; j<buf_len; j+=ts_struct.packet_len)
				{
					if ( ut->buffer->buf[j] == 0x47)
					{
						// Next packet is also ts.
						// Current packet is at h position
						processPacket(ut->buffer->buf,h,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
					}
					else
					{
						// Next packet is a rtp.
						processPacket(ut->buffer->buf,h,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
						if ( j+12 < buf_len)// File source can have a splice in the middle of the rtp header
						    m_pParser->parse_rtp(&ut->buffer->buf[j],buf_len-j,&rhdr); // Parse rtp header
						else  
							break;
						tmp = j+rhdr.header_length;
						if ( tmp<buf_len) // Skip for a rtp headerlength.
						   j =tmp; 
						else
						   break;
					}
					h=j;
				}
				// Remaining buffer will start with a rtp o ts. Copy it to a temp buffer and process it on next call
				j= h+ ts_struct.packet_len; // move to the end of last ts packet +1		
				if ( (part_packet_pos = buf_len -j)==0)
					processPacket(ut->buffer->buf,h,&vid_pes_start_found,&aud_pes_start_found,&sub_pes_start_found);
			}
			
			// This will only happen if source is a file. In Net source, packets are aligned based on MTU. *************************************************
			if ( part_packet_pos != 0)
			{
				memcpy(comp_ts_packet,&ut->buffer->buf[j-t],part_packet_pos); // Copy partial packet with timecode to temp buffer
 				if (ts_struct.type == TS_OVERRTP)
					part_packet_stop=1; // no real meaning
				else
					//part_packet_stop=ts_struct.rtphdr_len + (ts_struct.packets_in_mtu*ts_struct.packet_len) -part_packet_pos;// How many bytes of reminding packet with timecode is in next buffer.
			        part_packet_stop=ts_struct.packet_len+t -part_packet_pos ;
			}
			//************************************************************************************************************************************************

			delete[] ut->buffer->buf;
			delete ut->buffer;
			m_pDemuxFifo->pop();
		}

		if (exit)
			break;

		ResetEvent(m_DemuxFifoDataEvent);

		#ifdef _DEBUG
			loop_time = GetTickCount()-loop_time;
         #endif

	}while(TRUE);
}

//Processes one TS packet !
void CPushSourceUdp::processPacket(char* buf, int i, bool* vid_pes_start_found, bool* aud_pes_start_found ,bool* sub_pes_start_found)
{	
	u_short pid = ((buf[i+1] & 0x1f) << 8) | (BYTE)buf[i+2];

	u_short selected_subtitle_pid = m_Spid;
	u_short selected_primarypin_audio_pid = m_Apid;
	bool resetaudioES = m_resetAudES;
	bool resetvideoES = m_resetVidES;
	bool resetsubES = m_resetSubES;
	
	// bitrate******************************************************
	{
	    CAutoLock cAutoLock(&m_pLock);

		// Measure bitrate
		if (m_flags & DEMUX_FLAGS_MEASURE_BITRATE) // Measure bit rate of complete TS at the Demuxer input
		{
			ULONG tmp; 
			if ( (tmp=GetTickCount() -  m_bitratetimer) <1000)
			{
				 m_packetcounter+=1;
			}
			else
			{
				m_tsbitrate = m_packetcounter*(UINT)m_pParser->m_ts_structure.packet_len*1000 / (tmp) ; 
				m_bitratetimer = GetTickCount();
				m_packetcounter=0;
			}
		}

		if (m_flags & DEMUX_FLAGS_MEASURE_PID_BITRATE) // Measure bit rate for every possible PID
		{
			ULONGLONG tmp;
			std::vector<pidbitrate_t>::iterator it = std::find_if(m_PidSet.begin(), m_PidSet.end(),find_pid(pid));
			if (it != m_PidSet.end())
			{
				if ((tmp=GetTickCount() -  it->bitratetimer) <1000)
				{
					it->packetcounter+=1;
				}
				else
				{
					it->tsbitrate = it->packetcounter*(UINT)m_pParser->m_ts_structure.packet_len*1000 / (tmp) ; 
					it->bitratetimer = GetTickCount();
					it->packetcounter=0;			    
				}
			}
			else
			{
			    pidbitrate_t p;
				p.bitratetimer =GetTickCount();
				p.packetcounter = 0;
				p.tsbitrate =0;
				p.PID = pid;
				m_PidSet.push_back(p);
			}
		}
	}
	// ***************************************************************

	if (pid==0)
	{
	
	}

	else if(m_video_pin_rendered && pid==m_Vpid)
	{	
		if (resetvideoES)
		{
			*vid_pes_start_found=false;
			m_completeVideoES.len=0;
			CAutoLock cAutoLock(&m_pLock);
			m_resetVidES =false;
		}
		extractPayload(buf, i, &m_completeVideoES, m_pVideoFifo,vid_pes_start_found);
	}
	else if (m_audio_pin_rendered && pid==selected_primarypin_audio_pid)
	{
		if (resetaudioES)
		{
			*aud_pes_start_found=false;
			m_completeAudioES.len=0;
			CAutoLock cAutoLock(&m_pLock);
			m_resetAudES =false;
		}
		extractPayload(buf, i, &m_completeAudioES, m_pAudioFifo, aud_pes_start_found);
	}
	else if( (m_sub_pin_rendered || m_subtitle_polling) && pid==selected_subtitle_pid)
	{
		if (resetsubES)
		{
			*sub_pes_start_found=false;
			m_completeSubES.len=0;
			CAutoLock cAutoLock(&m_pLock);
			m_resetSubES =false;
		}		
		extractPayload(buf, i, &m_completeSubES, m_pSubsFifo, sub_pes_start_found);
	}

	else if ( pid == m_pParser->m_nit_pid && ! m_pParser->m_nit.complete)
	{
		m_pParser->parse_psi(buf,i,TABLE_NIT);
		if ( m_pParser->m_nit.complete)
				NotifyEvent(EC_MSG_NIT_TABLE_PARSED,0,0);
	}

	else if (pid == 0x11) //can be: SDT,BAT,ST
	{
		if (!m_pParser->m_sdt.complete)
		{
			m_pParser->parse_psi(buf, i, TABLE_SDT);
			if (m_pParser->global_service_list_parsed)
				NotifyEvent(EC_MSG_SERVICE_LIST_RECEIVED,0,0);
		}
	}

	else if (pid == 0x12) //can be: EIT,ST,CIT
	{
		    // Not finished- need samples with EIT !!!

			//m_pParser->parse_psi(buf, i, TABLE_EIT);
			//
			//if (m_pParser->m_dmt_eit.complete)
			//{
			//	if (! m_pParser->m_dmt_eit.first_table_received)
			//	{
   //                 m_pParser->m_dmt_eit.first_table_received=true;
			//		NotifyEvent(EC_MSG_EIT_TABLE_PARSED,0,0); // First table received
			//	}
			//	
			//	else if ( m_pParser->m_dmt_eit.new_complete)
			//		NotifyEvent(EC_MSG_EIT_TABLE_CHANGED,0,0); // Table has changed
			//}
	}

	else if (m_flags & DEMUX_FLAGS_MONITOR_SELECTED_PMT && pid == m_PMT_PID) // Monitor PAT for changes. This happens very rarely !
	{		
		// NOTE: Don't forget to lock when you add related function called by application! (GetNewPMT list() or similar)
		//CAutoLock cAutoLock(&m_pLock); // Lock, because application can also access m_pParser->m_dmt_last_PMT_of_curr_prog vector!
	    m_pParser->parse_psi(buf, i, TABLE_PMT);

		if (m_pParser->m_pmts.new_complete)
		{
			// Check if PID of currently selected audio, video, or subtitles has changed.
			// Changing video PID not supported, because new PID may have different format.

			// If currently selected PIDs are not in new table, select first available audio or subtitle track.
			int curpmtsize = m_pParser->m_dmt_last_PMT_of_curr_prog.size();
			bool vidPidSame=false;
			bool audPidSame=false;
			bool subPidSame=false;
			for (int i=0; i< curpmtsize; i++)
			{
			    if (m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid == m_Vpid && m_pParser->m_dmt_last_PMT_of_curr_prog[i].stream_type == m_VpidType)
					vidPidSame=true;
			    if (m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid == m_Apid && m_pParser->m_dmt_last_PMT_of_curr_prog[i].stream_type == m_ApidType)
					audPidSame=true;
				if (m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid == m_Spid && m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v.size()>0)
					subPidSame=true;
			}

			if (!vidPidSame)
			{
				for (int i=0; i< curpmtsize; i++)
			    {
					if (m_pParser->m_dmt_last_PMT_of_curr_prog[i].stream_type == m_VpidType)
					{
						m_Vpid = m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid;
                        m_resetVidES=true;
						break;
					}
				}
			}

			if (!audPidSame)
			{
				for (int i=0; i< curpmtsize; i++)
			    {
					if ( m_pParser->m_dmt_last_PMT_of_curr_prog[i].stream_type == m_ApidType) // Don't care about language
					{
						setPrimaryAudioPinMediaType(&m_pParser->m_dmt_last_PMT_of_curr_prog[i],m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid); 
						m_resetAudES=true;
						break;
					}
				}
			}

			if (!subPidSame)
			{
				for (int i=0; i< curpmtsize; i++) 
			    {
					//NOTE: Change this if you are going to add support for teletext subtitles!
					if ( m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v.size()>0 
						&& (m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].type>=0x10 && 
						m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].type<=0x14)		
						|| (m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].type>=0x20 &&
						 m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].type>=0x24)
						) 
					{
						  m_Spid = m_pParser->m_dmt_last_PMT_of_curr_prog[i].pid;
					      m_pPinSubtitles->m_page_id =m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].composition_page_id;
		                  m_pPinSubtitles->m_ancillary_id = m_pParser->m_dmt_last_PMT_of_curr_prog[i].subtitling_descriptors_v[0].ancillary_page_id;
			        }

			//Notify application. Application will then select appropriate track.
			NotifyEvent(EC_MSG_CURRENT_PMT_CHANGED,0,0); 	
		       }
			}
		}// end pmts_complete
	}// end PMT_PID
}

// Extract payload and push it's pointer to appropriate fifo.
// If payload size is bigger than ts packet, continue filling ful_es buffer.
// If payload size indicator is 0, continue filling es_buffer until next start code.
void CPushSourceUdp::extractPayload(char *buffer, int ts_start, payload_t* ful_es, CFifoES* fifo_es, bool* start_found) 
{
	HRESULT hr;
	pes_basic_info_t pesbasic;
	ZeroMemory(&pesbasic,sizeof(pes_basic_info_t));
	int ts_end=ts_start+m_pParser->m_ts_structure.packet_len;

	// Send TS packet to parser, so we can get some basic info like PTS.
	hr = m_pParser->parse_pes(buffer,ts_start,-1,-1,&pesbasic,0); 
	
	if (hr==E_FAIL)
		return;

	if (!pesbasic.unit_start)
	{
		if (!(*start_found))
			return;

		// Discontinuity check:
		// If discontinuity is detected we drop the packet and reset es builder pointer to zero.
		u_short cc = m_pParser->continuity_count(buffer,ts_start);
		u_short prev_cc=ful_es->last_continuity_count;
		int cc_diff = abs(cc - prev_cc);
		ful_es->last_continuity_count = m_pParser->continuity_count(buffer,ts_start);	//store continuity counter
		if (cc_diff != 1 && cc_diff != 15)
		{
			ful_es->discontinuity=true;
			ful_es->len=0;
			*start_found=false;
			NotifyEvent(EC_MSG_Discontinuity,(LONG_PTR)cc,(LONG_PTR)prev_cc);
			return;
		}

		// Continue filling ES packet
		memcpy(&(ful_es->buffer[ful_es->len]),&(buffer[pesbasic.frame_start]),ts_end-pesbasic.frame_start);
		ful_es->len+=ts_end-pesbasic.frame_start;
       
		if(ful_es->indicated_len!=0 && (int)(ful_es->indicated_len) == ful_es->len)		//check for size
			pushFullESPacket(ful_es,fifo_es);			

	}
	else
	{
		if(!(*start_found))
		{
			if (!pesbasic.has_PTS)														// first ES MUST have PTS
				return;
			*start_found=true;
		}
		
		ful_es->last_continuity_count = m_pParser->continuity_count(buffer,ts_start);   // Store continuity counter

		if (pesbasic.es_length!=0)
			ful_es->indicated_len=pesbasic.es_length;

		if (ful_es->len>0)// Process previous packet if exists
		{
			if (ful_es->len > MAX_ES_LEN)	//something went wrong
			{
				ful_es->len=0;
				ful_es->discontinuity=false;
				ful_es->has_pts=false;
				ful_es->indicated_len = 0;
				ful_es->present_time = 0;
			}
			else
			    pushFullESPacket(ful_es,fifo_es);
		}

		// START filling ES packet =========
		memcpy(&(ful_es->buffer[0]),&(buffer[pesbasic.frame_start]),ts_end-pesbasic.frame_start);
		ful_es->len=ts_end-pesbasic.frame_start;
		ful_es->present_time=pesbasic.PTS;
		ful_es->has_pts =pesbasic.has_PTS;
		// =================================

		if(ful_es->indicated_len!=0 && (int)(ful_es->indicated_len) == ful_es->len)		//check for size
			pushFullESPacket(ful_es,fifo_es);	
	}
}

void CPushSourceUdp::pushFullESPacket(CPushSourceUdp::payload_t *ful_es, CFifoES* fifo_es)
{
	    bool discard_sample=false;
		LONGLONG delay=0;

		if (m_source==SOURCE_NET && fifo_es->items() > MAX_ES_FIFO_SIZE)
		{
			goto reset;
			//discard_sample =true;
		}

		char* ES_buffer = &(ful_es->buffer[0]);
		int ES_buffer_len = ful_es->len;
		LONGLONG ES_present_time = ful_es->present_time;
		bool has_pts = ful_es->has_pts;
		bool ES_dicontuinity = ful_es->discontinuity;
		u_short ES_stream_type = ful_es->stream_type;
		
		bool ES_starts_with_i_frame =false;
		UINT ES_video_frames_arx = 1;
		UINT ES_video_frames_ary =1;
		UINT ES_video_frames_width= 0;
		UINT ES_video_frames_height =0;
        UINT ES_video_frames=0;
		UINT ES_video_i_frame_pos=0;
		bool ES_has_i_frame = false;
		BYTE ES_afd = 0;
		bool ES_pid_format_change=false;

		//*** Special handling for h.264:
		if(ful_es->stream_type == 0x01b)					// Get additional data for h.264 video
		{
			// Reset some of the previous values:
			m_h264_ext_info.ext_video_key_frame_found=false;
			m_h264_ext_info.h264_is_idr=false;
			m_h264_ext_info.h264_access_unit_count=0;

			m_pParser->h264_video(ful_es->buffer,ful_es->len,&m_h264_ext_info);
			ES_video_frames=m_h264_ext_info.h264_access_unit_count;
			ES_video_i_frame_pos=m_h264_ext_info.ext_video_key_frame_pos;
			ES_has_i_frame= m_h264_ext_info.ext_video_key_frame_found;
			if (ES_has_i_frame && ES_video_i_frame_pos<2)
				ES_starts_with_i_frame = true;
			ES_video_frames_arx = m_h264_ext_info.video_arx; 
			ES_video_frames_ary	= m_h264_ext_info.video_ary;
			ES_video_frames_width=m_h264_ext_info.video_width;
			ES_video_frames_height=m_h264_ext_info.video_height;
			ES_afd = m_h264_ext_info.afd;

			// IMORTANT: If first video sample has not an i-slice or idr frame, we can't synhronyse audio and video !
			if (m_first_vid_pts == -1 && !ES_has_i_frame)		// i-slice actually
				discard_sample = true;

			if (!discard_sample)
			{
			    if (m_first_vid_pts == -1)	// store first pts
				{					
					m_first_vid_pts = ES_present_time;
					m_pPinSubtitles->SetStartPts(m_first_vid_pts,0);
					m_pPinAudio->SetVideoStartPts(m_first_vid_pts);
				}

				char* esdata = new char[ES_buffer_len];
				memcpy(esdata,ES_buffer,ES_buffer_len);
				bufferES_t* esbuf = new bufferES_t;
			
				esbuf->discontinuity=ES_dicontuinity;
				esbuf->has_i_frame=ES_has_i_frame;
				esbuf->starts_with_i_frame=ES_starts_with_i_frame;
				esbuf->video_arx=ES_video_frames_arx; 
				esbuf->video_ary=ES_video_frames_ary;	
				esbuf->video_width= ES_video_frames_width;
				esbuf->video_height= ES_video_frames_height;
				esbuf->stream_type = ES_stream_type;
				esbuf->present_time=ES_present_time;
				esbuf->has_pts = has_pts;
				esbuf->buf = esdata;
				esbuf->length=ES_buffer_len;
				esbuf->video_frames=ES_video_frames;
				esbuf->video_i_frame_pos =  ES_video_i_frame_pos;
				esbuf->video_afd=ES_afd;

				//if (m_source==SOURCE_FILE)
				//{
				//	if (fifo_es->items()>MAX_ES_FIFO_SIZE_FOR_FILE_SOURCE)
				//		ResetEvent(m_PushLimitEvent);
				//}

				fifo_es->push(esbuf);
			}
		}
		//*** Common for all other types.
		else 
		{
			if ((ful_es->stream_type == 0x001 ||ful_es->stream_type == 0x02 ) )	// Get additional info for mpeg2 video
			{
				//reset some of the previous values
				m_mpeg2_ext_info.sample_fields=0;
				m_mpeg2_ext_info.is_key_frame=false;
				m_mpeg2_ext_info.key_frame_pos=0;

				m_pParser->mpeg_video(ful_es->buffer,ful_es->len, &m_mpeg2_ext_info);
				ES_video_frames = m_mpeg2_ext_info.sample_fields;
				ES_has_i_frame= m_mpeg2_ext_info.is_key_frame;
				ES_video_i_frame_pos = m_mpeg2_ext_info.key_frame_pos;
				if (ES_has_i_frame && ES_video_i_frame_pos<2)
					ES_starts_with_i_frame = true;
				ES_video_frames_arx = m_mpeg2_ext_info.video_arx;
				ES_video_frames_ary = m_mpeg2_ext_info.video_ary;
				ES_video_frames_width = m_mpeg2_ext_info.video_width;
				ES_video_frames_height = m_mpeg2_ext_info.video_height;
				ES_afd = m_mpeg2_ext_info.afd;
				
				if (m_first_vid_pts == -1)
				{
					m_first_vid_pts = ES_present_time;					// Store first video pts
					m_pPinSubtitles->SetStartPts(m_first_vid_pts,0);
					m_pPinAudio->SetVideoStartPts(m_first_vid_pts);
				}
			}
			

			// Discard audio packets with PTS earlyer than video ===========
			 if (ful_es->stream_type == 0x003 || ful_es->stream_type == 0x004 || ful_es->stream_type == 0x081 || ful_es->stream_type ==0x11 )
			{
				if (m_stream_has_video)	
				{
					if (m_first_vid_pts==-1) // Make sure that first processed ES is from video
						discard_sample =true;
					else
					{
						 if (m_first_aud_pts == -1 )
						{
							LONGLONG avdiff = ES_present_time-m_first_vid_pts;
							if (avdiff<1 || avdiff > 8000000000 )
								discard_sample=true;
							else
								m_first_aud_pts= ES_present_time;		//Store first audio pts that will be sent to decoder
    					}
					}
				}
				if (m_audio_pid_format_change)
				{
					ES_pid_format_change=true;
				    m_audio_pid_format_change=false;
				}
			}
			 // =========================================================

			 // Discard subtitles with PTS earlear than video ============
			// if (ful_es->stream_type == PRIVATE_DVB_SUBS)						
			//{
			//	if (m_stream_has_video)
			//	{
			//		if (m_first_vid_pts==-1)
			//			discard_sample =true;
			//		else
			//		{
			//			 if (m_first_sub_pts == -1 )
			//			{
			//				LONGLONG avdiff = ES_present_time-m_first_vid_pts;
			//				if (avdiff<1 || avdiff > 8000000000 )
			//					discard_sample=true;
			//				else
			//					m_first_sub_pts= ES_present_time;	// Store first audio pts that will be sent to decoder.
   // 					}
			//		}
			//	}
			//}
			 //=============================================================

			if (!discard_sample)
			{
					
				bufferES_t* esbuf = new bufferES_t;
				char* esdata = new char[ES_buffer_len];
				esbuf->buf=esdata;
				esbuf->length=ES_buffer_len;
				esbuf->present_time=ES_present_time;
				esbuf->has_pts = has_pts;
				esbuf->discontinuity=ES_dicontuinity;
				esbuf->starts_with_i_frame = ES_starts_with_i_frame;
				esbuf->has_i_frame=ES_has_i_frame;
				esbuf->video_arx=ES_video_frames_arx; 
				esbuf->video_ary=ES_video_frames_ary;
				esbuf->video_width= ES_video_frames_width;
				esbuf->video_height= ES_video_frames_height;
				esbuf->stream_type = ES_stream_type;
				esbuf->video_frames=ES_video_frames;
				esbuf->video_afd=ES_afd;
				esbuf->pid_format_change=ES_pid_format_change;
				memcpy(esdata,ES_buffer,ES_buffer_len);

				//if (m_source==SOURCE_FILE)
				//{
				//	if (fifo_es->items()>MAX_ES_FIFO_SIZE_FOR_FILE_SOURCE)
				//		ResetEvent(m_PushLimitEvent);
				//}

				fifo_es->push(esbuf);
			}
		}
		
        reset:

		// Reset es builder values:
		ful_es->len=0;
		ful_es->discontinuity=false;
		ful_es->has_pts=false;
		ful_es->indicated_len = 0;
		ful_es->present_time = 0;
}


//STDMETHODIMP CPushSourceUdp::ConfigureParserManualVideo(AM_MEDIA_TYPE * mt )
//{
//	CAutoLock cAutoLock(&m_pLock);
//	
//	ZeroMemory(&m_MediaTypeVideo, sizeof(AM_MEDIA_TYPE));
//	FreeMediaType(m_MediaTypeVideo);
//
//	m_MediaTypeVideo = *mt;
//
//
//	return S_OK;
//}
//
//
//STDMETHODIMP CPushSourceUdp::ConfigureParserManualAudio(AM_MEDIA_TYPE * mt )
//{
//	CAutoLock cAutoLock(&m_pLock);
//
//   	ZeroMemory(&m_MediaTypeAudio, sizeof(AM_MEDIA_TYPE));
//	FreeMediaType(m_MediaTypeAudio);
//
//	m_MediaTypeAudio = *mt;
//
//	//set pcr pid
//	piddetails_t* video_det = m_pParser->get_details(m_Vpid);
//	if(video_det!=NULL)
//	{
//		m_PCRpid=video_det->PCR_PID;
//	}
//
//	return S_OK;
//}

// Note: MediaType must be set only once
STDMETHODIMP CPushSourceUdp::ConfigureDemultiplexer(u_short video_pid, u_short audio_pid,u_short subtitle_pid, BYTE flags)
{
	//CAutoLock cAutoLock(&m_pLock);
	m_flags = flags;

	if (m_MediaTypeVideo.majortype!=GUID_NULL)
		FreeMediaType(m_MediaTypeVideo);

	ZeroMemory(&m_MediaTypeVideo, sizeof(AM_MEDIA_TYPE));
	m_MediaTypeVideo.majortype = MEDIATYPE_Video;
	
	// =video=:
	piddetails_t* video_det = m_pParser->get_details(video_pid);
	if (video_det!=NULL)
	{
		// demuxer ========================
		m_Vpid=video_pid;
		m_VpidType=video_det->stream_type;
		m_stream_has_video=true;
		// ================================

		// parser =========================
		m_PMT_PID= m_pParser->setCurrentStream(m_Vpid,VIDEO_STREAM);	// set selected video prog index and stream index
		// ================================

		// video pin ======================
		m_pPinVideo->m_time_per_field.nominator = video_det->pes_vdet.timeperfieldNom;
		m_pPinVideo->m_time_per_field.denominator = video_det->pes_vdet.timeperfieldDenom;
		m_pPinVideo->m_arx =video_det->pes_vdet.aspect_x;
		m_pPinVideo->m_ary =video_det->pes_vdet.aspect_y;
		m_pPinVideo->m_width = video_det->pes_vdet.width;
		m_pPinVideo->m_height =  video_det->pes_vdet.height;

		if(m_PCRpid!=0)
			m_PCRpid=video_det->PCR_PID;

		if (video_det->stream_type == 0x01 || video_det->stream_type==0x02)
		{
			m_MediaTypeVideo.subtype=MEDIASUBTYPE_MPEG2_VIDEO;
			m_MediaTypeVideo.formattype=FORMAT_MPEG2Video;
			m_MediaTypeVideo.bTemporalCompression=false;
			m_MediaTypeVideo.bFixedSizeSamples=false;

			m_MediaTypeVideo.cbFormat = sizeof(MPEG2VIDEOINFO) ;
			m_MediaTypeVideo.pbFormat = (BYTE*)CoTaskMemAlloc(m_MediaTypeVideo.cbFormat);
			if (m_MediaTypeVideo.pbFormat == NULL)
			{
				// Out of memory; 
			}
			ZeroMemory(m_MediaTypeVideo.pbFormat, m_MediaTypeVideo.cbFormat);
			MPEG2VIDEOINFO *pM2VIH = (MPEG2VIDEOINFO*)m_MediaTypeVideo.pbFormat;
			pM2VIH->hdr.AvgTimePerFrame=video_det->pes_vdet.timeperfieldNom *2/ video_det->pes_vdet.timeperfieldDenom; 
			pM2VIH->hdr.dwPictAspectRatioX=video_det->pes_vdet.aspect_x;
			pM2VIH->hdr.dwPictAspectRatioY=video_det->pes_vdet.aspect_y;
			pM2VIH->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pM2VIH->hdr.bmiHeader.biWidth=video_det->pes_vdet.width;
			pM2VIH->hdr.bmiHeader.biHeight=video_det->pes_vdet.height;
			pM2VIH->hdr.bmiHeader.biCompression = *(DWORD*)"MPG2";
			pM2VIH->dwLevel=m_pParser->toDsMpeg2Level(video_det->pes_vdet.level);
			pM2VIH->dwProfile=m_pParser->toDsMpeg2Profile(video_det->pes_vdet.profile);
			if (video_det->pes_vdet.interlaced_indicator==2) // field 1st
				pM2VIH->hdr.dwInterlaceFlags = AMINTERLACE_IsInterlaced |AMINTERLACE_Field1First |AMINTERLACE_FieldPatBothRegular |AMINTERLACE_DisplayModeBobOrWeave;
			else if (video_det->pes_vdet.interlaced_indicator==3) // field 2 1st
				pM2VIH->hdr.dwInterlaceFlags = AMINTERLACE_IsInterlaced |AMINTERLACE_FieldPatBothRegular |AMINTERLACE_DisplayModeBobOrWeave;
			//else
			//	pM2VIH->hdr.dwInterlaceFlags = 0 ;

			//clean
		}
		else if (video_det->stream_type == 0x1b)
		{

		    m_MediaTypeVideo.subtype = MEDIASUBTYPE_H264 ;
            m_MediaTypeVideo.formattype = FORMAT_VideoInfo2 ;
		    m_MediaTypeVideo.bFixedSizeSamples=false;
		    m_MediaTypeVideo.bTemporalCompression=true;
			m_MediaTypeVideo.cbFormat = sizeof(VIDEOINFOHEADER2) ;
			m_MediaTypeVideo.pbFormat = (BYTE*)CoTaskMemAlloc(m_MediaTypeVideo.cbFormat);
			if (m_MediaTypeVideo.pbFormat == NULL)
			{
				// Out of memory; return an error code.
			}
			ZeroMemory(m_MediaTypeVideo.pbFormat, m_MediaTypeVideo.cbFormat);

			VIDEOINFOHEADER2 *pMVIH = (VIDEOINFOHEADER2*)m_MediaTypeVideo.pbFormat;
			if (video_det->pes_vdet.aspect_y!=0)
			{
				UINT arx = video_det->pes_vdet.width * video_det->pes_vdet.aspect_x / video_det->pes_vdet.aspect_y;
				UINT ary = video_det->pes_vdet.height;
				if (arx!=0 && ary!=0)
					ReduceFraction(arx,ary);
				pMVIH->dwPictAspectRatioX=arx;
				pMVIH->dwPictAspectRatioY=ary;
			}
			else
			{
				pMVIH->dwPictAspectRatioX=video_det->pes_vdet.aspect_x;
				pMVIH->dwPictAspectRatioY=video_det->pes_vdet.aspect_y;
			}
			
			pMVIH->AvgTimePerFrame=video_det->pes_vdet.timeperfieldNom*2 / video_det->pes_vdet.timeperfieldDenom;
			if (video_det->pes_vdet.interlaced_indicator==2) // field 1 1st
				pMVIH->dwInterlaceFlags = AMINTERLACE_IsInterlaced |AMINTERLACE_Field1First |AMINTERLACE_FieldPatBothRegular |AMINTERLACE_DisplayModeBobOrWeave;
			else if (video_det->pes_vdet.interlaced_indicator==3) // field 2 1st
				pMVIH->dwInterlaceFlags = AMINTERLACE_IsInterlaced |AMINTERLACE_FieldPatBothRegular |AMINTERLACE_DisplayModeBobOrWeave;
			//else
			//	pMVIH->dwInterlaceFlags = 0 ;

			pMVIH->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pMVIH->bmiHeader.biCompression = *(DWORD*)"h264";
			pMVIH->bmiHeader.biWidth=video_det->pes_vdet.width;
			pMVIH->bmiHeader.biHeight=video_det->pes_vdet.height;
			
			//clean
		}
		// ================================
	}

	// =audio=:
	piddetails_t* audio_det = m_pParser->get_details(audio_pid);
	if (audio_det != NULL)
	{
         setPrimaryAudioPinMediaType(audio_det,audio_pid);
		 if (video_det==NULL)
			 m_PMT_PID=m_pParser->setCurrentStream(m_Apid,AUDIO_STREAM);	// Set programme index based on audio pid
	}

	// =subtitles=:
	SetSubtitlesPid(subtitle_pid);
	// set aspect ratio for vmr-9 mixer
	if (video_det!=NULL)
	{
		UINT arx=0;
		UINT ary =0;
		LONG vid_height=video_det->pes_vdet.height;
		LONG vid_width=video_det->pes_vdet.width;
		BYTE colorspace = 0;
		RECT src, trg;
		SetRectEmpty(&src);
        SetRectEmpty(&trg);

		switch (video_det->stream_type) 
		{
			case 0x1:
			case 0x2:

				arx = video_det->pes_vdet.aspect_x;
				ary = video_det->pes_vdet.aspect_y;

				break;
			case 0x1b:
				// Get ar from par and dimensions
						
				if (video_det->pes_vdet.aspect_y!=0)
					arx = video_det->pes_vdet.width * video_det->pes_vdet.aspect_x / video_det->pes_vdet.aspect_y;
				ary = video_det->pes_vdet.height;
			
				if (arx!=0 && ary!=0)
					ReduceFraction(arx,ary);
				
           		break;
			default:
				break;
		}

	    if (flags & DEMUX_FLAGS_AYUV_SUBTITLES)
		    colorspace = COLORSPACE_AYUV;

		// Set media type. Renderer might choose different dimensions and ar in CheckMediaType(). Changes are signaled to dvb sub decoder in that function. 
		m_pPinSubtitles->SetMediaTypeParams(colorspace,0,0,src,trg,arx,ary);
		//m_pPinSubtitles->SetMediaTypeParams(colorspace,vid_width,vid_height,src,trg,arx,ary);
	}

	return S_OK;
}

HRESULT CPushSourceUdp::setPrimaryAudioPinMediaType(piddetails_t* audio_det, u_short audio_pid)
{
	//TODO: cotaskmemfree

	    HRESULT hr =  AUDIO_PID_CHANGE_NO_CHANGES_NEEDED;
	   
		GUID prevSubtype = GUID_NULL;
		WORD prevChanNum = 0;
		DWORD prevSampFreq=0;
		
	   if (m_MediaTypeAudio.majortype != GUID_NULL)
	   {
		    prevSubtype = m_MediaTypeAudio.subtype;
			prevChanNum = ((MPEG1WAVEFORMAT*)m_MediaTypeAudio.pbFormat)->wfx.nChannels;
            prevSampFreq = ((MPEG1WAVEFORMAT*)m_MediaTypeAudio.pbFormat)->wfx.nSamplesPerSec;
		    FreeMediaType(m_MediaTypeAudio);
	   }

		ZeroMemory(&m_MediaTypeAudio, sizeof(AM_MEDIA_TYPE));
		m_MediaTypeAudio.majortype = MEDIATYPE_Audio;

	    // demuxer ========================
		m_Apid = audio_pid;
		if (audio_det->has_data)		// Check if stream actually contains any audio packets. Audio may be listed in PMT, but audio packets may not be present.
			m_stream_has_audio=true;
		else
			m_stream_has_audio=false;

		m_ApidType=audio_det->stream_type;
		// ================================

		// audio pin ======================
		//TODO track changes for audio
		//m_pPinAudio->m_channels = audio_det->pes_adet.channels;
		//m_pPinAudio->m_samp_freq = audio_det->pes_adet.sampfreq;

		if(m_PCRpid!=0)
			m_PCRpid=audio_det->PCR_PID;

		if(audio_det->stream_type == 0x03 || audio_det->stream_type == 0x04 || audio_det->stream_type == 0x81 || audio_det->stream_type ==0x11)
		{
			m_MediaTypeAudio.majortype = MEDIATYPE_Audio ;
			if (audio_det->stream_type == 0x04)
				m_MediaTypeAudio.subtype = MEDIASUBTYPE_MPEG2_AUDIO;
			else if (audio_det->stream_type== 0x03)
				m_MediaTypeAudio.subtype =MEDIASUBTYPE_MPEG1AudioPayload;
			else if (audio_det->stream_type == 0x81)
				m_MediaTypeAudio.subtype = MEDIASUBTYPE_DOLBY_AC3;
			else if (audio_det->stream_type == 0x11)
				m_MediaTypeAudio.subtype=MEDIASUBTYPE_LATM_AAC;		

			m_MediaTypeAudio.formattype = FORMAT_WaveFormatEx;//FORMAT_WaveFormatEx;
			m_MediaTypeAudio.cbFormat = sizeof(WAVEFORMATEX) + sizeof(MPEG1WAVEFORMAT);
			m_MediaTypeAudio.pbFormat = (BYTE*)CoTaskMemAlloc(m_MediaTypeAudio.cbFormat);
			m_MediaTypeAudio.bFixedSizeSamples=false;
			m_MediaTypeAudio.bTemporalCompression=false;
			m_MediaTypeAudio.lSampleSize=1;

			ZeroMemory(m_MediaTypeAudio.pbFormat, m_MediaTypeAudio.cbFormat);
			MPEG1WAVEFORMAT *pWF =(MPEG1WAVEFORMAT*)m_MediaTypeAudio.pbFormat;
				pWF->wfx.nBlockAlign=0;
			pWF->wfx.cbSize =0; //extra data
			pWF->wfx.wBitsPerSample=0;
			if (audio_det->stream_type == 0x03 || audio_det->stream_type== 0x04)
			{
				pWF->wfx.wFormatTag=WAVE_FORMAT_MPEG;
				pWF->fwHeadLayer= m_pParser->toDsMpeg2Layer(audio_det->pes_adet.mpeg_layer);
			}
			pWF->wfx.nChannels=audio_det->pes_adet.channels;
			pWF->wfx.nSamplesPerSec=audio_det->pes_adet.sampfreq;
			pWF->wfx.nAvgBytesPerSec=audio_det->pes_adet.bitrate*125;

			if (prevSubtype != m_MediaTypeAudio.subtype)
				hr =  AUDIO_PID_CHANGE_RECONNECTION_NEEDED;
			else if ( (prevChanNum != pWF->wfx.nChannels) || (prevSampFreq != pWF->wfx.nSamplesPerSec) )
				hr = AUDIO_PID_CHANGE_FORMAT_CHANGE_NEEDED ;

			if (prevSubtype != GUID_NULL)
			{
				m_audio_pid_format_change=true;
			}

		}
		// ================================

		return hr;
}

// DVB subtitles decoding thread. 
void CPushSourceUdp::DVBSubDecodeThread()
{
	LONGLONG ds_ref_time=0;
	// get starting pts from video pin
	while(m_pPinSubtitles->GetStartPts(&ds_ref_time)==-1)
	{
		if (WaitForSingleObject(m_pPinSubtitles->m_ExitFillBuffer, 0)!=WAIT_TIMEOUT)
			return;
		Sleep(500);
	}

	BYTE time_out;
	LONGLONG ds_time_out=0;
    LONGLONG sample_ds_start;
	LONGLONG last_sample_time=0;
	unitES_t* ut=NULL;
	bufferES_t* bt=NULL;
	int bsize=0;
    CRefTime creftime;
	BYTE decode_result;

	do
	{
		if (WaitForSingleObject(m_pPinSubtitles->m_ExitFillBuffer, 0)!=WAIT_TIMEOUT)
			return;
      
		// get stream time
	    StreamTime(creftime);

		if (m_pSubsFifo->items()>0)	// new subtitle available
		{
			CAutoLock cAutoLock(&m_pLockSub);

			ut = m_pSubsFifo->front();
			bt = ut->buffer;

			// calculate if subtitle is about to be shown.
			sample_ds_start = ( ( bt->present_time +  (m_pPinSubtitles->GetPtsCounter()*MAX_PTS) - m_pPinSubtitles->GetStartPts(&ds_ref_time))*1000/9) + GLOBAL_SUB_DELAY + ds_ref_time;
			if (creftime.m_time > sample_ds_start || (sample_ds_start + DS_MAX_PTS) <  creftime.m_time )
			{
				decode_result = DVBSubDecode(bt,m_pPinSubtitles->m_page_id, m_pPinSubtitles->m_ancillary_id,&time_out);
				if (decode_result == COMPLETE_DISPLAY_SET)
                    m_dvb_sub_status = DVBDEC_SHOW_NEW;
				
				if (time_out > 5)
					ds_time_out=50000000; //ds units
				else
					ds_time_out = time_out*10000000;

				last_sample_time = sample_ds_start;
			}
			else
			{
				// check if previous sample expired
				if (m_dvb_sub_status==DVBDEC_SHOW_PREV)
				{
					if (last_sample_time + ds_time_out < creftime.m_time )
						m_dvb_sub_status=DVBDEC_CLEAR;
				}
			}
		}
		else
		{
			CAutoLock cAutoLock(&m_pLockSub);

			// check if previous sample expired
			if (m_dvb_sub_status==DVBDEC_SHOW_PREV)
			{
				if (last_sample_time + ds_time_out < creftime.m_time )
					m_dvb_sub_status=DVBDEC_CLEAR;
			}
		}
	
		Sleep(100); //cca 10 fps

	}
	while (TRUE);
}

BYTE CPushSourceUdp::DVBSubDecode(bufferES_t* bt, u_short page_id, u_short ancillary_id, BYTE* time_out)
{
	BYTE decode_result;
	decode_result = m_pPinSubtitles->m_dvb_decoder->PreProcessor(bt->buf, bt->length, page_id,ancillary_id,NULL,time_out);
	m_pSubsFifo->pop();
	delete[] bt->buf;
	delete bt;

	return decode_result;
}


// Dynamic stream changes:
// ================================================================================
// Following functions must be used after demuxer is configured and graph is running. 
//  ===============================================================================
// At the moment only dynamic pid change for subtitles is supported.
STDMETHODIMP CPushSourceUdp::SetVideoPid(u_short iVpid)
{
	CAutoLock cAutoLock(&m_pLock);
	//CPushSourceUdp::m_Vpid = iVpid;
 //   //Set also pid type for parsing video headers
	//piddetails_t* video_det = m_pParser->get_details(iVpid);
 //   if (video_det!=NULL)
	//	CPushSourceUdp::m_VpidType = video_det->stream_type;
	m_resetSubES=true;
	return S_OK;
}

STDMETHODIMP CPushSourceUdp::SetAudioPid(u_short iApid)
{
	HRESULT hr = AUDIO_PID_CHANGE_NO_CHANGES_NEEDED;
	
	CAutoLock cAutoLock(&m_pLock);
	piddetails_t* audio_det = m_pParser->get_details(iApid);
	if (audio_det != NULL)
	{
         hr = setPrimaryAudioPinMediaType(audio_det,iApid); 
		 m_resetAudES=true;
	}



	return hr;
}

STDMETHODIMP CPushSourceUdp::SetSubtitlesPid(u_short iSpid)
{
	CAutoLock cAutoLock(&m_pLock);

	// =subtitles=:
	piddetails_t* sub_det = m_pParser->get_details(iSpid);
	if (sub_det!=NULL)
	{
		// demuxer =======================
		m_Spid = iSpid;
		// ==============================

		// subtitle pin =================
		m_pPinSubtitles->m_page_id = sub_det->subtitling_descriptors_v[0].composition_page_id;
		m_pPinSubtitles->m_ancillary_id = sub_det->subtitling_descriptors_v[0].ancillary_page_id;

		if (sub_det->subtitling_descriptors_v.size() > 0)
		{
			// Start DVB decoder thread if application wants to poll for subtitles.
			if ( (m_flags & DEMUX_FLAGS_POLL_SUBTITLES) ==true )
			{
				m_subtitle_polling =true;
				if (m_SubtitleDecoderThread==NULL)
				{
					m_SubtitleDecoderThread = CreateThread( NULL,						// security
							  NULL,										// stack size
							  (LPTHREAD_START_ROUTINE)CPushSourceUdp::DVBDecodingThreadStaticEntryPoint,// entry-point-function
							  this,										// arg list 
							  NULL,										// suspend
							  NULL );									//&uiThread1ID
						OutputDebugString(TEXT("DVB decoding thread started.\n"));
				}
			}
		}
		// ==============================
	}
	else
		m_Spid = 0;

	return S_OK;
}

// ==============================================================================

STDMETHODIMP CPushSourceUdp::Pause ()
{
    CAutoLock cAutoLock(&m_pLock);
	
	if (m_pause_second_time) // Not the best solution - but it works
	    SetEvent(m_DemuxerConfiguredEvent); // Signal to demux thread that demuxer can start
	m_pause_second_time=TRUE;

    return CSource::Pause();
}

HRESULT CPushSourceUdp::StreamTime(CRefTime& rtStream)
{
	//if(m_State == State_Stopped)
	//	rtStream = 0;
	//else if (m_State == State_Paused)
	//	rtStream =0;
	//else
		return CBaseFilter::StreamTime(rtStream);

	return S_OK;
}

STDMETHODIMP CPushSourceUdp::Stop()
{
    HRESULT hr ;
	hr =  S_OK;

    CAutoLock cAutoLock(&m_pLock);

	if (!m_dont_leave_multicast)
	{
		m_pReceiver->Stop();
		SetEvent(m_pReceiver->data->m_data_rcvd_event); // prevent blocking
		
		
		SetEvent(m_ExitParseThreadEvent); 
        SetEvent(m_PushLimitEvent);						// prevent blocking
		WaitForSingleObject(m_DemuxFillingEnded,INFINITE);
		
		SetEvent(m_ExitDemuxThreadEvent);
		SetEvent(m_DemuxFifoDataEvent);					// prevent blocking
		SetEvent(m_DemuxerConfiguredEvent);				// if Stop() is called before filter is configured.
		SetEvent(m_pPinVideo->m_ExitFillBuffer);
		SetEvent(m_pPinAudio->m_ExitFillBuffer);
		SetEvent(m_pPinSubtitles->m_ExitFillBuffer);

		if (m_mode == PARSING_MODE)
			WaitForSingleObject(m_ParseThread, 1000);	// Prevent stopping filter before parser finished.

	}
	m_dont_leave_multicast=false;

	return CSource::Stop();
}


STDMETHODIMP CPushSourceUdp::Run(REFERENCE_TIME tStart)
{
	HRESULT hr=S_OK;
	CAutoLock cAutoLock(&m_pLock);
  
   // Start receiving:
   if (!m_reciver_started)
   {
	   m_pReceiver->m_multiIpAddr = CPushSourceUdp::m_Mip;
	   m_pReceiver->m_nicIpAddr = CPushSourceUdp::m_Nip;
	   m_pReceiver->m_iPort = htons(CPushSourceUdp::m_Port);
	   m_pReceiver->m_nic_index = m_nic_index;
	   if(_tcsclen(m_src_file)>0)
			_tcscpy_s(m_pReceiver->m_src_file,_countof(m_pReceiver->m_src_file),m_src_file);
	   m_pParser->m_mode = m_mode;

	   Dump1(TEXT("Ip addres: %d - UINT network order\n"), m_pReceiver->m_multiIpAddr);
	   Dump1(TEXT("Nic address: %d - UINT network order\n"), m_pReceiver->m_nicIpAddr);
	   Dump1(TEXT("Port: %d \n"), CPushSourceUdp::m_Port);
	   
	   m_pPinVideo->m_source = m_source;
	   m_pPinAudio->m_source = m_source;
	   m_pPinSubtitles->m_source = m_source;
	   m_pReceiver->m_source = m_source;
	   m_pParser->m_source=m_source;

	   hr = m_pReceiver->Start(); // init

	   m_pParser->m_src_file_len = m_pReceiver->m_file_len; // Used only if source is a file.
	   
	   if (hr != S_OK)
		  return hr;
	  
	   m_pReceiver->StartReceiving(); 
	   NotifyEvent(EC_RECEIVER_STARTED,0,0);

	   OutputDebugString(TEXT("Receiver started...\n"));
	   m_reciver_started=true;
   }
   
   // Initial parsing:  
   if (m_ParseThread==NULL)
   {
		m_ParseThread = CreateThread( NULL,						// security
					  NULL,										// stack size
					  (LPTHREAD_START_ROUTINE)CPushSourceUdp::ParseThreadStaticEntryPoint,// entry-point-function
					  this,										// arg list 
					  NULL,										// suspend
					  NULL );									//&uiThread1ID
	    OutputDebugString(TEXT("Parsing thread started.\n"));
		//SetThreadPriority(m_ParseThread, THREAD_PRIORITY_HIGHEST);

   }

   // Parse and demux:
   if (m_mode == PLAYBACK_MODE && m_ParseDemuxThread==NULL)
   {
	   
	   // Start demuxing thread
		m_ParseDemuxThread = CreateThread( NULL,				// security
					    NULL ,									// stack size
					  (LPTHREAD_START_ROUTINE)CPushSourceUdp::ParseAndDemuxThreadStaticEntryPoint,// entry-point-function
					  this,										// arg list 
					  NULL,										// suspend
					  NULL );									//&uiThread1ID
		OutputDebugString(TEXT("Demuxing thread thread started.\n"));
		//SetThreadPriority(m_ParseDemuxThread, THREAD_PRIORITY_HIGHEST);
   }
  
   return CSource::Run(tStart);
}

STDMETHODIMP CPushSourceUdp::GetPATPid(int prog_index, u_short* prog_num, u_short* pat_pid)
{
	*prog_num = m_pParser->m_programes[prog_index].first;
	*pat_pid =  m_pParser->get_pat_pid(*prog_num);
	return S_OK;
}

STDMETHODIMP CPushSourceUdp::GetNITPid(u_short* nit_pid)
{
   *nit_pid =  m_pParser->get_pat_pid(0);
	return S_OK;
}

STDMETHODIMP CPushSourceUdp::GetPMTType(int prog_index, int pid_index, u_short* pid, int* type)
{
	m_pParser->get_pmt_types(prog_index,pid_index,pid,type);

	return S_OK;
}


CUnknown * WINAPI CPushSourceUdp::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    CPushSourceUdp *pNewFilter = new CPushSourceUdp(pUnk, phr );

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;
}


// Helper functions =============================================
// Shell array sorting algorythm
void CPushPinUdpVideo::shellsort(LONGLONG a[],int n)
{
	int j,i,m;
	LONGLONG mid;
	for(m = n/2;m>0;m/=2)
	{
		for(j = m;j< n;j++)
		{
			for(i=j-m;i>=0;i-=m)
			{
				if(a[i+m]>=a[i])
					break;
				else
				{
					mid = a[i];
					a[i] = a[i+m];
					a[i+m] = mid;
				}
			}
		}
	}
};
// ==============================================================