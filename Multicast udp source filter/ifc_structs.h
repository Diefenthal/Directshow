
#include <vector>

#ifndef H_IFCSTRUCTS
#define H_IFCSTRUCTS

typedef unsigned short  u_short;

typedef struct _teletext_ebu_t
{
	char language[4];
	u_short type;
	BYTE magazine_number;
	BYTE page_number;
}teletext_ebu_t;

typedef struct _subtitling_t
{
	char language[4];
	u_short type;
}subtitling_t;

typedef struct _pesvideodet_t
{
	LONG width;
	LONG height;
	UINT aspect_x;
	UINT aspect_y;
	LONGLONG timeperframe; // 100 - nanosecond unit
	UINT profile;
	UINT level;
	UINT bitrate;//kbps
}pesvideodet_t;

typedef struct _pesaudiodet_t
{
	u_short channels;
	u_short mpeg_layer;
	u_short mpeg_version;
	UINT sampfreq;
	UINT bitrate; //kbps
} pesaudiodet_t;

typedef struct _piddetails_t
{
	u_short stream_type;
	u_short private_type;
	u_short pid;
	 struct pesvideodet_t pes_vdet;
	 struct pesaudiodet_t pes_adet;
	BOOL pes_parsed;
	BOOL pes_supported;
	char language[4];
	vector<subtitling_t> subtitling_descriptors_v;
	vector<teletext_ebu_t> teletext_descriptors_v;
}_piddetails_t;

typedef struct _interop_piddetails_t
{
	u_short stream_type;
	u_short private_type;
	u_short pid;
	pesvideodet_t pes_vdet;
	pesaudiodet_t pes_adet;
	BOOL pes_supported;
	char language[4];
	int subtitling_descriptor_count;
	int teletext_descriptor_count;
}interop_piddetails_t;

#endif