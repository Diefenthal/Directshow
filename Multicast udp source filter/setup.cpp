#include "proj.h"
#include <initguid.h>
#include "udpguids.h"
#include "udpfilter.h"
#include "proppage.h"


// For a specialized source filter like this, it is best to leave out the 
// AMOVIESETUP_FILTER altogether, so that the filter is not available for 
// intelligent connect. We use CLSID to create the filter or just 
// use 'new' in application.



// video out

const AMOVIESETUP_MEDIATYPE sudVidOpPinTypes[4] =
{
	{
		&MEDIATYPE_Video,             
		&MEDIASUBTYPE_H264
	},
	{
		&MEDIATYPE_Video,          
		&MEDIASUBTYPE_AVC
	},
	{
		&MEDIATYPE_Video,             
		&MEDIASUBTYPE_MPEG2_VIDEO
	},
	{
		&MEDIATYPE_Video,             
		&MEDIASUBTYPE_MPEG1Video
	},

};
// audio out
const AMOVIESETUP_MEDIATYPE sudAudOpPinTypes[5] =
{
	{
		&MEDIATYPE_Audio,              
		&MEDIASUBTYPE_MPEG2_AUDIO
	},
	{
		&MEDIATYPE_Audio,             
		&MEDIASUBTYPE_DOLBY_AC3
	},
	{
		&MEDIATYPE_Audio,              
		&MEDIASUBTYPE_MPEG1AudioPayload
	},

	{
		&MEDIATYPE_Audio,              
		&MEDIASUBTYPE_LATM_AAC
	},
	{
		&MEDIATYPE_Audio,              
		&MEDIASUBTYPE_MPEG1Audio
	},

};
// DVB subtitles out
const AMOVIESETUP_MEDIATYPE sudSubOpPinTypes =
{
    &MEDIATYPE_Video,              // Major type
	&GUID_NULL
};
//ttx out
const AMOVIESETUP_MEDIATYPE sudTtxOpPinTypes =
{
    &MEDIATYPE_VBI,              // Major type
	&MEDIASUBTYPE_TELETEXT
};
//ttx out

const AMOVIESETUP_PIN sudOutputPin[3] = 
{
	{
		L"Output-video",      // Obsolete, not used.
		FALSE,          // Is this pin rendered?
		TRUE,           // Is it an output pin?
		FALSE,          // Can the filter create zero instances?
		FALSE,          // Does the filter create multiple instances?
		&CLSID_NULL,    // Obsolete.
		NULL,           // Obsolete.
		4,              // Number of media types.
		sudVidOpPinTypes  // Pointer to media types.
	},

	{
		L"Output-audio",      // Obsolete, not used.
		FALSE,          // Is this pin rendered?
		TRUE,           // Is it an output pin?
		FALSE,          // Can the filter create zero instances?
		FALSE,          // Does the filter create multiple instances?
		&CLSID_NULL,    // Obsolete.
		NULL,           // Obsolete.
		5,              // Number of media types.
		sudAudOpPinTypes  // Pointer to media types.
	},

	{
		L"Output-subtitles",      // Obsolete, not used.
		FALSE,          // Is this pin rendered?
		TRUE,           // Is it an output pin?
		FALSE,          // Can the filter create zero instances?
		FALSE,          // Does the filter create multiple instances?
		&CLSID_NULL,    // Obsolete.
		NULL,           // Obsolete.
		1,              // Number of media types.
		&sudSubOpPinTypes  // Pointer to media types.
	}

	//	{
	//	L"Output-teletext",      // Obsolete, not used.
	//	FALSE,          // Is this pin rendered?
	//	TRUE,           // Is it an output pin?
	//	FALSE,          // Can the filter create zero instances?
	//	FALSE,          // Does the filter create multiple instances?
	//	&CLSID_NULL,    // Obsolete.
	//	NULL,           // Obsolete.
	//	1,              // Number of media types.
	//	&sudTtxOpPinTypes  // Pointer to media types.
	//}

};

const AMOVIESETUP_FILTER sudPushSourceUdp =
{
    &CLSID_UdpSourceFilter, // Filter CLSID
    g_wszUdpFilterName,     // String name
    MERIT_DO_NOT_USE,       // Filter merit
    3,                      // Number pins
    sudOutputPin           // Pin details
};

CFactoryTemplate g_Templates[2] = 
{
    { 
      g_wszUdpFilterName,                // Name
      &CLSID_UdpSourceFilter,            // CLSID
      CPushSourceUdp::CreateInstance,  // Method to create an instance of Component
      NULL,                              // Initialization function
      &sudPushSourceUdp                  // Set-up information (for filters)
    },

    // This entry is for the property page.
    { 
        L"Filter Properties",
        &CLSID_UdpSourceFilterPropPage,
		CUdpFilterProp::CreateInstance,
        NULL, NULL
    }

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (default implementations)
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}