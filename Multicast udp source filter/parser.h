//Copyright (c) 2010, Uroš Steklasa
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//	* Redistributions may not be sold, nor may they be used in a commercial product or activity.
//	* Redistributions of source code and/or in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Uroš Steklasa "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//version: 0.90
#ifndef H_PARSER
#define H_PARSER

#include "bits.h"
#include <streams.h>
#include <vector>
#include <algorithm>

#define PRIVATE_AC3 0x81
#define PRIVATE_DVB_SUBS 0x8801
#define PRIVATE_TTX_SUBS 0x8802

#define PROGRAM_STREAM_MAP	0xbc
#define PRIVATE_STREAM_1	0xbd
#define PADDING_STREAM		0xbe
#define PRIVATE_STREAM_2	0xbf
#define ECM_STREAM			0xf0
#define EMM_STREAM			0xf1
#define ANCILLARY_STREAM	0xf9
#define PROGRAM_STREAM_DIRECTORY 0xff
#define DSMCC_STREAM		0xf2
#define ITUTRecH222typeE    0xf8

#define EC_PARSER_COMPLETE 0xC3500
#define EC_MSG_Discontinuity 0xC3501
#define EC_PARSER_STARTED 0xC3502
#define EC_RECEIVER_STARTED 0xC3503
#define EC_PARSER_NOT_MPEGTS 0xC3504
#define EC_MSG_FirstAudioPacket 0xC3510
#define EC_MSG_FirstVideoPacket 0xC3511
#define EC_MSG_ParserRecvdFirstSample 0xC3512
#define EC_MSG_Delay 0xC3520
#define EC_MSG_Resync 0xC3521

#define EC_MSG_TS_PTS_WRAP 0xC3522
#define EC_MSG_TS_PTS_JUMP 0xc3523
#define EC_TS_VIDEO_PIN 0xC2
#define EC_TS_AUDIO_PIN 0xC3
#define EC_TS_SUBTITLE_PIN 0xC4

#define EC_MSG_AspectRatioChanged 0xC3530
#define EC_MSG_VideoDimensionChanged 0xC3531
#define EC_MSG_VideoADFChanged 0xc3532
#define EC_MSG_SERVICE_LIST_RECEIVED 0xC3550
#define EC_MSG_RTSP_PROTOCOL 0xc3551
#define EC_MSG_NIT_TABLE_PARSED 0xc3552
#define EC_MSG_EIT_TABLE_PARSED 0xc3553
#define EC_MSG_EIT_TABLE_CHANGED 0xc3554
#define EC_MSG_CURRENT_PMT_CHANGED 0xc3555 
#define EC_MSG_AT_LEAST_ONE_PMT_CHANGED 0xc3556  // NOT IMPLEMENTED YET !!!


#define NAL_BUILDER_MAX_LEN 512000
#define NALU_MAX_LEN 65535

#define MPEG2_VIDEO_FLAGS_PROGRESSIVE_SEQUENCE  0x2
#define MPEG2_VIDEO_FLAGS_FIELDS_1				0x4
#define MPEG2_VIDEO_FLAGS_FIELDS_2				0x8
#define MPEG2_VIDEO_FLAGS_FIELDS_3				0x10
#define MPEG2_VIDEO_FLAGS_FIELDS_4				0x20
#define MPEG2_VIDEO_FLAGS_FIELDS_6				0x40

#define TABLE_PAT 0x10
#define TABLE_PMT 0x11
#define TABLE_SDT 0x12
#define TABLE_EIT 0x13   //pid 0x12
#define TABLE_NIT 0x14

#define MAX_PROGS 20
#define MAX_PID_COUNT_PER_PROG 50

#define MAX_PAT_SIZE 1024
#define MAX_PMT_SIZE 10240

# define MAX_MPEG_PICT_HEADER_SIZE 45 // TODO: check if this is true
#define MAX_PICT_TIM_SEI_SIZE	45
#define MAX_SERVICE_NAME_LEN MAX_DESC_NAME_LEN
#define MAX_EVENT_NAME_LEN MAX_DESC_NAME_LEN
#define MAX_EVENT_TEXT_LEN 10000 //10k

#define VIDEO_STREAM 0x1
#define AUDIO_STREAM 0x2

#define PLAYBACK_MODE 1		// Filter is used for playback. Initial parser will exit as soon as all basic informations are retreived.
#define PARSING_MODE 2		// Filter is used for parsing - initial parser will exit when application stops it. Some additional informations could be gathered.
							// Parsing mode can be useful for channel scanner.

#define SOURCE_NET 1
#define SOURCE_FILE 2

#define INIT_PARSER_TIMEOUT 6000  // 6sec                  After that time parser exits even if it hasn't parsed all data yet.    
#define INIT_PARSER_IMPORTANT_DATA_TIMEOUT 3000 // 3 sec After that time parser checks if it has enaugh data for playback.
#define INIT_PARSER_MAX_FILE_LOOKUP 25000000 // Search only first 25 Megabytes 

#define MAX_PACKET_SIZE 208  

enum MAYOR_TYPE
{
   MAYOR_TYPE_VIDEO, 
   MAYOR_TYPE_AUDIO,
   MAYOR_TYPE_UNKNOWN
};
using namespace std;

struct piddetails_bas_t
{
	u_short stream_type;
	u_short private_stream_type;
	u_short pid;
	u_short PCR_PID;
	bool pes_parsed;
	bool pes_supported;
	bool has_data;
	unsigned char language[4];
};

struct piddetails_t : piddetails_bas_t
{
	pesvideodet_t pes_vdet;
	pesaudiodet_t pes_adet;
	vector<subtitling_t> subtitling_descriptors_v;
	vector<teletext_ebu_t> teletext_descriptors_v;
};

// descriptors
struct service_desc_t
{
	u_short prog;
	char service_provider[MAX_SERVICE_NAME_LEN];
	char service_name[MAX_SERVICE_NAME_LEN];
};

struct short_event_desc_t
{
	u_short prog;
	u_short date_mjd;
	UINT time_utcbcd;
	UINT duration_bcd;
	char event_name[MAX_EVENT_NAME_LEN];
	char event_text[MAX_EVENT_TEXT_LEN];
};



struct nit_descriptors_t
{
	USHORT network_id;

	network_name_desc_t network_name_descriptor;
	bool has_network_name_descriptor;
	
	std::vector<service_list_desc_t> service_list_descriptors;
	bool has_service_list_descriptor;
	
	cable_delivery_system_desc_t cable_delivery_system_descriptor;
	bool has_cable_delivery_system_descriptor;
	
	satellite_delivery_system_desc_t satellite_delivery_system_descriptor;
	bool has_satellite_delivery_system_descriptor;
	
	terrestial_delivery_system_desc_t terrestial_delivery_system_descriptor;
	bool has_terrestial_delivery_system_descriptor;

	nit_descriptors_t()
	{
	    ZeroMemory(&network_name_descriptor, sizeof(network_name_descriptor));
		ZeroMemory(&cable_delivery_system_descriptor, sizeof(cable_delivery_system_descriptor));
		ZeroMemory(&satellite_delivery_system_descriptor, sizeof(satellite_delivery_system_descriptor));
		ZeroMemory(&terrestial_delivery_system_descriptor,sizeof(terrestial_delivery_system_descriptor));
		has_network_name_descriptor = false;
		has_service_list_descriptor=false;
		has_cable_delivery_system_descriptor =false;
		has_satellite_delivery_system_descriptor=false;
		has_terrestial_delivery_system_descriptor =false;
		network_id =0;
	};
};

struct nit_ts_descriptors_t : nit_descriptors_t
{
	USHORT transport_stream_id;
	USHORT original_network_id;

	nit_ts_descriptors_t() : nit_descriptors_t()
	{
	    transport_stream_id=0;
		original_network_id=0;
	};
};

//================

struct psi_table_t
{
	bool complete;				// Indicates if a complete table is received.
	bool new_complete;          // Indicates if a complete table with different version than previous is available
	bool first_table_received;  //          
		
	BYTE table_id;
	bool first_part_received;
	char current_section[1024];
	int section_position;
	int section_length;
	BYTE last_section_number;
	BYTE section_number;
	int parsed_sections_count;
	BYTE version_number;
	bool has_changed;          // Internal indicitor that notifies descriptors parsers to discard previous data (clearing lists,vectors).
};

struct pat_table_t : psi_table_t
{
	vector<pair<u_short,pair<u_short,bool>>> list; //[prog_num,[pmt_pid,parsed]]
} ;

struct pmt_table_t : psi_table_t
{
	u_short PCR;
	u_short prog_info_len;
	u_short current_pmt_program;
	bool first_complete_table;
};

struct nit_table_t : psi_table_t
{

};

struct sdt_table_t : psi_table_t
{
	
};

struct eit_table_t : psi_table_t
{

};

struct pes_basic_info_t
{
	LONGLONG PTS;
	bool has_PTS;
	LONGLONG DTS;
	bool has_DTS;
	int frame_start;		// ES start
	u_short es_length;
	bool unit_start;
	u_short stream_type;
};

// mpeg2 info [ ext_pict_hdr = extendend picture header, seq_hdr = sequence header ]
struct mpeg2_ext_info_t
{
	int sample_fields;		// number of fields in sample
	bool is_key_frame;		
	UINT key_frame_pos;
	UINT video_arx;
	UINT video_ary;
	UINT video_width;
	UINT video_height;
	BYTE afd;

};

// h264 info
struct h264_ext_info_t
{
	UINT ext_video_key_frame_pos; 
	bool ext_video_key_frame_found;
	int h264_access_unit_pos[50];
	int h264_access_unit_count;
	bool h264_is_idr;		 
	UINT video_arx;
	UINT video_ary;
	UINT video_width;
	UINT video_height;
	BYTE afd;
};

// h.264 sequence parameter set
struct SPS_flags_and_values_t
{
	bool video_nal_hrd_parameters_present_flag; 
	bool video_vcl_hrd_parameters_present_flag; 
	bool video_pic_struct_present_flag; 
	bool separate_colour_plane_flag;
	UINT max_frame_num;
};

struct mpeg2_seq_hdr_values_t
{
	BYTE progressive_sequence;
};

struct mpeg2_pict_cod_ext_values_t
{
	BYTE fields;
};

struct es_info_t
{
	bool mpeg_pict_ext_checked;					//MPEG
	bool mpeg2_video_picture_header_received;	//MPEG
	char* mpeg2_video_picture_header;			//MPEG
	bool nal_seq_param_set_parsed;				//h.264
	bool first_video_pic_struct_parsed;			//h.264
	bool first_pict_timing_received;			//h.264
	char *first_pict_timing_sei;				//h.264
	SPS_flags_and_values_t*  sps;				//h.264
	mpeg2_seq_hdr_values_t* seq_hdr;			//mpeg
	mpeg2_pict_cod_ext_values_t* pict_cod;		//mpeg

	BYTE mpeg2_video_picture_header_last_map_pos;
	BYTE first_pict_timing_sei_last_map_pos;
    BYTE sps_last_map_pos;
	BYTE slice_last_map_pos;
	BYTE seq_hdr_map_pos;
	BYTE pict_cod_map_pos;
};

struct slice_header_values_t
{
	int last_pic_parameter_set_id;
	int last_frame_num;
};

struct selected_stream_t
{
	int vid_prog_index;
	int vid_stream_index;
	int aud_prog_index;
	int aud_stream_index;

	selected_stream_t()
	{
		aud_prog_index=-1;
		aud_stream_index=-1;
		vid_prog_index=-1;
		vid_stream_index=-1;
	};
};

struct rtp_header
{
	int header_length;
	USHORT seq_number;
	UINT timestamp;
	UINT ssrc;
};

//enum ts_structure_enum
//{
//	TS_STANDARD,
//	TS_TIMECODE,   // 4 byte time code in front ov every packet
//	TS_OVERRTP         // RTP header in front of every MTU 
//};

struct ts_structure
{
   ts_structure_enum_t type;
   int start_ofs;
   int packet_len;
   int packets_in_mtu;    // Relavant only for RTP
   int rtphdr_len;        // Relavant only for RTP
   ts_structure()
   {
       type = TS_STANDARD;
	   packets_in_mtu = 0;
	   start_ofs=0;
	   rtphdr_len=0;
	   packet_len =0;
   }
};

class CParser:CBits
{
	private:
		DWORD m_start_time;
		bool m_first_call;
		LONGLONG m_counter;
		int m_recv_buffer_size;
		
		CCritSec m_pLock;
		CSource* m_pSrcFilter;

		void processPacket(char* buffer, int start, int len);
		bool findPacketStructure(char* buf, int len);

		void parse_pat_data( );
		void parse_pmt_data();
		void parse_sdt_data();
		void parse_eit_data();
		void parse_nit_data();
		void descriptor_type(char*, int, u_short*, UINT*);

		BYTE adaption_field_control(char* , int);
		BYTE adaption_field_length(char* , int);
		void psi_section(char*, int, int*,int*, bool*, bool*);

		bool m_all_pes_parsed;
		bool is_pes_type_supported(u_short);
		MAYOR_TYPE pes_mayor_type(u_short);
		 		
		void mpeg_audio_header(char*, int, int, int, int);
		void dolby_audio_header(char*, int, int, int);
		void iso_iec_14496_3_header(char*, int, int, int, int);
		void aacAudioMuxElement(bool muxConfigPresent, char*, pesaudiodet_t* ad);
		void aacStreamMuxConfig(char*, int* pos, BYTE* offset, pesaudiodet_t* ad);
		UINT aacSpecificConfig(char*,int* pos, BYTE* offset, pesaudiodet_t* ad);
		BYTE aacGetAudioObjectType(char*,int* pos, BYTE* offset);

		HRESULT mpeg_video(char*, int, int, int, int);
		HRESULT mpeg_video_picture_header(char*, int, int, int, int, mpeg2_ext_info_t*, int* len);
		int mpeg_video_sequence_header(char*, int, int, int, int, mpeg2_ext_info_t*);
		int find_pict_coding_ext(char* buf, int len);
		
		HRESULT h264_video(char*, int, int, int,int);
		
		int avc_seq_param_set(char*,int,int,int,int, h264_ext_info_t*);
		int avc_sei_msg(char*,int,int,int,int,h264_ext_info_t*);
		int avc_pict_timing_sei(char*,int,int,int,int,h264_ext_info_t* );
		bool avc_slice_header(char*,int,int,h264_ext_info_t*);
		void iso_693_language_decsriptor(char*,int,int,char*);
		void teletext_descriptor(char*, int, int, vector<teletext_ebu_t>*);
		void subtitling_descriptor(char*, int, int, vector<subtitling_t>*);
		void services_descriptor(char*, int,u_short);
		void short_event_descriptor(char*, int, u_short, short_event_desc_t*,bool );

		void parse_network_name_desc(char* buf, int len, network_name_desc_t* dsc);
		void parse_service_list_desc(char* buf, int len, vector<service_list_desc_t>* dsc);
		void parse_cable_delivery_system_desc(char* buf, int len, cable_delivery_system_desc_t* dsc);
		void parse_satellite_delivery_system_desc(char* buf, int len, satellite_delivery_system_desc_t* dsc);
		void parse_terrestial_delivery_system_desc(char* buf, int len, terrestial_delivery_system_desc_t* dsc);

		int find_nal_start(char*buf,int pos, int end, int* code_len);
		int remove_emulation_prevention_bytes(char* input, int len, int pos, char* output);

		int afd_mpeg(char*, int, int, int, int, mpeg2_ext_info_t*);
		int afd_h264(char*,int,int,int,int,h264_ext_info_t* );
		BYTE getAfd(char*);

		void clear_videodet(pesvideodet_t*);
		void clear_audiodet(pesaudiodet_t*);
		void clear_parsed_data();
	
		// Data from headers of video elementary streams.
		// Data is retreived by initial parser and is then used at the begining of demuxing thread.
		// This way demuxer thread doesn't need to wait for headers.
		es_info_t m_pes[MAX_PROGS][MAX_PID_COUNT_PER_PROG];		

		vector<pair<u_short,char[MAX_SERVICE_NAME_LEN]>> m_services; //[prog_num, name]

		// partial packet
        #define PART_PACKET_SIZE  MAX_PACKET_SIZE*2
		char m_part_packet[PART_PACKET_SIZE];
		int m_part_packet_pos;
		int m_part_packet_stop;

		// ES builder variables for initial parser:
		char m_nal_unit[NALU_MAX_LEN];
		char m_nal_builder[NAL_BUILDER_MAX_LEN];
		int m_nal_pos;
		bool m_nal_build;
		char m_mpeg_builder[NAL_BUILDER_MAX_LEN];
		int m_mpeg_pos;
		bool m_mpeg_build;

		char m_first_pict_timing_sei[MAX_PROGS][MAX_PICT_TIM_SEI_SIZE];
		char m_first_mpeg_pict_hdr[MAX_PROGS][MAX_MPEG_PICT_HEADER_SIZE];
		
		SPS_flags_and_values_t m_sps[MAX_PROGS];
		mpeg2_seq_hdr_values_t m_seqhdr[MAX_PROGS];
		mpeg2_pict_cod_ext_values_t m_pictcod[MAX_PROGS];

		// m_dmt prfix means that this variables are used only by demux thread.
		//=========================================
		SPS_flags_and_values_t m_dmt_sps;			 // Last received SPS values
		mpeg2_seq_hdr_values_t m_dmt_seqhdr;         // Last received SEQ header values
		mpeg2_pict_cod_ext_values_t m_dmt_pict_cod;  // Last received Picture Cod Ext values
		slice_header_values_t m_dmt_slice;           // Last received slice values
		bool m_dmt_sps_received;
		bool m_dmt_seqhdr_received;
		bool m_dmt_pictcod_received;
		// ========================================

		vector<service_desc_t> m_global_services;  // Service descriptor.

		void clear_tables();

		int findbyte(char* buf, int len, BYTE byte);
		int findshort(char*buf, int len, short bytes);

		selected_stream_t m_cur_prog;

	public:
		
		CParser(int buf_size);
		CParser(CSource*);					// Constructor with Directshow soiurce filter backpointer.
		~CParser();
		BYTE m_mode;
		BYTE m_source;
		LONGLONG m_src_file_len;
		bool m_all_data_parsed;
		bool m_enaugh_data_parsed; // Enaugh data parsed for playback (at least 1 audio and 1 video in one programe)
		bool global_service_list_parsed;
		ts_structure m_ts_structure;
		int m_nit_pid;

		bool transport_stream(char*, int);						 // Call this function with ts buffer as input until all_data_parsed isn't true
		
		vector<pair<u_short, vector<piddetails_t>>> m_programes; // Parsing result.
		
		vector<piddetails_t> m_dmt_last_PMT_of_curr_prog; // PMT could change during playback. This vector holds last parsed PMT details of currently selected program.
		
		HRESULT parse_pes(char*, int, int, int, pes_basic_info_t*, byte);
		HRESULT parse_rtp(char*, int, rtp_header*);
		u_short continuity_count(char*, int);

		pmt_table_t m_pmts;                                      //pmt table
		sdt_table_t m_sdt;										// sdt table
		pat_table_t m_pat;										// pat table
		eit_table_t m_dmt_eit;							// eit table - used only by demuxer
		nit_table_t m_nit;                                      // nit table - used only by demuxer
		rtp_header m_first_rtp_hdr;                             // first rtp header
		
		nit_descriptors_t m_nit_net_descs; // Contains some of descriptors that can be found in NIT table.
		vector<nit_ts_descriptors_t> m_nit_ts_descs;
		
		void parse_psi(char*,  int, BYTE);

		// overloaded functions for demultiplexer:
		void h264_video(char*, int,h264_ext_info_t*);
		void mpeg_video(char*, int, mpeg2_ext_info_t*);

		bool payload_unit_start(char* , int );

		piddetails_t* get_details(int pid);
		void programesCount(int*);							// Get number of programes (usually only one).
		void pidCount(int, int*);							// Get number of different PIDSs for a particular programe.
		void parsed_data(int, int , interop_piddetails_t*); // Parsed data structure
		void get_sub_desc(int , int, int , subtitling_t*);
		void get_ttx_desc(int , int , int , teletext_ebu_t*);
		void get_audio_det(int,int, pesaudiodet_t*);
		void get_video_det(int,int, pesvideodet_t*);
		void get_pmt_types(int, int, u_short*, int*);
		u_short get_pat_pid(int prog_num);
		u_short get_current_next_event(int prog, char* current, char* next);
		void get_service_descriptor(int index, unsigned char* provider, unsigned char* name, u_short* prog);
		int services_count();

		USHORT setCurrentStream(int pid, BYTE type);        // Because application sets streams by PIDs and not programes, we must guess which programe is selected by video or audio PID.

		static DWORD toDsMpeg2Profile(DWORD profile);
		static DWORD toDsMpeg2Level(DWORD level);
		static WORD toDsMpeg2Layer(WORD layer);

};

#endif