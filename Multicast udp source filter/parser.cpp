//Copyright (c) 2010, Uro� Steklasa
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//	* Redistributions may not be sold, nor may they be used in a commercial product or activity.
//	* Redistributions of source code and/or in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Uro� Steklasa "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//version: 0.90

#include <math.h>
#include "parser.h"

#define SYNC_BYTE 0x47
#define MIN_PACKET_LENGTH 188

//***mpeg video
const BYTE Mpeg_AspectX[]=
{
	1,
	1,
	4,
	16,
	221,
};

const BYTE Mpeg_AspectY[]=
{
	1,
	1,
	3,
	9,
	100,
};

const UINT MpegTimePerFieldN[]=
{
	// ds units
	200000,  //forbideen
	625625,  //23.976fps
	625000,  //24
	200000,  //25
	500500,  //29,97
	500000,  //30
	100000,  //50
	250250,  //59.94
	250000,  //60
};

const UINT MpegTimePerFieldD[]=
{
	1,
	3,
	3,
	1,
	3,
	3,
	1,
	3,
	3,
};
//***

//***avc
const BYTE Avc_SubWidthC[]=
{
	1,
	2,
	2,
	1,
};

const BYTE Avc_SubHeightC[]=
{
    1,
    2,
    1,
    1,
};

const BYTE Avc_Aspect_X[]=
{
	0,
	1,
	12,
	10,
	16,
	40,
	24,
	20,
	32,
	80,
	18,
	15,
	64,
	160,
	4,
	3,
	2,
};

const BYTE Avc_Aspect_Y[]=
{
	0,
	1,
	11,
	11,
	11,
	33,
	11,
	11,
	11,
	33,
	11,
	11,
	33,
	99,
	3,
	2,
	1,
};
//***

//***dolby
const UINT DolbySampFreq[]=
{
48000,
44100,
32000,
};

// kbytes/s
const u_short Dolby_frmscod[38][4] = {
 {32,  64,   69,   96   },
 {32,  64,   70,   96   },
 {40,  80,   87,   120  },
 {40,  80,   88,   120  },
 {48,  96,   104,  144  },
 {48,  96,   105,  144  },
 {56,  112,  121,  168  },
 {56,  112,  122,  168  },
 {64,  128,  139,  192  },
 {64,  128,  140,  192  },
 {80,  160,  174,  240  },
 {80,  160,  175,  240  },
 {96,  192,  208,  288  },
 {96,  192,  209,  288  },
 {112, 224,  243,  336  },
 {112, 224,  244,  336  },
 {128, 256,  278,  384  },
 {128,  256,  279,  384  },
 {160,  320,  348,  480  },
 {160,  320,  349,  480  },
 {192,  384,  417,  576  },
 {192, 384,  418,  576  },
 {224, 448,  487,  672  },
 {224,  448,  488,  672  },
 {256, 512,  557,  768  },
 {256, 512,  558,  768  },
 {320, 640,  696,  960  },
 {320, 640,  697,  960  },
 {384, 768,  835,  1152 },
 {384, 768,  836,  1152 },
 {448, 896,  975,  1344 },
 {448, 896,  976,  1344 },
 {512, 1024, 1114, 1536 },
 {512, 1024, 1115, 1536 },
 {576, 1152, 1253, 1728 },
 {576, 1152, 1254, 1728 },
 {640, 1280, 1393, 1920 },
 {640, 1280, 1394, 1920 },
};
//***
//mpeg audio

//kbits/s
const UINT MpegV1[][3] =
{
{ 0,0,0}, 
{ 32,32,32 }, 
{ 64,48,40 }, 
{ 96,56,48 },
{ 128,64,56}, 
{ 160,80,64}, 
{ 192,96,80},
{ 224,112,96}, 
{ 256,128,112}, 
{ 288,160,128},
{ 320,192,160},
{ 352,224,192},
{ 96,256,224}, 
{ 416,320,256}, 
{ 448,384,320}, 
{ 0,0,0},
};

//kbits/s
const UINT MpegV2[][3] =
{
{ 0,0,0},
{ 32,8,8},
{ 48,16,16},
{ 56,24,24},
{ 64,32,32},
{ 80,40,40},
{ 96,48,48},
{ 112,56,56},
{ 128,64,64},
{ 144,80,80},
{ 160,96,96},
{ 176,112,112},
{ 192,128,128},
{ 224,144,144},
{ 256,160,160},
{ 0,0,0}
};

//* AAC
const UINT AACSampFreq[]=
{
9600,
88200,
64000,
48000,
44100,
32000,
24000,
22050,
16000,
12000,
11025,
8000,
7350,
0,
0,
0
};


CParser::CParser(int buf_size)
{
	m_recv_buffer_size =buf_size;
	m_counter=0;

	m_first_call =true;

	m_mode = PLAYBACK_MODE;
	m_source = SOURCE_NET;
	m_src_file_len=0;
	m_pSrcFilter=NULL;

	m_all_data_parsed=false;
	m_all_pes_parsed =false;
	m_enaugh_data_parsed = false;
	global_service_list_parsed=false;
	
	clear_tables();
	
	m_nal_pos=0;
	m_mpeg_pos=0;
	m_nal_build=false;	
	m_mpeg_build=false;

	ZeroMemory( m_part_packet, PART_PACKET_SIZE);
	 m_part_packet_pos=0;
	 m_part_packet_stop=0;

	for (int i=0 ; i< MAX_PROGS; i++)
	{
		ZeroMemory(&(m_sps[i]),sizeof(SPS_flags_and_values_t));
		ZeroMemory(&(m_seqhdr[i]),sizeof(mpeg2_seq_hdr_values_t));
		ZeroMemory(&(m_pictcod[i]),sizeof(mpeg2_pict_cod_ext_values_t));
	}
	ZeroMemory(&m_dmt_sps,sizeof(SPS_flags_and_values_t));
	ZeroMemory(&m_dmt_seqhdr,sizeof(mpeg2_seq_hdr_values_t));
	ZeroMemory(&m_dmt_pict_cod,sizeof(mpeg2_pict_cod_ext_values_t));
	m_dmt_sps_received=false;
	m_dmt_seqhdr_received=false;
	m_dmt_pictcod_received=false;

	m_dmt_slice.last_frame_num=-1;
	m_dmt_slice.last_pic_parameter_set_id=-1;

	ZeroMemory(&m_first_rtp_hdr,sizeof(rtp_header));

	m_nit_pid = -1;
}

CParser::~CParser()
{
	
}

// Initial parsing - main loop
// Use this function for initial parsing. Check for "m_all_data_parsed" variable when function returns.
// Once m_all_data_parsed is true, this function should be not used any more, demultiplexer should use only other public functions.
// This function rocognises packets with length larger than 188 (aligned or unaligned), packets with 4 byte timecodes (m2s,aligned) and 
// packets over RTP (aligned). 
bool CParser::transport_stream(char *buffer, int len)
{
	if (m_first_call)
	{
		m_start_time = GetTickCount();

		m_first_call=false; 
		if (!findPacketStructure(buffer, len)) // What kind of MPEG_TS are we receiving
		{
		    m_all_data_parsed= true;
			return false;
		}
		m_part_packet_pos=m_ts_structure.start_ofs;
	}

   if (m_mode == PLAYBACK_MODE )	// chanses for overflow are minimal
   {
	   DWORD tmp=GetTickCount()-m_start_time;
	   // Auto search limitations :
	    if (m_source ==SOURCE_NET && tmp<INIT_PARSER_TIMEOUT && tmp >INIT_PARSER_IMPORTANT_DATA_TIMEOUT )
		{
			// Check if we have at least 1 audio and 1 video
			for (UINT l=0; l<m_programes.size(); l++)
			{   
				UINT a=0;
				UINT v=0;
				UINT stream_num=m_programes[l].second.size();
				for (int m=0; m<stream_num; m++ )
				{
					MAYOR_TYPE t = pes_mayor_type(m_programes[l].second[m].stream_type);
					if ( t==MAYOR_TYPE_VIDEO && m_programes[l].second[m].pes_parsed ) 
						v+=1;
					else if ( t==MAYOR_TYPE_AUDIO && m_programes[l].second[m].pes_parsed) 
						a+=1;
				}
				if (a>0 && v >0)
				{
					m_enaugh_data_parsed=true;
					return true;
				}
			}
		}
	    else if (m_source ==SOURCE_NET && tmp>INIT_PARSER_TIMEOUT)
		{
			m_all_data_parsed=true;	// force exit 
			return true;
		}
		else if (m_source==SOURCE_FILE)
		{
			m_counter+=1;
			if (m_counter*m_recv_buffer_size > INIT_PARSER_MAX_FILE_LOOKUP || m_counter*m_recv_buffer_size > m_src_file_len)
			{
				m_all_data_parsed=true;
				return true;
			}
		}
   }
   
    int t = 0; 
	if ( m_ts_structure.type == TS_TIMECODE)
		t=4;

	// This will only happen if source is a file. In Net source, packets are aligned based on MTU.
	// We can't just choose appropriate buffer for a file source because of a RTP with variable length support
	// *******************************************************************************************
   if (  m_part_packet_stop != 0 )  // Process prev partial packet
   {
		if (m_ts_structure.type != TS_OVERRTP)
		{
		    memcpy(&m_part_packet[m_part_packet_pos],buffer,m_part_packet_stop); // Copy reminding part of a ts packet
		    processPacket(m_part_packet,t,m_ts_structure.packet_len); // Process full packet
		    m_part_packet_pos = m_part_packet_stop; //Align
		}
		else
		{
			//RTP can either start with rtp header or ts
			if ( m_part_packet_stop == SYNC_BYTE )
			{
			    // 
				memcpy(&m_part_packet[m_part_packet_pos],buffer,m_ts_structure.packet_len); // Copy reminding part of a ts packet
				processPacket(m_part_packet,t,m_ts_structure.packet_len); // Process full packet
				m_part_packet_pos = m_ts_structure.packet_len; //Align			
			}
			else
			{
			    rtp_header rhdr;
				parse_rtp(m_part_packet,PART_PACKET_SIZE,&rhdr);
				if (rhdr.ssrc == m_first_rtp_hdr.ssrc) // It shouldnever happen in mpeg-ts over rtp.
				{
					int i = rhdr.header_length;
					processPacket(&m_part_packet[i],0,m_ts_structure.packet_len);
					m_part_packet_pos = i + m_ts_structure.packet_len; //Align	
				}
				else
				{
					// Only for testing.
					m_part_packet_pos=0; // Reset. New rtp header needs to be found.
				}
			}
		}
   }
   // *************************************************************************************************

    int j = m_part_packet_stop+t; // packet start + timecodeoffset

	if ( m_ts_structure.type == TS_STANDARD || m_ts_structure.type == TS_TIMECODE )
	{
		int ps=0; //Packet start
		for( ; j<len;j+=m_ts_structure.packet_len+t)		
		{
			do// Make sure that there is nothing in between the packets.
			{
				if (buffer[j] == SYNC_BYTE) 
				{
					ps=j;				 
					if (j+ m_ts_structure.packet_len<=len )
			             processPacket(buffer,j,m_ts_structure.packet_len);
					 break;
				}
				else
					j+=1;
			}while (j<len);
		}
		if ( len-ps >= m_ts_structure.packet_len ) // Full packet with or without any additional header.
		{
			m_part_packet_pos=0;
			m_part_packet_stop=0;
		}
		else
		    m_part_packet_pos = (len - ps ) +t ;
		
		j=ps;
	}
	else  //rtp packets 
	{
		// RTP payload is not constant. It must only have an integer number of ts packets.
		// Header length may not be constant, so it must be allways parsed.
		// TODO: Collect rtp packets and process them in correct sequence. Is that really neccessary for initial parsing ?
		rtp_header rhdr;
		int h =0; // Position of last processet ts packet
		int tmp;

		// buffer with offset has either ts or rtp start
		if (buffer[j] !=  SYNC_BYTE)
		{
		    parse_rtp(&buffer[j],len-j,&rhdr);
			if (rhdr.ssrc == m_first_rtp_hdr.ssrc)
		        j+= rhdr.header_length; //Jump to ts start
			else
			{
				// This is only for testing. It should never happen!
			    // find new rtp header
				tmp = findshort(&buffer[j],len-j,m_first_rtp_hdr.ssrc); 
				if (tmp!=-1 && tmp >=8)
				{
		           parse_rtp(&buffer[tmp-1],len-j,&rhdr);
		           j+= rhdr.header_length; //Jump to ts start
				}
				else
					j=len; //Skip complete buffer
			}
		}

		h=j;
		j += m_ts_structure.packet_len; // Jump to next ts or next rtp start. Buffer will never be smaller than ts length.
		for ( ; j<len; j+=m_ts_structure.packet_len)
		{
		    if ( buffer[j] == SYNC_BYTE)
			{
				// Next packet is also ts.
				// Current packet is at h position
				processPacket(buffer,h,m_ts_structure.packet_len);
			}
			else
			{
				// Next packet is a rtp.
				processPacket(buffer,h,m_ts_structure.packet_len);
				if ( j +12 < len) // // File source can have a splice in the middle of the rtp header
				{
				    parse_rtp(&buffer[j],len-j,&rhdr); // Parse rtp header,
				}
				else
					break;

                tmp=j+rhdr.header_length;
				
				if ( tmp<len) // Skip for a rtp headerlength.
				   j =tmp; 
				else
				   break;
			}
			h=j;
		}
		// Remaining buffer will start with a rtp o ts. Copy it to a temp buffer and process it on next call
		j= h+ m_ts_structure.packet_len; // move to the end of last ts packet +1		
		if ( (m_part_packet_pos = len -j) == 0)
			processPacket(buffer,h,m_ts_structure.packet_len);
	} //end rtp

	// This will only happen if source is a file. In Net source, packets are aligned based on MTU. *************************************************
	if ( m_part_packet_pos != 0)
	{
	    memcpy(m_part_packet,&buffer[j-t],m_part_packet_pos); // Copy partial packet with timecode to temp buffer
 	    if (m_ts_structure.type == TS_OVERRTP)
			m_part_packet_stop=1; // no real meaning
		else
		    m_part_packet_stop=m_ts_structure.packet_len+t-m_part_packet_pos;// How many bytes of reminding packet with timecode is in next buffer.
	}
	// **********************************************************************************************************************************************

	if (!m_all_data_parsed && m_pat.complete && m_pmts.complete && m_all_pes_parsed )
		m_all_data_parsed=true;

	return true;
}


void CParser::processPacket(char* buffer, int j, int len)
{
	u_short pid; 
	byte scrambled;
	int pat_length;
	int stream_num;
		
	pid = ((buffer[j+1] & 0x1f) << 8) | (BYTE)buffer[j+2];
	scrambled = (buffer[j+3] & 0xc0) >> 6;

	// nit table
	//if (pid==0x10)
	//{
	//	// todo NIT table
	//}

	if (pid==0x11 && !m_sdt.complete)
		parse_psi(buffer, j, TABLE_SDT);
	//else if ( pid == m_nit_pid && !m_nit.complete)
	//	parse_psi(buffer,j,TABLE_NIT);


	//TODO: PSI version check: not in the initial parsing

	// Search for pat table.
	if (!m_pat.complete )
	{
		if ( pid == 0 )			// pid=0 parse pat table
			CParser::parse_psi(buffer, j, (BYTE)TABLE_PAT);
	}
	else						// Only after when complete pat table is received, pmt and pes are parsed for each program.
	{
		if (!m_pmts.complete)	// Search and parse pmt for each program:
		{
			pat_length = m_pat.list.size();
			for (int k=0; k<pat_length; k++)
			{
				if ( m_pat.list[k].second.first == pid && m_pat.list[k].first != 0)
				{
					if (!m_pat.list[k].second.second)
						parse_psi(buffer, j, (BYTE)TABLE_PMT);
				}
			}
		}
			
		int not_parsed_pes_count=0;		// Search and parse audio and video pes packets for each program:
		int ap,vp;
		for (UINT l=0; l<m_programes.size(); l++)
		{   
			stream_num=m_programes[l].second.size();
			for (int m=0; m<stream_num; m++ )
			{	
				if (!(m_programes[l].second[m].pes_supported))
					continue;
					
				if (!(m_programes[l].second[m].pes_parsed))
					not_parsed_pes_count+=1;
				else
					continue;						// ???? conditional continuation 
					
				if(m_programes[l].second[m].pid == pid)
					parse_pes(buffer,j,l,m,NULL,scrambled);  
			}
		}
		if (m_pmts.complete && not_parsed_pes_count==0 )
			m_all_pes_parsed=true;
	}
}

// psi parsing ************************************************************** //

// Psi tables are parsed by sections
void CParser::parse_psi(char * buf, int pos, BYTE psi_table) //pos=beginning of a ts packet
{
	psi_table_t * table;
	int offset;
	switch (psi_table)
	{
		case (BYTE) TABLE_PAT :
			table = (psi_table_t*)&m_pat;
			offset = 8;
			break;
		case (BYTE) TABLE_PMT :
			table =(psi_table_t*)&m_pmts;
			offset =12;
			break;
		case (BYTE) TABLE_SDT :
			table = (psi_table_t*)&m_sdt;
			offset =11;
			break;
		case (BYTE) TABLE_EIT :
			table = (psi_table_t*)&m_dmt_eit;
			offset = 14;
			break;
		case (BYTE) TABLE_NIT :
			 table = (psi_table_t*)&m_nit;
			 offset=8;
			break;

		default :
			return;
	}
	
	bool discard_prev=false;

	bool has_sect_start;
	int data_start;
	int section_start;
	bool has_payload;
	psi_section(buf,pos,&data_start,&section_start, &has_sect_start, &has_payload);

	if(!has_payload)
		return;

	int length=0;
    int fbyte=-1;						// forbidden byte
	if ( !has_sect_start ) 
	{
		if (table->first_part_received)	//Continue filling previus section.
		{
			length=MIN_PACKET_LENGTH-(data_start-pos);
			fbyte = findbyte(&(buf[data_start]),length,0xff);
			if (fbyte != -1)
				length = fbyte;
			if (length+table->section_position > table->section_length)
				length =  table->section_length- table->section_position;
			
			if (length >0)// Why can this happen ?
			{
				memcpy(&(table->current_section[table->section_position]), &(buf[data_start]),length);
				table->section_position+=length;
             }

			//check
			if (table->section_position>=table->section_length)
			{
				if (psi_table == TABLE_PAT)
					CParser::parse_pat_data();
				else if (psi_table == TABLE_PMT)
					CParser::parse_pmt_data();
				else if (psi_table == TABLE_SDT)
						parse_sdt_data();
				else if (psi_table == TABLE_EIT)
					parse_eit_data();
				else if (psi_table == TABLE_NIT)
					parse_nit_data();

				table->first_part_received =false; // start building new section
				table->parsed_sections_count+=1;
				if (table->parsed_sections_count>table->last_section_number)
				{
					table->complete=true;
				}
				table->has_changed=false;
			}
		}
	}
	else	// Section start
	{
		if (table->first_part_received && data_start==section_start )	// Previus section was not completed and parsed - reset builder
		{
			table->first_part_received=false;
			table->section_position=0;
		}

		BYTE version_number = (buf[section_start+5]&&0x3e)>>1;
		bool current_next_indicator = buf[section_start+5]& 0x1 ;

		if (table->complete)
		{
			if (table->version_number!=version_number && current_next_indicator)
			{
				table->has_changed =true;
				table->section_position=0;
				table->parsed_sections_count = 0;
				table->new_complete =true;
				table->complete = false;
			}
			else
			{
				table->new_complete =false;
				return;
			}
		}
		else
		{
		     if (table->first_part_received && table->version_number != version_number) // Table has changed in the middle of section parsing
			 {
			    table->has_changed =true;
				table->section_position=0;
				table->parsed_sections_count = 0;
			 }
		}
		
		BYTE table_id = (BYTE)buf[section_start]; 
		u_short section_length = ((buf[section_start+1] & 0xf) << 8) | ( (BYTE)buf[section_start+2]) ;
		section_length+=3;		// full section : crc32 + 3 bytes before 
		u_short transport_stream_id = ((buf[section_start+3] & 0xff) << 8) | (BYTE)(buf[section_start+4]) ;

		BYTE section_number = buf[section_start+6];
		BYTE last_section_number = buf[section_start+7];
	
		u_short PCR_PID=0;
		u_short program_info_length=0;
		u_short program_number=0;

		// table specific handling ======================================================================
		if ( psi_table == TABLE_PAT)
		{
			if (table_id!=0)
				return;
		}
		
		if (psi_table == TABLE_PMT)
		{
			PCR_PID = (buf[section_start+8]&0x1f)<<8 | (BYTE)buf[section_start+9];
			program_info_length = ((buf[section_start+10] & 0xf) <<8) | (BYTE)(buf[section_start+11]&0xff);
			program_number = ((buf[section_start+3] & 0xff) << 8) | (buf[section_start+4] & 0xff) ;
			((pmt_table_t*)table)->PCR=PCR_PID;
			((pmt_table_t*)table)->prog_info_len = program_info_length;
			((pmt_table_t*)table)->current_pmt_program = program_number;
		}
		if (psi_table == TABLE_SDT)
		{
			if(table_id != 0x42)
				return; // not SDT table
		}

		if (psi_table == TABLE_EIT)
		{
				if (table_id != 0x4e)
					return;	// only actual TS, present/following event information is supported
				// TODO: actual TS, event schedule information id: 0x50 - 0x5f
		}
		if (psi_table == TABLE_NIT)
		{
		    if (table_id != 0x40) // only actual network
				return;

			m_nit_net_descs.network_id = ((buf[section_start+3] & 0xff) <<8) | (BYTE)(buf[section_start+4]&0xff);
		}
		// ===============================================================================================

		if ( data_start!=section_start )
		{
			if (table->first_part_received)									// Continue filling last part of previus section.
			{
				length = section_start-data_start;
				fbyte = findbyte(&(buf[data_start]),length,0xff);
				if (fbyte != -1)
					length = fbyte;
				if (length+table->section_position > table->section_length) // Junk data between sections ???
					length = table->section_length- table->section_position;

				if (length >0) // Why can this happen ?
				{
					memcpy(&(table->current_section[table->section_position]), &(buf[data_start]),length);
					table->section_position+=length;
				}

				//last part of section received
				if (psi_table == TABLE_PAT)
					CParser::parse_pat_data();
				else if (psi_table == TABLE_PMT)
					CParser::parse_pmt_data();
				else if (psi_table == TABLE_SDT)
					parse_sdt_data();
				else if (psi_table == TABLE_EIT)
					parse_eit_data();
				else if (psi_table == TABLE_NIT)
					parse_nit_data();

				table->first_part_received =false; // start building new section
				table->parsed_sections_count+=1;
				table->has_changed=false;
				if (table->parsed_sections_count>table->last_section_number)
				{
					table->complete=true;
				}
			}
		}
		
		table->complete = false;
		table->section_length=section_length;
		table->last_section_number=last_section_number;
		table->table_id = table_id;
		table->section_number = section_number;
		table->version_number = version_number;
		
		table->section_position = 0;
		// Begin filling section buffer:
		length = MIN_PACKET_LENGTH-(section_start-pos);

		fbyte = findbyte(&(buf[data_start+offset]),length-offset,0xff);
		if (fbyte != -1)
			length = fbyte;
		
		length = min(length,section_length); // Just to be sure.

		int data_length = length-offset;
		if (data_length <0)
			data_length=0;

		memcpy(&(table->current_section[table->section_position]), &(buf[section_start+offset]),data_length);
		table->section_position+=data_length;
		table->first_part_received=true;

		if (length>=section_length)
		{
			if (psi_table == TABLE_PAT)
				CParser::parse_pat_data();
			else if (psi_table == TABLE_PMT)
				CParser::parse_pmt_data();
			else if (psi_table == TABLE_SDT)
					parse_sdt_data();
			else if (psi_table == TABLE_EIT)
					parse_eit_data();
			else if (psi_table == TABLE_NIT)
					parse_nit_data();

			table->first_part_received =false; // start building new section
			table->parsed_sections_count+=1;
			table->has_changed=false;
			if (table->parsed_sections_count>table->last_section_number)
			{
				table->complete=true;
			}

		}
	}
}

// Parse complete PAT section without headers
void CParser::parse_pat_data()
{
	//CAutoLock cAutoLock(&m_pLock);
	
	u_short program_number;
	u_short program_map_PID;

	int len = m_pat.section_length-12 ; //-offset(8) - crc32(4)
	for (int i=0; i < len; i=i+4)
	{
		program_number = ((m_pat.current_section[i] & 0xff) << 8) | (BYTE)m_pat.current_section[i+1];
		program_map_PID = ((m_pat.current_section[i+2] & 0x1f) << 8) | (BYTE)m_pat.current_section[i+3];
		m_pat.list.push_back(make_pair(program_number,make_pair(program_map_PID,false)));

		if (program_number ==0)
			m_nit_pid = program_map_PID;
	}
}


// Parse complete PMT section without headers
void CParser::parse_pmt_data()
{
	//CAutoLock cAutoLock(&m_pLock);
	
	vector<piddetails_t> v_details;
	int section_len = m_pmts.section_length-16; // -offset(12) - crc32 (4)

	char lang_code[4]={ 0x0, 0x0, 0x0, 0x0};
    u_short desc_len;
	UINT desc_tag;
	int i=0;
	for ( ;i<m_pmts.prog_info_len; )
	{
		CParser::descriptor_type(m_pmts.current_section,i,&desc_len,&desc_tag);
		if (desc_tag==10)
			CParser::iso_693_language_decsriptor(m_pmts.current_section,i+2,desc_len,lang_code);
		i=i+2+desc_len;
	}
	
	int j= i;
	u_short stream_type;
    u_short elementary_PID;
    u_short ES_info_length;

 	for ( ; j< i+section_len-m_pmts.prog_info_len; )
	{
		piddetails_t s_piddetails;
		ZeroMemory(&s_piddetails,sizeof(piddetails_bas_t));
		clear_videodet(&(s_piddetails.pes_vdet));
		clear_audiodet(&(s_piddetails.pes_adet));

		memset(lang_code,0,4);

		stream_type= (BYTE)m_pmts.current_section[j];
		elementary_PID= ((m_pmts.current_section[j+1]& 0x1f) << 8) | (BYTE)(m_pmts.current_section[j+2]);
		ES_info_length=((m_pmts.current_section[j+3] & 0xf) <<8) | (BYTE)(m_pmts.current_section[j+4]);
		j=j+5;
		int k=j;
		
		for (;j< k+ES_info_length; )
		{
			CParser::descriptor_type(m_pmts.current_section,j,&desc_len,&desc_tag);
			
			switch (desc_tag)
			{
				case 10:	// language descriptor
					CParser::iso_693_language_decsriptor(m_pmts.current_section,j+2,desc_len,lang_code);
					break;
				case 0x56:	// ttx_ebu data
					CParser::teletext_descriptor(m_pmts.current_section,j+2,desc_len,&(s_piddetails.teletext_descriptors_v));
					break;
				case 0x59:	//dvb subs
					CParser::subtitling_descriptor(m_pmts.current_section,j+2,desc_len,&(s_piddetails.subtitling_descriptors_v));
					break;
				case 0x6A:
					if (stream_type==0x06)
						s_piddetails.private_stream_type= PRIVATE_AC3;
					break;
			}
			
			j=j+2+desc_len;
		}
        
		s_piddetails.pid=elementary_PID;
		s_piddetails.PCR_PID=m_pmts.PCR;
		s_piddetails.stream_type=stream_type;
		memcpy(s_piddetails.language,lang_code,4);
		s_piddetails.pes_parsed=false;
		if (is_pes_type_supported(stream_type))
			s_piddetails.pes_supported=true;
		else // Some private PES parsing may be supported by this parser
		{
			if (s_piddetails.private_stream_type == PRIVATE_AC3)
				s_piddetails.pes_supported=true;
			else
				s_piddetails.pes_supported=false;
		}
	
		if (!m_pmts.first_complete_table) // Initial parser
		    v_details.push_back(s_piddetails);
		else                     
		{
			// Monitor for changes
			if (m_pmts.parsed_sections_count==0)
				m_dmt_last_PMT_of_curr_prog.clear();
			m_dmt_last_PMT_of_curr_prog.push_back(s_piddetails); 
		}
	}
	
	if (!m_pmts.first_complete_table)
	{
		m_programes.push_back(make_pair(m_pmts.current_pmt_program,v_details));
	
		int patlen = m_pat.list.size();
		int notparsed=0;

		for (int a=0; a<patlen; a++)					// Find correct program in pat table and set parsed flag to true:
		{
			if (m_pat.list[a].first == m_pmts.current_pmt_program || m_pat.list[a].first==0) // NIT pid is also listed in pat prog=0 
				m_pat.list[a].second.second=true;											 // PMT table for program parsed.
		
			else if (m_pat.list[a].second.second==false)
					notparsed++;
		}

		if (notparsed==0)
		{
			m_pmts.complete=true;					// Pmt for all programes complete
			m_pmts.first_complete_table=true;
		}
	}
}

void CParser::parse_sdt_data()
{
	bool has_service_descriptor=false;
	int len = m_sdt.section_length-15;	//offset (11) - crc32 (4)
	int i=0;
	for (; i< len; )
	{
		u_short service_id = ((m_sdt.current_section[i] & 0xff)<<8) | (BYTE) m_sdt.current_section[i+1];
		u_short descriptors_loop_length = (m_sdt.current_section[i+3] & 0xf) << 4  | (BYTE) m_sdt.current_section[i+4];
		
		int j=0;
		for (; j<descriptors_loop_length;)
		{
			u_short desc_len;
			UINT tag;
			descriptor_type(m_sdt.current_section,i+5+j,&desc_len,&tag);
			if (tag == 0x48 && !global_service_list_parsed)
			{	
				services_descriptor(&(m_sdt.current_section[i+7+j]),desc_len,service_id); //This is the only descriptor we are interested in SDT !!!!
				has_service_descriptor=true;
			}
			j+=desc_len+2;
		}

		i+=descriptors_loop_length+5;
	}

	// ========================================================
	if (has_service_descriptor)
		global_service_list_parsed=true;

	// =========================================================
}

void CParser::parse_eit_data()
{
	int len = m_dmt_eit.section_length - 18; // 14 +crc (4)
	int i=0;
	for (; i< len; )
	{
		u_short event_id = ((m_dmt_eit.current_section[i] & 0xff)<<8) | (BYTE) m_dmt_eit.current_section[i+1];
		u_short mjd = ((m_dmt_eit.current_section[i+2] & 0xff)<<8) | (BYTE) m_dmt_eit.current_section[i+3];
		UINT utc_bcd =((m_dmt_eit.current_section[i+4] & 0xff)<<16) | ((m_dmt_eit.current_section[i+5] & 0xff)<<8) | (BYTE) m_dmt_eit.current_section[i+6];
		UINT duration_bcd = ((m_dmt_eit.current_section[i+7] & 0xff)<<16) | ((m_dmt_eit.current_section[i+8] & 0xff)<<8) | (BYTE) m_dmt_eit.current_section[i+9];
		u_short descriptors_loop_length = ((m_dmt_eit.current_section[i+9]&0xf)<<4) |  (BYTE) m_dmt_eit.current_section[i+10];
		
		short_event_desc_t desc;
		desc.date_mjd =mjd;
		desc.time_utcbcd=utc_bcd;
		desc.duration_bcd=duration_bcd;

		int j=0;
		for (; j<descriptors_loop_length;)
		{
			u_short desc_len;
			UINT tag;
			descriptor_type(m_dmt_eit.current_section,i+12+j,&desc_len,&tag);
			if (tag == 0x4D ) // short event descriptor
			{	
				short_event_descriptor(m_dmt_eit.current_section,i+14+j,event_id,&desc,m_dmt_eit.has_changed);
			}
			j+=desc_len+2;
		
		}
		i+=descriptors_loop_length+12;
	}
}

void CParser::parse_nit_data()
{
	int i = 0;
	int len = m_pat.section_length-12 ; //-offset(8) - crc32(4)

	u_short network_descriptors_length = ((m_nit.current_section[i] & 0xf)<<8) | (BYTE) m_nit.current_section[i+1];
	i+=2;
    int j=0;
	for ( ; j<network_descriptors_length;)
	{
		u_short desc_len;
		UINT tag;
		descriptor_type(m_nit.current_section,i+j,&desc_len,&tag);
			
		// Parse descriptor
		switch (tag)
		{
		case 0x40:
			//m_nit_descs
			if (!m_nit_net_descs.has_network_name_descriptor )
			{
				parse_network_name_desc(&m_nit.current_section[i+j],desc_len,&m_nit_net_descs.network_name_descriptor);
				m_nit_net_descs.has_network_name_descriptor =true;
			}
			break;

		case 0x41 :
			if (!m_nit_net_descs.has_service_list_descriptor)
			{
			    parse_service_list_desc(&m_nit.current_section[i+j],desc_len,& m_nit_net_descs.service_list_descriptors);
				m_nit_net_descs.has_service_list_descriptor=true;
			}

			break;

		case 0x43 :
			if (!m_nit_net_descs.has_satellite_delivery_system_descriptor)
			{
			    parse_satellite_delivery_system_desc(&m_nit.current_section[i+j],desc_len,& m_nit_net_descs.satellite_delivery_system_descriptor);
				m_nit_net_descs.has_satellite_delivery_system_descriptor=true;
			}

			break;

		case 0x44 :
			if (!m_nit_net_descs.has_cable_delivery_system_descriptor)
			{
			    parse_cable_delivery_system_desc(&m_nit.current_section[i+j],desc_len,& m_nit_net_descs.cable_delivery_system_descriptor);
				m_nit_net_descs.has_cable_delivery_system_descriptor=true;
			}

			break;

		case 0x5a :
			if (!m_nit_net_descs.has_terrestial_delivery_system_descriptor)
			{
			    parse_terrestial_delivery_system_desc(&m_nit.current_section[i+j],desc_len,& m_nit_net_descs.terrestial_delivery_system_descriptor);
				m_nit_net_descs.has_terrestial_delivery_system_descriptor=true;
			}

			break;		
		
		default:
			break;
		}

		j+= desc_len+2;
	}
	i+=j;

	USHORT transport_stream_loop_length = (m_nit.current_section[i]&0xf)<<8 | (BYTE) m_nit.current_section[i+1];
	i+=2;
    int k=0;
	for (; k< transport_stream_loop_length;)
	{
	    USHORT transport_stream_id = (m_nit.current_section[i+k]&0xff)<<8 | (BYTE) m_nit.current_section[i+k+1];
		USHORT original_network_id = (m_nit.current_section[i+k+2]&0xff)<<8 | (BYTE) m_nit.current_section[i+k+3];
	    USHORT transport_descriptor_len = (m_nit.current_section[i+k+4]&0xf)<<8 | (BYTE) m_nit.current_section[i+k+5];

		//add to vector even if we don't support descriptor
		nit_ts_descriptors_t ntsd;
		ntsd.transport_stream_id = transport_stream_id;
		ntsd.original_network_id = original_network_id;
  		
		int pos = i+k+6;      
		int l=0;
		for ( ; l< transport_descriptor_len;)
		{
		    u_short desc_len;
		    UINT tag;
		    descriptor_type(m_nit.current_section,pos+l,&desc_len,&tag);
		
			switch (tag)
			{
		case 0x40:
				parse_network_name_desc(&m_nit.current_section[pos+l],desc_len,&ntsd.network_name_descriptor);
				ntsd.has_network_name_descriptor =true;
			break;

		case 0x41 :
     		    parse_service_list_desc(&m_nit.current_section[pos+l],desc_len,&ntsd.service_list_descriptors);
			break;

		case 0x43 :
			    parse_satellite_delivery_system_desc(&m_nit.current_section[pos+l],desc_len,& ntsd.satellite_delivery_system_descriptor);
				ntsd.has_satellite_delivery_system_descriptor=true;
			break;

		case 0x44 :
			    parse_cable_delivery_system_desc(&m_nit.current_section[pos+l],desc_len,& ntsd.cable_delivery_system_descriptor);
				ntsd.has_cable_delivery_system_descriptor = true;
			break;

		case 0x5a :
			    parse_terrestial_delivery_system_desc(&m_nit.current_section[pos+l],desc_len,& ntsd.terrestial_delivery_system_descriptor);
				ntsd.has_terrestial_delivery_system_descriptor=true;
			break;	
			
			default:
				break;
			}

			l+=desc_len+2;
		}
		m_nit_ts_descs.push_back(ntsd);
		k+= 6+transport_descriptor_len; 
	}
}

void CParser::descriptor_type(char* buf, int pos, u_short* desc_len, UINT* tag)
{
	*desc_len = (BYTE)buf[pos+1];
    *tag=(BYTE)buf[pos];

	return;
}

void CParser::psi_section(char* buffer, int pos, int* data_start, int* sect_start, bool* has_sect_start, bool* has_payload)
{
	int start=pos+4;
	BYTE adaption_field = adaption_field_control(buffer, pos);
	if (adaption_field == 0x2 || adaption_field == 0x3  )
			start +=  (int)adaption_field_length(buffer, pos)+1;
	*data_start=start;
	
	if (adaption_field==0x01 || adaption_field==0x03 )
		*has_payload=true;
	else
		*has_payload=false;

	if (payload_unit_start(buffer,pos))
	{
		int pointer_field_start=start;
		*data_start= pointer_field_start + 1;
		*sect_start = pointer_field_start + 1 + (BYTE)buffer[pointer_field_start];
		*has_sect_start=true;
	}
	else
		*has_sect_start=false;
}

bool CParser::payload_unit_start(char*  buffer, int pos)
{
	if ( (buffer[pos+1] & 0x40) >> 6 )
		return true;
	else
		return false;
}


BYTE CParser::adaption_field_control(char *buffer, int pos)
{
	return (buffer[pos+3] & 0x30)>>4;
}

BYTE CParser::adaption_field_length(char *buffer, int pos)
{
	return (BYTE)buffer[pos+4];
}

// ************************************************************** //

// pes parsing
// ************************************************************** //
// For DS filters to connect, some basic data has to be parsed before, especially for sound filters !
//

HRESULT CParser::parse_pes(char * buf, int pos, int prog_index, int stream_index, pes_basic_info_t* pesinfo, byte scrambled) // pos = beginning of ts packet
{
	bool use_builder=false;
	char* builder_buf;
	int*  builder_pos;
	bool* build;

	if (pesinfo == NULL )
	{
		// Build complete ES for initial parsing if headers are not found at the beginning of each ES.
	   	if (m_programes[prog_index].second[stream_index].stream_type == 0x1b)
		{
			build = &m_nal_build;
			builder_buf = m_nal_builder;
			builder_pos = &m_nal_pos;
		}
		else if (m_programes[prog_index].second[stream_index].stream_type == 0x1 || m_programes[prog_index].second[stream_index].stream_type == 0x2)
		{
		   	build = &m_mpeg_build;
			builder_buf = m_mpeg_builder;
			builder_pos = &m_mpeg_pos;
		}
		else
		{
			build = &use_builder; 
		}
	}
	
	BYTE adaption_field = adaption_field_control(buf, pos);
	int start = pos +4;
	
	BYTE adaptation_field_length;
	if (adaption_field==0x02 || adaption_field==0x03)
	{
		adaptation_field_length = adaption_field_length(buf, pos);
		if (adaptation_field_length>183)
			return E_FAIL;
		start +=  adaptation_field_length+1;
	}

	if ( !(adaption_field==0x01 || adaption_field==0x03) )
		return E_FAIL;	// no payload

	if (!payload_unit_start(buf,pos) )
	{
	   if (pesinfo != NULL)
	   {
		   pesinfo->frame_start=start;
		   pesinfo->unit_start=false;
	   }
	   else
	   {
			if (*build)
			{
				if (*builder_pos<NAL_BUILDER_MAX_LEN)
				{
					int pes_len = (pos + MIN_PACKET_LENGTH)-start;
					memcpy(&(builder_buf[*builder_pos]),&(buf[start]), pes_len);
					*builder_pos+=pes_len;
				}
				else
				{
					*build=false;
					*builder_pos=0;
				}
			}
	   }
	  return S_OK; // Not the beginning of pes packet. No PES headers .
	}

	// Second check
	DWORD packet_start_code_prefix = ( ((buf[start]&0xff) << 16) | ((buf[start+1]&0xff) << 8) | (BYTE)(buf[start+2]) );
	if (packet_start_code_prefix!=0x000001)
	{
	   if (pesinfo != NULL)
	   {
		   pesinfo->frame_start=start;
		   pesinfo->unit_start=false;
	   }
		return S_OK; 
	}

	if (pesinfo != NULL)
		pesinfo->unit_start=true;

	BYTE stream_id = buf[start+3]; 
	u_short pes_packet_length= (BYTE)buf[start+4]<<8 |(BYTE)buf[start+5]; 

	if (stream_id !=PROGRAM_STREAM_MAP &&
		stream_id != PADDING_STREAM &&
		stream_id != PRIVATE_STREAM_2 &&
		stream_id != ECM_STREAM &&
		stream_id != EMM_STREAM &&
		stream_id != PROGRAM_STREAM_DIRECTORY &&
		stream_id != DSMCC_STREAM &&
		stream_id != ITUTRecH222typeE
		)
	{
		
		BYTE data_alignment_indicator = (buf[start+6]&0x7)>>2;
		BYTE PES_header_data_length= (BYTE)buf[start+8];
		u_short PTS_DTS_flags = (buf[start+7]&0x80)>>6 | (buf[start+7]&0x40 >>6);
		BYTE ESCR_flag = (buf[start+7]&0x20)>>5;
		BYTE ES_rate_flag = (buf[start+7]&0x10)>>4;
		BYTE DSM_trick_mode_flag= (buf[start+7]&0x8)>>3;
		BYTE additional_info_copy_flag = (buf[start+7]&0x4)>>2;
		BYTE PES_CRC_flag = (buf[start+7]&0x2)>>1;
		BYTE PES_extension_flag = buf[start+7]&0x1;

		int frame_start= start+9;
		 if (  PES_header_data_length >= MIN_PACKET_LENGTH -9)
			return E_FAIL; // No payload, stuffing bytes
		frame_start+=PES_header_data_length; // Optional plus stuffing bytes
		u_short es_length=pes_packet_length - (frame_start-start-6 );
		u_short part_es_len =pos + MIN_PACKET_LENGTH - frame_start;
		if(pesinfo!=NULL)
		{
			pesinfo->frame_start=frame_start;

			if (pes_packet_length==0)
				pesinfo->es_length=0;
			else
				pesinfo->es_length = es_length;
		
			if (PTS_DTS_flags==0x2)
			{
				BYTE PTS32_30 = (buf[start+9]&0xe)>>1;
				BYTE PTS29_22 = (BYTE)(buf[start+10]);
				BYTE PTS21_14 =	(buf[start+11]&0xfe)>>1;
				BYTE PTS13_6 =  (BYTE)(buf[start+12]);
				BYTE PTS6_0 = (buf[start+13]&0xfe)>>1;
				
				pesinfo->has_PTS=true;
				pesinfo->has_DTS=false;
				ULONGLONG PTS =((ULONGLONG)PTS32_30<<30) + ((ULONGLONG)PTS29_22 << 22) + ((ULONGLONG)PTS21_14<<15) +( (ULONGLONG)PTS13_6<<7) + (ULONGLONG)PTS6_0;
				pesinfo->PTS=  (LONGLONG)PTS;
				pesinfo->DTS=0;

			}
			else if (PTS_DTS_flags == 0x3)
			{
				BYTE PTS32_30 = (buf[start+9]&0xe)>>1;
				BYTE PTS29_22 = (BYTE)(buf[start+10]);
				BYTE PTS21_14 =	(buf[start+11]&0xfe)>>1;
				BYTE PTS13_6 =  (BYTE)(buf[start+12]) ;
				BYTE PTS6_0 = (buf[start+13]&0xfe)>>1;

				BYTE DTS32_30 = (buf[start+14]&0xe)>>1;
				u_short DTS29_15 = ((BYTE)(buf[start+15])<<7) | ((buf[start+16]&0xfe)>>1);
				u_short DTS14_0 = ((BYTE)(buf[start+17])<<7) | ((buf[start+18]&0xfe)>>1);
				
				pesinfo->has_PTS=true;
				pesinfo->has_DTS=true;			
				ULONGLONG PTS=((ULONGLONG)PTS32_30<<30) + ((ULONGLONG)PTS29_22 << 22) + ((ULONGLONG)PTS21_14<<15) +( (ULONGLONG)PTS13_6<<7) + (ULONGLONG)PTS6_0;
				pesinfo->PTS=  (LONGLONG)PTS ;
				
			}
		}

     	// page 66
		if (pesinfo == NULL && scrambled==0) // initial parser
		{
			if (m_programes[prog_index].second[stream_index].stream_type == 0x03 ||  
				m_programes[prog_index].second[stream_index].stream_type == 0x04)
			{
				CParser::mpeg_audio_header(buf,frame_start,pos+MIN_PACKET_LENGTH,prog_index,stream_index); 
			}

			else if (m_programes[prog_index].second[stream_index].stream_type == 0x81)
			{
				CParser::dolby_audio_header(buf,frame_start,prog_index,stream_index);
			}

			else if (m_programes[prog_index].second[stream_index].stream_type == 0x11) // 144496-3 LATM
			{
				CParser::iso_iec_14496_3_header(buf,frame_start,pos+MIN_PACKET_LENGTH,prog_index,stream_index);
			}

			else if (m_programes[prog_index].second[stream_index].stream_type == 0x6) // private stream
			{
				if (m_programes[prog_index].second[stream_index].private_stream_type == PRIVATE_AC3) // cheat
				{
					m_programes[prog_index].second[stream_index].stream_type = PRIVATE_AC3;
					CParser::dolby_audio_header(buf,frame_start,prog_index,stream_index);
				}
			}

			// #################################################
			// For video parsing more data needs to be collected.
			else if (m_programes[prog_index].second[stream_index].stream_type == 0x1b || m_programes[prog_index].second[stream_index].stream_type == 0x1 || m_programes[prog_index].second[stream_index].stream_type == 0x2)
			{
				bool found=false;
				HRESULT result;
				if (*build && *builder_pos>0)	// Check complete ES.
				{
					if (m_programes[prog_index].second[stream_index].stream_type == 0x1b )
						result = CParser::h264_video(builder_buf,0,prog_index,stream_index,*builder_pos);
					else if (m_programes[prog_index].second[stream_index].stream_type == 0x1 || m_programes[prog_index].second[stream_index].stream_type == 0x2)
						result = CParser::mpeg_video(builder_buf,0,prog_index,stream_index,*builder_pos);

					if (result==S_OK)
					{
						*build=false;
						found=true;
					}
					*builder_pos=0;				// Reset ES builder position
				}

				if (!found)
				{
					// Check at the beginning of a new ES !
					if (m_programes[prog_index].second[stream_index].stream_type == 0x1b )
						result = CParser::h264_video(buf,frame_start,prog_index,stream_index,pos+MIN_PACKET_LENGTH);
					else if (m_programes[prog_index].second[stream_index].stream_type == 0x1 || m_programes[prog_index].second[stream_index].stream_type == 0x2)
						result =  CParser::mpeg_video(buf,frame_start,prog_index,stream_index,pos+MIN_PACKET_LENGTH);

					if (result!= S_OK)
					{
						// Start building complete ES:
						memcpy(&(builder_buf[*builder_pos]),&(buf[frame_start]),part_es_len);
						*builder_pos+=part_es_len;
						*build=true;
					}
					else
						*build=false;
				}
			}
			// #################################################

		}
	}

	else if (stream_id==PROGRAM_STREAM_MAP ||
		stream_id == PRIVATE_STREAM_2 ||
		stream_id == ECM_STREAM ||
		stream_id == EMM_STREAM ||
		stream_id == PROGRAM_STREAM_DIRECTORY ||
		stream_id == DSMCC_STREAM ||
		stream_id == ITUTRecH222typeE
		)
	{
		// Data starts with frame_start
		// TODO: support for private streams: 
		return E_FAIL;

	}
	else if (stream_id == PADDING_STREAM)
	{
		return E_FAIL;
	}

	return S_OK;
}


//ES: mpeg audio ============================================================================
void CParser::mpeg_audio_header(char* buf, int pos, int ts_end, int prog_index, int stream_index)
{
	if (m_programes[prog_index].second[stream_index].pes_parsed)
		return;
	
	u_short frame_sync = ( (buf[pos]&0xff) <<3) | (buf[pos+1]&0xe0)>>5  ;
	if (frame_sync != 0x7ff)	// Frame not aligned, try to find it
	{
		bool found=false;
		pos+=1;
		for (;pos<ts_end;)
		{
			frame_sync =  ( (buf[pos]&0xff) <<3) | (buf[pos+1]&0xe0)>>5 ;
			if (frame_sync  == 0x7ff )
			{
				found=true;
				break;
			}
			pos+=1;
		}
		if (!found)
			return;
	}
	
	m_programes[prog_index].second[stream_index].has_data=true;

	bool posible_error=false;

	u_short version= (buf[pos+1]&0x18)>>3;
	u_short layer= 3- ( (buf[pos+1]&0x6)>>1 );  // layer_minus_one
	u_short samp_freq_info=(buf[pos+2]&0xc)>>2;
	u_short bitrate_info = (buf[pos+2]&0xf0)>>4;
	u_short channel_mode = (buf[pos+3]&0xc0)>>6;
	u_short channels=2;
	UINT samp_freq=0;
	UINT bitrate=0;
	switch(version)
	{
	case 0x0: //v2.5
		bitrate = MpegV2[bitrate_info][layer];
    	if (samp_freq_info==0x0)
			samp_freq = 11025;
		else if (samp_freq_info==0x1)
			samp_freq = 12000;
		else if (samp_freq_info==0x2)
			samp_freq = 8000;
		else
			posible_error=true;
		break;
	case 0x2: // v2
		bitrate = MpegV2[bitrate_info][layer];
		if (samp_freq_info==0x0)
			samp_freq = 22050;
		else if (samp_freq_info==0x1)
			samp_freq = 24000;
		else if (samp_freq_info==0x2)
			samp_freq = 16000;
		else
			posible_error=true;
		break;
	case 0x3: //v1
		bitrate = MpegV1[bitrate_info][layer];
		if (samp_freq_info==0x0)
			samp_freq = 44100;
		else if (samp_freq_info==0x1)
			samp_freq = 48000;
		else if (samp_freq_info==0x2)
			samp_freq = 32000;
		else
			posible_error=true;
		break;
	}

	if (channel_mode==0x3) //mono
		channels=1;

	if(!posible_error)
	{
		m_programes[prog_index].second[stream_index].pes_adet.sampfreq=samp_freq;
		m_programes[prog_index].second[stream_index].pes_adet.mpeg_layer=layer+1;
		m_programes[prog_index].second[stream_index].pes_adet.mpeg_version=version;
		m_programes[prog_index].second[stream_index].pes_adet.channels=channels;
		m_programes[prog_index].second[stream_index].pes_adet.bitrate=bitrate;
		m_programes[prog_index].second[stream_index].pes_parsed=true;
	}
}

//ES: dolby audio ===========================================================================
void CParser::dolby_audio_header(char* buf, int pos, int prog_index, int stream_index)
{
	if (m_programes[prog_index].second[stream_index].pes_parsed)
		return;
	
	u_short frame_sync = ( (buf[pos]&0xff) <<8 ) | (BYTE)buf[pos+1]  ;
	if (frame_sync != 0xB77)
		return;

	m_programes[prog_index].second[stream_index].has_data=true;

	UINT samp_freq=44100;
	UINT bitrate=0;
	
	BYTE fs_cod = (buf[pos+4]&0xc0)>>6;
	BYTE frmsize_cod = buf[pos+4]&0x3f;

	if (fs_cod<=0x2)
		samp_freq = DolbySampFreq[fs_cod];
	if (frmsize_cod<38)
		bitrate = Dolby_frmscod[frmsize_cod][0];

	//acmod:
	u_short num_of_channels=2;
	BYTE num_of_channels_info=(buf[pos+6]&0xe0)>>5;
	switch(num_of_channels_info)
	{
		case 0:
		case 2:
			num_of_channels=2;
			break;
		case 1:
			num_of_channels=1;
			break;
		case 3:
		case 4:
			num_of_channels=3;
			break;
		case 5:
		case 6:
			num_of_channels=4;
			break;
		case 7:
			num_of_channels=5;
			break;
	}

	m_programes[prog_index].second[stream_index].pes_adet.sampfreq=samp_freq;
	m_programes[prog_index].second[stream_index].pes_adet.channels=num_of_channels;
	m_programes[prog_index].second[stream_index].pes_adet.bitrate=bitrate;
	m_programes[prog_index].second[stream_index].pes_adet.mpeg_layer=0;
	m_programes[prog_index].second[stream_index].pes_adet.mpeg_version=0;
	m_programes[prog_index].second[stream_index].pes_parsed=true;
}

//ES mpeg LATM
void CParser::iso_iec_14496_3_header(char* buf, int pos, int ts_end, int prog_index, int stream_index)
{
	if (m_programes[prog_index].second[stream_index].pes_parsed)
		return;
	
	u_short frame_sync = ( (buf[pos]&0xff) <<3) | (buf[pos+1]&0xf8)>>5  ;
	
	if (frame_sync == 0x2b7)	
	{
		m_programes[prog_index].second[stream_index].has_data=true;
	   
		u_short audioMuxLengthBytes = ( (buf[pos+1]&0x1f) <<5 ) | (BYTE)buf[pos+2];
		aacAudioMuxElement(true, &(buf[pos+3]),&(m_programes[prog_index].second[stream_index].pes_adet));
		
		if (m_programes[prog_index].second[stream_index].pes_adet.sampfreq!=0)
			m_programes[prog_index].second[stream_index].pes_parsed=true;
	}

}

void CParser::aacAudioMuxElement(bool muxConfigPresent, char* buf, pesaudiodet_t* ad)
{
	int pos=0;
	BYTE offset=0;
	if (muxConfigPresent)
	{
		BYTE useSameStreamMux = read_bit(buf,&pos,&offset);
		if (!useSameStreamMux)
		{
			aacStreamMuxConfig(buf,&pos,&offset,ad);
		}
	}
}
void CParser::aacStreamMuxConfig(char* buf,int* pos, BYTE* offset, pesaudiodet_t* ad)
{
	BYTE audioMuxVersionA;
	UINT taraBufferFullness;
	BYTE audioMuxVersion = read_bit(buf,pos,offset);
	if (audioMuxVersion==1)
		audioMuxVersionA = read_bit(buf,pos,offset);
	else
		audioMuxVersionA=0;

	if (audioMuxVersionA == 0)
	{
		if (audioMuxVersion ==1)
			taraBufferFullness = LatmGetValue(buf,pos,offset);
		
		BYTE streamCnt=0;
		BYTE allStreamsSameTimeFraming = read_bit(buf,pos,offset);
		BYTE numSubFrames = read_bits(buf,pos,offset,6);
		BYTE numProgram = read_bits(buf,pos,offset,4);

		BYTE  useSameConfig=0;
		
		for (BYTE prog =0; prog <= numProgram; prog++)
		{
			BYTE numLayer = read_bits(buf,pos,offset,3);
			for (BYTE lay=0; lay <=numLayer; lay++)
			{
			    //
		        //
			    if (prog == 0 && lay==0)
					useSameConfig =0;
				else
					useSameConfig =read_bit(buf,pos,offset);
				if (!useSameConfig)
				{
					if (audioMuxVersion ==0)
					{
						aacSpecificConfig(buf,pos,offset,ad);
					}
					else
					{
						UINT ascLen = LatmGetValue(buf,pos,offset);
						ascLen -= aacSpecificConfig(buf,pos,offset,ad);
						move_for_bits(pos,offset,ascLen);
						break; // aacAudioSpecificCoding function NOT COMPLETE !
					}
				}
			}

			break; // Parse just first program. 
			//
			//
		}
	}
}

// This function MUST return number of bits read for aacStreamMuxConfig !!!
UINT CParser::aacSpecificConfig(char* buf,int* pos, BYTE* offset, pesaudiodet_t* ad)
{
	UINT samp_freq;
	UINT ext_samp_freq;
	BYTE audioObjectType = aacGetAudioObjectType(buf,pos,offset);
	BYTE samplingFrequencyIndex = read_bits(buf,pos,offset,4);
	if (samplingFrequencyIndex==0xf)
		samp_freq=read_bits(buf,pos,offset,24);
	else 
		samp_freq = AACSampFreq[samplingFrequencyIndex];

	BYTE channelConfiguration = read_bits(buf,pos,offset,4);

	bool sbrPresentFlag= false;
    BYTE extensionAudioObjectType;
	if (audioObjectType == 5)
	{
		extensionAudioObjectType = audioObjectType;
		sbrPresentFlag = true;
		BYTE extensionSamplingFrequencyIndex = read_bits(buf,pos,offset,4);
		if (extensionSamplingFrequencyIndex==0xf)
			ext_samp_freq=read_bits(buf,pos,offset,24);
		audioObjectType = aacGetAudioObjectType(buf,pos,offset);
	}
	else 
		extensionAudioObjectType =0;

	switch (audioObjectType)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
		case 7:
		case 17:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:

		break;

		case 8:

			break;

		case 9:

			break;

		case 12:

			break;

		case 13:
		case 14:
		case 15:
		case 16:
			break;

		case 24:
			break;

		case 25:
			break;

		case 26:
		case 27:

			break;

		case 28:
			break;

		case 32:
		case 33:
		case 34:
			break;

		case 35:
			break;

		default:
			break;

	}

	switch (audioObjectType)
	{
		case 17:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:

		break;
	}

	switch (channelConfiguration)
	{
		case 7:
			channelConfiguration = 8;
		
	    default:
		   break;
	}

		ad->sampfreq=samp_freq*2;            // Cheating - this is true only for HE-AAC. TODO: parse profile
		ad->mpeg_layer=0;
		ad->mpeg_version=0;
		ad->channels=channelConfiguration;
		ad->bitrate=0;

	return 0;
}

BYTE CParser::aacGetAudioObjectType(char* buf,int* pos, BYTE* offset)
{
	BYTE audioObjectType = read_bits(buf,pos,offset,5);
	if (audioObjectType ==31)
	{
		audioObjectType =  32 + read_bits(buf,pos,offset,6);
	}
	return audioObjectType;
}

//ES: mpeg video ==============================================================================
// This overloaded function is called only by INITIAL PARESER
HRESULT CParser::mpeg_video(char *buf, int pos, int prog_index, int stream_index, int ts_end)
{
	HRESULT hr = E_FAIL;
	int pict_hdr_len =0;

	for (int i = pos; i< ts_end; i++)
	{
		if ( !((BYTE)buf[i]==0x00 && (BYTE)buf[i+1]==0x00 && (BYTE)buf[i+2]==0x01) )  
			continue;

		if ( (BYTE)buf[i+3]==0xb3 )
		{
		   i+=mpeg_video_sequence_header(buf,i,prog_index,stream_index,ts_end,NULL);
		}

		else if ( (BYTE)buf[i+3]==0x0  )
		{
			if (m_programes[prog_index].second[stream_index].pes_parsed) // Check picture header after sequence header [pes_parsed=true]
			{
				hr = mpeg_video_picture_header(buf,i,prog_index,stream_index,ts_end,NULL,&pict_hdr_len);
				i+=pict_hdr_len;
			}
			else
			{
				// Copy header and parse it later after sequence header is received
				if (!m_pes[prog_index][stream_index].mpeg2_video_picture_header_received)
				{
					if (find_pict_coding_ext(&(buf[pos]),min(20,ts_end-pos))!=-1)
					{
						pict_hdr_len=min(MAX_MPEG_PICT_HEADER_SIZE,ts_end-pos);
						BYTE mappos = m_pes[prog_index][stream_index]. mpeg2_video_picture_header_last_map_pos;
						memcpy(&(m_first_mpeg_pict_hdr[mappos]),&(buf[i]),pict_hdr_len);
						m_pes[prog_index][stream_index].mpeg2_video_picture_header = m_first_mpeg_pict_hdr[mappos];
						m_pes[prog_index][stream_index]. mpeg2_video_picture_header_last_map_pos+=1;
						m_pes[prog_index][stream_index].mpeg2_video_picture_header_received=true;

						i+=pict_hdr_len;
					}
				}
			}
		}

		else if ((BYTE)buf[i+3]==0xb2 )	// user data- search for afd start code
		{
			if (buf[i+4]==0x44 && buf[i+5]==0x54 && buf[i+6]==0x47 && buf[i+7]==0x31 )
			{
				i+=afd_mpeg(buf,i+8,prog_index,stream_index,ts_end,NULL);
			}
		}
	}
	return hr; // Return S_OK only if sequence header AND picture extended header were found.
}

// Parse full mpeg2 ES, so we can count fields
// This overloaded function is called only by DEMUX THREAD
void CParser::mpeg_video(char* ES_buf, int len, mpeg2_ext_info_t* full_info)
{
	mpeg2_ext_info_t pes_info ; 
	ZeroMemory(&pes_info,sizeof(mpeg2_ext_info_t));
	int pict_hdr_len=0;

	// Try to get  info from initial parser if pict cod. ext. isn't received by now.
	if (!m_dmt_pictcod_received)
	{
		if (m_cur_prog.vid_prog_index!=-1 && m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].pict_cod!=NULL)
		{
			m_dmt_pict_cod.fields = m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].pict_cod->fields;
			m_dmt_pictcod_received=true;
		}
	}
	if (m_dmt_pict_cod.fields==0)
		m_dmt_pict_cod.fields=2;

	full_info->sample_fields = 0;
	
	//  Search for video_picture start codes:
	for (int i=0; i<len; i++)
	{	
		// Sequnce header start code
       if ((BYTE)ES_buf[i]==0x00 && (BYTE)ES_buf[i+1]==0x00 && (BYTE)ES_buf[i+2]==0x01 && (BYTE)ES_buf[i+3]==0xb3)
	   {
		  i+=mpeg_video_sequence_header(ES_buf,i,0,0,len,&pes_info);		   
		  full_info->video_arx = pes_info.video_arx;
		  full_info->video_ary = pes_info.video_ary;
		  full_info->video_height = pes_info.video_height;
		  full_info->video_width = pes_info.video_width;

	   }
		//  Video_picture start code
		else if ( (BYTE)ES_buf[i]==0x00 && (BYTE)ES_buf[i+1]==0x00 && (BYTE)ES_buf[i+2]==0x01 && (BYTE)ES_buf[i+3]==0x00 ) 
		{
			mpeg_video_picture_header(ES_buf,i,0,0,len,&pes_info,&pict_hdr_len);         

		   full_info->sample_fields+=m_dmt_pict_cod.fields;											// Number of "fields" in sample
		   
		   if ( pes_info.is_key_frame && !full_info->is_key_frame)
		   {
				full_info->is_key_frame=true;														// Sample contains at least one key frame.
				full_info->key_frame_pos = max(0,full_info->sample_fields-m_dmt_pict_cod.fields);   // First field in sample that is key field.
		   }
		   i+=pict_hdr_len;
       }

		else if ((BYTE)ES_buf[i]==0x00 && (BYTE)ES_buf[i+1]==0x00 && (BYTE)ES_buf[i+2]==0x01 && (BYTE)ES_buf[i+3]==0xb2)
		{
			if (ES_buf[i+4]==0x44 && ES_buf[i+5]==0x54 && ES_buf[i+6]==0x47 && ES_buf[i+7]==0x31 )
			{
				i+=afd_mpeg(ES_buf,i+8,0,0,len,NULL);
			}
		}
	}
}

int CParser::mpeg_video_sequence_header(char *buf, int pos, int prog_index, int stream_index, int ts_end, mpeg2_ext_info_t* pesinfo)
{
    UINT x_size = ((buf[pos+4]&0xff) << 4) | ((buf[pos+5]&0xf0) >>4);
	UINT y_size = ((buf[pos+5]&0xf) << 8) | (BYTE)(buf[pos+6]);
	BYTE aspect_ratio = (buf[pos+7]&0xf0)>>4;
	BYTE frame_rate = (buf[pos+7]&0xf);
	UINT bitrate = ((buf[pos+8]&0xff)<< 10) | ((buf[pos+9]&0xff)<< 2) | ((buf[pos+10]&0xc0)>>6) ;
	DWORD ar_x = 1;
	DWORD ar_y = 1;
	LONGLONG timeperfieldN=0;
	LONGLONG timeperfieldD=1;

	int start = pos;

	if (aspect_ratio<=0x4)
	{
		ar_x =Mpeg_AspectX[aspect_ratio];
		ar_y = Mpeg_AspectY[aspect_ratio];
	}

	if (frame_rate <=0x8)
	{
		timeperfieldN=MpegTimePerFieldN[frame_rate];
		timeperfieldD=MpegTimePerFieldD[frame_rate];
	}
	
	pos+=11;
	BYTE load_intra_quant_matrix = (buf[pos]&0x2)>>1;
	if (load_intra_quant_matrix)
		pos+=1;
	BYTE load_non_intra_quant_mat = buf[pos]&0x1;
	if (load_non_intra_quant_mat)
		pos+=1;

	pos+=1;

	bool ext_found=true;	// extension header
	while ( !((BYTE)buf[pos]==0x00 && (BYTE)buf[pos+1]==0x00 && (BYTE)buf[pos+2]==0x01 && (BYTE)buf[pos+3]==0xb5) )
	{
		pos++;
		if (pos >= ts_end || pos > start+140)
		{
			ext_found=false;
			break;
		}
	}
	
	u_short hor_ext=0;
	u_short ver_ext=0;
    u_short bitrate_ext=0;
    u_short framerate_ext_n=0;
    u_short framerate_ext_d=1;
	BYTE profile=0x0;
	BYTE level=0x0;
	BYTE progressive_sequence=0;
	BYTE marker=0;

	if (ext_found)
	{
		BYTE ext_id = (buf[pos+4]&0xf0)>>4;
		if (ext_id==0x1)
		{
			// Sequence extension:
			hor_ext = (buf[pos+5]&0x1)<<1 | ((buf[pos+6]&0x80)>>7);
			ver_ext = (buf[pos+6]&0xc0)>>5;
			bitrate_ext=  (buf[pos+6]&0x1f)<<7 | ((buf[pos+7]&0xfe)>>1) ;
			marker = buf[pos+7]&0x1;
			progressive_sequence = (buf[pos+5]&0xf)>>3;
			framerate_ext_n = (buf[pos+9]&0x60)<<5;
			framerate_ext_d = (buf[pos+9]&0x1f);
			profile = buf[pos+4]&0x7;
			level = (buf[pos+5]&0xf0)>>4;

			if (framerate_ext_n !=0)
			{
				timeperfieldN = timeperfieldN * (framerate_ext_d+1);
				timeperfieldD = timeperfieldD *  (framerate_ext_n+1);
			}

			if (marker)
			{
				x_size = (hor_ext<<12)+x_size;
				y_size = (hor_ext<<12)+y_size;
				bitrate = ((bitrate_ext <<18)+bitrate)*4/10; //KB/s
			}
		}
	}

	pos+=10;
	if (pesinfo!=NULL)
	{	
		pesinfo->video_arx=ar_x;
		pesinfo->video_ary=ar_y;
		pesinfo->video_width=x_size;
		pesinfo->video_height =y_size;
		m_dmt_seqhdr.progressive_sequence = progressive_sequence;
		m_dmt_seqhdr_received=true;
		
		for ( ;pos<ts_end; pos++ )
		{
			if ( (BYTE)buf[pos]==0x00 && (BYTE)buf[pos+1]==0x00 && (BYTE)buf[pos+2]==0x01 && (BYTE)buf[pos+3]==0xb8 )
			{			
				pesinfo->is_key_frame=true; //Gop found, gop must start with i-frame.
				break;
			}
		}
		return pos+3-start;
	}

	if (!m_programes[prog_index].second[stream_index].pes_parsed)
	{
		m_programes[prog_index].second[stream_index].has_data=true;
		m_programes[prog_index].second[stream_index].pes_vdet.aspect_x=ar_x;
		m_programes[prog_index].second[stream_index].pes_vdet.aspect_y=ar_y;
		m_programes[prog_index].second[stream_index].pes_vdet.height=y_size;
		m_programes[prog_index].second[stream_index].pes_vdet.width=x_size;
		m_programes[prog_index].second[stream_index].pes_vdet.timeperfieldNom=timeperfieldN;
		m_programes[prog_index].second[stream_index].pes_vdet.timeperfieldDenom=timeperfieldD;
		m_programes[prog_index].second[stream_index].pes_vdet.bitrate=bitrate;
		m_programes[prog_index].second[stream_index].pes_vdet.profile=profile;
		m_programes[prog_index].second[stream_index].pes_vdet.level=level;
		m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=0;

		BYTE map = m_pes[prog_index][stream_index].seq_hdr_map_pos;
		m_pes[prog_index][stream_index].seq_hdr = &(m_seqhdr[map]);
		m_pes[prog_index][stream_index].seq_hdr->progressive_sequence=progressive_sequence;
		m_pes[prog_index][stream_index].seq_hdr_map_pos+=1;

		int pict_hdr_len=0;
		// Check if picture header was received by now.
		if (m_pes[prog_index][stream_index].mpeg2_video_picture_header_received)
			mpeg_video_picture_header(m_pes[prog_index][stream_index].mpeg2_video_picture_header,0,prog_index,stream_index,MAX_MPEG_PICT_HEADER_SIZE,NULL,&pict_hdr_len);

		m_programes[prog_index].second[stream_index].pes_parsed=true;
	}

	return pos+3-start;
}

HRESULT CParser::mpeg_video_picture_header(char *buf, int pos, int prog_index, int stream_index, int ts_end, mpeg2_ext_info_t* pesinfo, int*len)
{
	HRESULT hr = E_FAIL;
	BYTE frame_type = ((buf[pos+5]&0x38) >> 3) ;

	int start = pos;

	//  Search for Picture_Coding_Extension 
	pos+=7;
	bool ext=false;
	
	int f_result = find_pict_coding_ext(&(buf[pos]),min(12,ts_end-pos));
	if (f_result != -1)
	{
		ext=true;
		pos+=f_result;
	}

	*len = pos-start;
	if (ext)
	{
		*len = pos+8 -start;
		hr = S_OK;
		BYTE ext_id = (buf[pos+4]&0xf0)>>4;
		if (ext_id==0x8)
		{
			BYTE picture_structure = (buf[pos+6] & 0x03);
			BYTE top_first = (buf[pos+7]&0x80)>>7;
			BYTE repeat_first_field = (buf[pos+7]& 0x3)>>1; 
			BYTE progressive=(buf[pos+8]&0x80)>>7;
			int frame_picture = 0;
			int fields = 0;
			bool prog_seq=false;
			if (pesinfo != NULL)
			{
				// If seq header isn't received by this time, we try to get info from initial parser.
				if (!m_dmt_seqhdr_received )
				{
					if (m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].seq_hdr!=NULL)
					{
						m_dmt_seqhdr.progressive_sequence = m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].seq_hdr->progressive_sequence;
						m_dmt_seqhdr_received =true;
					}
				}
				if (m_dmt_seqhdr.progressive_sequence==0x1)
					prog_seq =true;
			}
			else
			{
				if (m_pes[prog_index][stream_index].seq_hdr!=NULL)
				{
					if (m_pes[prog_index][stream_index].seq_hdr->progressive_sequence ==0x1) 
					prog_seq = true;
				}
			}

			if ( picture_structure == 3 ) frame_picture = 1;

			if ( frame_picture == 0 )
			{
				fields = 1;
			}
			else if (prog_seq ) 
			{
				if ( repeat_first_field == 0 ) fields = 2;
				if ( (repeat_first_field == 1) && (top_first == 0) ) fields = 4;
				if ( (repeat_first_field == 1) && (top_first == 1) ) fields = 6;
			}
			else
			{   // interlaced sequence
				if ( progressive == 0 ) fields = 2;
				if ( (progressive == 1) && (repeat_first_field == 0) ) fields = 2;
				if ( (progressive == 1) && (repeat_first_field == 1) ) fields = 3;
			}

			if (pesinfo!=NULL)
			{
				if (frame_type==0x01)
					pesinfo->is_key_frame=true;
				
				m_dmt_pict_cod.fields = fields;
				m_dmt_pictcod_received=true;
			}
			
			else // initial parser
			{
				if (m_pes[prog_index][stream_index].mpeg_pict_ext_checked)
				{
					return hr;
				}

				// Save number of fields
				BYTE map = m_pes[prog_index][stream_index].pict_cod_map_pos;
				m_pes[prog_index][stream_index].pict_cod = &(m_pictcod[map]); 
				m_pes[prog_index][stream_index].pict_cod->fields=fields;
				m_pes[prog_index][stream_index].pict_cod_map_pos+=1;
				
				if (progressive)
					m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=1;
				else
				{
					if(top_first)
						m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=2;
					else
						m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=3;
				}
		
				m_pes[prog_index][stream_index].mpeg_pict_ext_checked=true;
			}
		}
	}
	return hr;
}

int CParser::find_pict_coding_ext(char* buf, int len)
{
	int result = -1;

	for (int i=0  ;i<len;i++ )
	{
		if( (BYTE)buf[i]==0x00 && (BYTE)buf[i+1]==0x00 && (BYTE)buf[i+2]==0x01 && (BYTE)buf[i+3]==0xb5 )
		{
			result =i;
			break;
		}
	}
	return result;
}

int CParser::afd_mpeg(char *buf, int pos, int prog_index, int stream_index, int ts_end, mpeg2_ext_info_t* pesinfo)
{

	BYTE afd_format = getAfd(&buf[pos]);
	if (pesinfo == NULL)
		m_programes[prog_index].second[stream_index].pes_vdet.afd=afd_format;
	else
		pesinfo->afd=afd_format;

	return 10;
}


//ES: H264 video ============================================================================

// Parse beginning or full ES of h.264
// Input for this function is buffer of NAL units.
// This overloaded function is called only by initial parser !
HRESULT CParser::h264_video(char * buf, int pos, int prog_index, int stream_index, int ts_end)
{
	// PES starts with nal unit and can contain several nal units
	// Start code MAY be included. It can be 3 or 4 bytes long (4 for first nal of each frame)
	
	HRESULT hr =E_FAIL; // Sequence parameter set has the most important data: return true if found.

	BYTE forbidden_zero_bit;
    BYTE nal_ref_idc;
    BYTE unit_type;

	// Find first nal
	int code_len=0;
	int nal_pos = find_nal_start(buf,pos,ts_end,&code_len);

	if (nal_pos!=-1)
	{
		int nal_len;
		int next_nal_start;
		for (int i= nal_pos; i<ts_end; )
		{
			// Find nal length
			next_nal_start=find_nal_start(buf,i,ts_end,&code_len);
			if(next_nal_start!=-1)
				nal_len=next_nal_start-code_len-nal_pos;
			else
			{
				if (ts_end-pos>184)
					nal_len = ts_end-nal_pos;
				else
					 nal_len = 0; // This function is also used  for single ts packet with h.264 pes start, where last NAL may not be complete. 
			}

			// Remove emulation_prevention_three_byte for NAL unit
			int strip_pos=remove_emulation_prevention_bytes(buf,nal_pos,nal_len,m_nal_unit);
		
			if (strip_pos>0 && nal_len>0) 
			{
				forbidden_zero_bit = (m_nal_unit[0]&0x1)>>7;
				nal_ref_idc = (m_nal_unit[0]&0x60)>>5;
				unit_type = m_nal_unit[0]&0x1f;
				
				if (unit_type == 0x7 ) 
				{
					if ( !m_pes[prog_index][stream_index].nal_seq_param_set_parsed)// Only once in initial parsing, because buffer can contain whole PES.
						CParser::avc_seq_param_set(m_nal_unit,1,strip_pos,prog_index,stream_index,NULL);
					hr =S_OK;
				}
				else if (unit_type == 0x6 )
				{
					CParser::avc_sei_msg(m_nal_unit,1,strip_pos,prog_index,stream_index,NULL);
				}
			}
			
			if(next_nal_start != -1)
			{
				i=next_nal_start+3;
				nal_pos=next_nal_start;
			}
			else
				break;
		}
	}
	return hr;
}

// This overloaded function is called only by demultiplexer.
void CParser::h264_video(char* buf, int len, h264_ext_info_t* pesext)
{
	BYTE forbidden_zero_bit;
    BYTE nal_ref_idc;
    BYTE unit_type;
	bool delimiter_found=false;

	h264_ext_info_t pes_info;							// Dummy struct, so existing functions can be used.
	ZeroMemory(&pes_info,sizeof(h264_ext_info_t));

	int code_len=0;
	int nal_pos = find_nal_start(buf,0,len,&code_len);	//Find first nal

	if (nal_pos!=-1)
	{
		int nal_len;
		int next_nal_start;
		for (int i= nal_pos; i<len; i=i+3)
		{
			next_nal_start=find_nal_start(buf,i,len,&code_len); //Find nal length
			if(next_nal_start!=-1)
				nal_len=(next_nal_start-code_len)-nal_pos;
			else
				nal_len = len-nal_pos;

			int strip_pos=remove_emulation_prevention_bytes(buf,nal_pos,nal_len,m_nal_unit); // Remove emulation_prevention_three_byte

			if (strip_pos>0 && nal_len>0) 
			{
				forbidden_zero_bit = (m_nal_unit[0]&0x1)>>7;
				nal_ref_idc = (m_nal_unit[0]&0x60)>>5;
				unit_type = m_nal_unit[0]&0x1f;
				
				if (unit_type == 0x9) // Access unit delimiter - should be allways first
				{
					delimiter_found=true;
					pesext->h264_access_unit_pos[pesext->h264_access_unit_count]=nal_pos;
					pesext->h264_access_unit_count+=1;
				}
				
				else if (unit_type == 0x7) // SPS
				{
					CParser::avc_seq_param_set(m_nal_unit,1,strip_pos,-1,-1,&pes_info);
	
					pesext->video_arx=pes_info.video_arx;
					pesext->video_ary = pes_info.video_ary;
					pesext->video_height = pes_info.video_height;
					pesext->video_width = pes_info.video_width;
				}

				else if (unit_type == 0x6 ) // SEI
				{
					CParser::avc_sei_msg(m_nal_unit,1,strip_pos,0,0,&pes_info);
					pesext->afd=pes_info.afd;
				}

				else if (unit_type == 5 ) // IDR picture
				{
					if (avc_slice_header(m_nal_unit,1,strip_pos,pesext) && !delimiter_found)
					{
						pesext->h264_access_unit_pos[pesext->h264_access_unit_count]=nal_pos;
						pesext->h264_access_unit_count+=1;
					}
					if (!pesext->ext_video_key_frame_found)
					{
						pesext->ext_video_key_frame_pos=pesext->h264_access_unit_count;
						pesext->ext_video_key_frame_found=true;
					}
				}

				else if (unit_type == 1 || unit_type==2) // non-idr, part a slices
				{
					if (avc_slice_header(m_nal_unit,1,strip_pos,pesext) && !delimiter_found) // first vcl nal
					{
						pesext->h264_access_unit_pos[pesext->h264_access_unit_count]=nal_pos;
						pesext->h264_access_unit_count+=1;
					}
				}
			}

			i=i+nal_len+3;

			if(next_nal_start != -1)
			{
				i=next_nal_start+3;
				nal_pos=next_nal_start;
			}
			else
				break;
		}
	}
}

int CParser::find_nal_start(char*buf,int pos, int end, int* code_len)
{
	int result=-1;
	for(int i=pos; i<end-4;i++)
	{
		// Search for start code // NAL LENGTH
		if ((BYTE) buf[i]==0x00 && (BYTE)buf[i+1]==0x00 )
		{
			if ((BYTE)buf[i+2]==0x01)
			{
				result=i+3;
				*code_len=3;
				break;
			}
			else if ((BYTE)buf[i+2]==0x00 && (BYTE)buf[i+3]==0x01 )
			{
				result=i+4;
				*code_len=4;
				break;
			}
			else
				continue;
		}
	}
	return result;
}

int CParser::remove_emulation_prevention_bytes(char *input,int pos,int len, char *output)
{
	int strip_pos=0;
	int end=pos+len;
	for (;pos<end; pos++)
	{
		if(pos+2<len && ( input[pos]==0x0 && input[pos+1]==0x0 && input[pos+2]==0x3))
		{
			output[strip_pos]=input[pos];
			output[strip_pos+1]=input[pos+1];
			pos+=2;
			strip_pos+=2;
		}
		else
		{
			output[strip_pos]=input[pos];
			strip_pos+=1;
		}
	}
	return strip_pos;
}

int CParser::avc_seq_param_set(char* buf, int pos, int len,int prog_index, int stream_index,h264_ext_info_t* pesinfo)
{
	UINT pic_width;
	UINT pic_height;

	bool _video_nal_hrd_parameters_present_flag=false; 
	bool _video_vcl_hrd_parameters_present_flag=false; 
	bool _video_pic_struct_present_flag=false; 
	bool _separate_colour_plane_flag=false;
	UINT _max_frame_num=0;

	BYTE profile_idc = buf[pos];
	BYTE level_idc = buf[pos+2];
	
	// Variable length start:
	BYTE startbit=0;
	int start_pos=pos;
	pos+=3;
	UINT seq_param_seq_id = expcolomb_ui(buf,&pos,&startbit);

	UINT ChromaArrayType=1;

	if( profile_idc  ==  100  ||  profile_idc  ==  110  ||  profile_idc  ==  122  ||
		profile_idc  ==  244  ||  profile_idc  ==  44  ||  profile_idc  ==  83 
		|| profile_idc  == 86 )
	{
		UINT chroma_format_idc = expcolomb_ui(buf,&pos,&startbit);
		if (chroma_format_idc==3)
		{
			BYTE seperate_colour_plane_flag = read_bit(buf,&pos,&startbit);
			if (!seperate_colour_plane_flag)
			{
				ChromaArrayType=chroma_format_idc;
				_separate_colour_plane_flag=true;
			}
			else
			{
				ChromaArrayType=1;
				_separate_colour_plane_flag=true;
			}
		}
	
		UINT bit_depth_luma_minus8 = expcolomb_ui(buf,&pos,&startbit);
		UINT bit_depth_chroma_minus8 = expcolomb_ui(buf,&pos,&startbit);
		
		BYTE qpprime_y_zero_transform_bypass_flag= read_bit(buf,&pos,&startbit);
		BYTE seq_scaling_matrix_present_flag = read_bit(buf,&pos,&startbit);
		
		BYTE seq_scaling_list_present_flag;
		if (seq_scaling_matrix_present_flag)
		{
			for (int i=0;  i < ((chroma_format_idc != 3 )? 8:12 ); i++)
			{
				seq_scaling_list_present_flag =  read_bit(buf,&pos,&startbit);
			}
		}
	}

	UINT log2_max_frame_num_minus4 = expcolomb_ui(buf,&pos,&startbit);
	_max_frame_num=log2_max_frame_num_minus4 + 4;
	UINT pic_order_cnt_type =expcolomb_ui(buf,&pos,&startbit);

	if( pic_order_cnt_type == 0 ) 
	{
		UINT log2_max_pic_order_cnt_lsb_minus4 =expcolomb_ui(buf,&pos,&startbit);
	}
	else if ( pic_order_cnt_type == 1 )
	{
		move_for_bits(&pos,&startbit,1);
		int offset_for_non_ref_pic = expcolomb_se(buf,&pos,&startbit);
		int  offset_for_top_to_bottom_field = expcolomb_se(buf,&pos,&startbit);
		UINT num_ref_frames_in_pic_order_cnt_cycle =expcolomb_ui(buf,&pos,&startbit);
		for(UINT i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
		{
			int offset_for_ref_frame_i= expcolomb_se(buf,&pos,&startbit);
		}
	}

	UINT num_ref_frames = expcolomb_ui(buf,&pos,&startbit);

	move_for_bits(&pos,&startbit,1);
	
	UINT pic_width_in_mbs_minus1 = expcolomb_ui(buf,&pos,&startbit);
	UINT pic_height_in_map_units_minus1 = expcolomb_ui(buf,&pos,&startbit);
	BYTE frame_mbs_only_flag = read_bit(buf,&pos,&startbit);

	if (!frame_mbs_only_flag)
		move_for_bits(&pos,&startbit,1);
	move_for_bits(&pos,&startbit,1);
	BYTE frame_cropping_flag = read_bit(buf,&pos,&startbit);
	
	pic_width=(pic_width_in_mbs_minus1+1)*16;
	pic_height =(pic_height_in_map_units_minus1+1)*16*(2-frame_mbs_only_flag);

	if ( frame_cropping_flag)
	{
		int CropUnitX;
		int CropUnitY;
		if (ChromaArrayType==0)
		{
			CropUnitX=1;
			CropUnitY=2-frame_mbs_only_flag;
		}
		else
		{
		    CropUnitX=Avc_SubWidthC [ChromaArrayType];
            CropUnitY=Avc_SubHeightC[ChromaArrayType]*(2-frame_mbs_only_flag);
		}
		
		UINT frame_crop_left_offset = expcolomb_ui(buf,&pos,&startbit);
		UINT frame_crop_right_offset = expcolomb_ui(buf,&pos,&startbit);
		UINT frame_crop_top_offset = expcolomb_ui(buf,&pos,&startbit);
		UINT  frame_crop_bottom_offset = expcolomb_ui(buf,&pos,&startbit);

		pic_width -=(frame_crop_left_offset+frame_crop_right_offset )*CropUnitX;
		pic_height -=(frame_crop_top_offset +frame_crop_bottom_offset)*CropUnitY;

	}
	BYTE vui_parameters_present_flag = read_bit(buf,&pos,&startbit);

	UINT arx=1;
	UINT ary=1;
	if (vui_parameters_present_flag)
	{
		BYTE aspect_ratio_info_present_flag = read_bit(buf,&pos,&startbit);

		if (aspect_ratio_info_present_flag)
		{
			BYTE aspect_ratio_idc = read_bits(buf,&pos,&startbit,8);
			if (aspect_ratio_idc >0 && aspect_ratio_idc<=254)
			{
				arx=Avc_Aspect_X[aspect_ratio_idc];
				ary=Avc_Aspect_Y[aspect_ratio_idc];
			}
			if (aspect_ratio_idc==255)
			{
				arx=read_bits(buf,&pos,&startbit,16);
				ary = read_bits(buf,&pos,&startbit,16);
			}
	    }

		BYTE overscan_info_present_flag = read_bit(buf,&pos,&startbit);
		if (overscan_info_present_flag )
			move_for_bits(&pos,&startbit,1);
		BYTE video_signal_type_present_flag = read_bit(buf,&pos,&startbit);
		if (video_signal_type_present_flag)
		{
			BYTE video_format = read_bits(buf,&pos,&startbit,3);
			BYTE video_full_range_flag = read_bit(buf,&pos,&startbit);
			BYTE colour_description_present_flag = read_bit(buf,&pos,&startbit);
			if (colour_description_present_flag)
			{
				BYTE colour_primaries = read_bits(buf,&pos,&startbit,8);
				BYTE transfer_characteristics = read_bits(buf,&pos,&startbit,8);
				BYTE matrix_coefficients  =read_bits(buf,&pos,&startbit,8);
			}
		}
		BYTE chroma_loc_info_present_flag = read_bit(buf,&pos,&startbit);
		if (chroma_loc_info_present_flag)
		{
			DWORD chroma_sample_loc_type_top_field =expcolomb_ui(buf,&pos,&startbit);
			DWORD chroma_sample_loc_type_bottom_field = expcolomb_ui(buf,&pos,&startbit);
		}
		BYTE timing_info_present_flag = read_bit(buf,&pos,&startbit);
		if (timing_info_present_flag)
		{
			UINT num_units_in_tick = read_bits(buf,&pos,&startbit,32);
			UINT time_scale = read_bits(buf,&pos,&startbit,32);
			BYTE fixed_frame_rate_flag  = read_bit(buf,&pos,&startbit);
		}
		BYTE nal_hrd_parameters_present_flag  = read_bit(buf,&pos,&startbit);
		
		_video_nal_hrd_parameters_present_flag=false;
		_video_vcl_hrd_parameters_present_flag=false;

		if (nal_hrd_parameters_present_flag)
		{
			_video_nal_hrd_parameters_present_flag=true;
			
			UINT cpb_cnt_minus1 = expcolomb_ui(buf,&pos,&startbit);
			BYTE bit_rate_scale = read_bits(buf,&pos,&startbit,4);
			BYTE cpb_size_scale = read_bits(buf,&pos,&startbit,4);

			for( UINT SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++ ) 
			{
				UINT vala= expcolomb_ui(buf,&pos,&startbit);
				UINT valb= expcolomb_ui(buf,&pos,&startbit);
				BYTE valc = read_bit(buf,&pos,&startbit);
			}
			move_for_bits(&pos,&startbit,20);
		}

		BYTE vcl_hrd_parameters_present_flag  = read_bit(buf,&pos,&startbit); 

		if (vcl_hrd_parameters_present_flag)
		{
			_video_vcl_hrd_parameters_present_flag=true;

			UINT cpb_cnt_minus1 = expcolomb_ui(buf,&pos,&startbit);

			BYTE bit_rate_scale = read_bits(buf,&pos,&startbit,4);

			BYTE cpb_size_scale = read_bits(buf,&pos,&startbit,4);

			for( UINT SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++ )
			{
				UINT vala= expcolomb_ui(buf,&pos,&startbit);
				UINT valb= expcolomb_ui(buf,&pos,&startbit);
				BYTE valc = read_bit(buf,&pos,&startbit);
			}
			move_for_bits(&pos,&startbit,20);
		}
		if( nal_hrd_parameters_present_flag  || vcl_hrd_parameters_present_flag ) 
			move_for_bits(&pos,&startbit,1);

		BYTE pic_struct_present_flag = read_bit(buf,&pos,&startbit);
		if (pic_struct_present_flag)
			_video_pic_struct_present_flag=true;
	
		BYTE bitstream_restriction_flag = read_bit(buf,&pos,&startbit);

		if (bitstream_restriction_flag)
		{
			BYTE motion_vectors_over_pic_boundaries_flag= read_bit(buf,&pos,&startbit);

			UINT max_bytes_per_pic_denom =expcolomb_ui(buf,&pos,&startbit);

			UINT max_bits_per_mb_denom =	expcolomb_ui(buf,&pos,&startbit);

			UINT log2_max_mv_length_horizonta=expcolomb_ui(buf,&pos,&startbit);

			UINT log2_max_mv_length_vertical =expcolomb_ui(buf,&pos,&startbit);

			UINT num_reorder_frames=expcolomb_ui(buf,&pos,&startbit);

			UINT  max_dec_frame_buffering =expcolomb_ui(buf,&pos,&startbit);
		}
	}

	// DEMULTIPLEXER:

	if (pesinfo!=NULL)
	{
		pesinfo->video_arx=arx;
		pesinfo->video_ary=ary;
		pesinfo->video_height=pic_height;
		pesinfo->video_width =pic_width;

		// Save parameters of LAST received SPS 
		m_dmt_sps.video_nal_hrd_parameters_present_flag = _video_nal_hrd_parameters_present_flag;
		m_dmt_sps.video_vcl_hrd_parameters_present_flag = _video_vcl_hrd_parameters_present_flag;
		m_dmt_sps.video_pic_struct_present_flag = _video_pic_struct_present_flag;
		m_dmt_sps.separate_colour_plane_flag = _separate_colour_plane_flag;
		m_dmt_sps.max_frame_num = _max_frame_num;

		if (!m_dmt_sps_received)
			m_dmt_sps_received=true;

		return pos-start_pos;
	}

	// INITIAL PARSER::

	if (!m_pes[prog_index][stream_index].nal_seq_param_set_parsed)
	{
		BYTE map = m_pes[prog_index][stream_index].sps_last_map_pos;
		//map SPS for this stream:_
		m_pes[prog_index][stream_index].sps = &(m_sps[map]);
		m_pes[prog_index][stream_index].sps->video_nal_hrd_parameters_present_flag = _video_nal_hrd_parameters_present_flag;
		m_pes[prog_index][stream_index].sps->video_vcl_hrd_parameters_present_flag = _video_vcl_hrd_parameters_present_flag;
		m_pes[prog_index][stream_index].sps->video_pic_struct_present_flag = _video_pic_struct_present_flag;
		m_pes[prog_index][stream_index].sps->separate_colour_plane_flag = _separate_colour_plane_flag;
		m_pes[prog_index][stream_index].sps->max_frame_num = _max_frame_num;
		m_pes[prog_index][stream_index].sps_last_map_pos+=1;
		m_pes[prog_index][stream_index].nal_seq_param_set_parsed=true;
	}
	
	if (!m_programes[prog_index].second[stream_index].pes_parsed)
	{	
		m_programes[prog_index].second[stream_index].has_data=true;
		m_programes[prog_index].second[stream_index].pes_vdet.width=pic_width;
		m_programes[prog_index].second[stream_index].pes_vdet.height=pic_height;
		m_programes[prog_index].second[stream_index].pes_vdet.profile=profile_idc;
		m_programes[prog_index].second[stream_index].pes_vdet.level=level_idc;
		m_programes[prog_index].second[stream_index].pes_vdet.aspect_x=arx;
		m_programes[prog_index].second[stream_index].pes_vdet.aspect_y=ary;
		m_programes[prog_index].second[stream_index].pes_vdet.timeperfieldNom=0; // no informations
		m_programes[prog_index].second[stream_index].pes_vdet.timeperfieldDenom=1;
		m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=0;
		m_programes[prog_index].second[stream_index].pes_vdet.bitrate=0;

		// if picture_timing sei is received by now, we parse it, otherwise we don't wait. Interlacing therefore won't be allways parsed in initial parsing.
		if (m_pes[prog_index][stream_index].first_pict_timing_received)
		{
			avc_pict_timing_sei(m_pes[prog_index][stream_index].first_pict_timing_sei, 0, MAX_PICT_TIM_SEI_SIZE, prog_index, stream_index,pesinfo);
		}

		m_programes[prog_index].second[stream_index].pes_parsed=true;
	}

	return pos-start_pos;
}

int CParser::avc_sei_msg(char* buf, int pos, int len, int prog_index, int stream_index,h264_ext_info_t* pesinfo)
{
	UINT payloadType=0;
	UINT payloadSize=0;
	BYTE startbit=0;
    BYTE ffbyte;
	while((BYTE)buf[pos]==0xff)
	{
		ffbyte = buf[pos];
		pos+=1;
		payloadType+=255;
	}
	BYTE last_payload_type_byte=buf[pos];
	pos+=1;
	payloadType+=last_payload_type_byte;
	while ((BYTE)buf[pos]==0xff)
	{
		ffbyte = buf[pos];
		pos+=1;
		payloadSize+=255;
	}
	BYTE last_payload_size_byte = buf[pos];
	pos+=1;
	payloadSize+=last_payload_type_byte;
	
	int j=pos;
	if (payloadType == 1)
	{
		// INITIAL PARSER::
		if (pesinfo == NULL) 
		{
			if (!m_pes[prog_index][stream_index].first_pict_timing_received)
			{
				// Copy full SEI message 
				BYTE mappos = m_pes[prog_index][stream_index].first_pict_timing_sei_last_map_pos;

				ZeroMemory(&(m_first_pict_timing_sei[mappos]),sizeof(m_first_pict_timing_sei));
				memcpy( &(m_first_pict_timing_sei[mappos]),&(buf[pos]),min(payloadSize,sizeof(m_first_pict_timing_sei)) );
				m_pes[prog_index][stream_index].first_pict_timing_sei = m_first_pict_timing_sei[mappos];
                m_pes[prog_index][stream_index].first_pict_timing_sei_last_map_pos+=1;
                m_pes[prog_index][stream_index].first_pict_timing_received=true;;
			}
			else
			{
				if(m_pes[prog_index][stream_index].nal_seq_param_set_parsed)
				{
					j= avc_pict_timing_sei(buf, pos, payloadSize,prog_index, stream_index,NULL);
				}
			}
		}
	}
	else if (payloadType == 11 )
	{
		// TODO: sequence layer characteristics SET:
	}

	else if (payloadType == 4)	// user data itu_t_35
	{
		BYTE itu_t_t35_country_code = (BYTE)buf[pos];
		if (itu_t_t35_country_code!= 0xff)
		{
			pos+=1;
		}
		else
		{
			BYTE itu_t_t35_country_code_extension_byte =  (BYTE)buf[pos+1];
			pos+=2;
		}
		
		u_short itu_t_t35_provider_code = (buf[pos]&0xff)<<8 | (BYTE)buf[pos+1];

		UINT user_identifier = (buf[pos+2]&0xff)<<24 | (buf[pos+3]&0xff)<<16 | (buf[pos+4]&0xff)<<8 | (BYTE)buf[pos+5];

		if (user_identifier == 0x44544731)
		{
			if (pesinfo==NULL)
				afd_h264(buf,pos+6,prog_index,stream_index,0,NULL);
			else
				afd_h264(buf,pos+6,0,0,0,pesinfo);
		}
	}

	//else if (payloadType == 5) // user data unregistered
	//{
	//
	//}

	return payloadSize+1;
}

int CParser::avc_pict_timing_sei(char* buf, int pos,int len, int prog_index, int stream_index,h264_ext_info_t* pesinfo)
{

	UINT cpb_removal_delay;
	UINT dpb_output_delay;
	BYTE pic_struct = 0x0;

	BYTE startbit=0;
	int start=pos;

	SPS_flags_and_values_t* pSPS;
	if (pesinfo!=NULL)
		pSPS = &m_dmt_sps;							// demux thread
	else
		pSPS=m_pes[prog_index][stream_index].sps;	// initial parser

	if (pSPS==NULL)
		return pos-start;

	bool CpbDpbDelaysPresentFlag=false;
	if (pSPS->video_nal_hrd_parameters_present_flag || pSPS->video_vcl_hrd_parameters_present_flag)
		CpbDpbDelaysPresentFlag=true;

	if (CpbDpbDelaysPresentFlag)
	{
		cpb_removal_delay = expcolomb_ui(buf,&pos,&startbit);
		dpb_output_delay = expcolomb_ui(buf,&pos,&startbit);
	}
	if(pSPS->video_pic_struct_present_flag)
	{
		pic_struct = read_bits(buf,&pos,&startbit,4);
		//BYTE ct_type=0;
		//BYTE clock_timestamp_flag = read_bits(buf,&pos,&startbit,1);
		
		// INITIAL PARSER:
		if (pesinfo==NULL )
		{
			if (!m_pes[prog_index][stream_index].first_video_pic_struct_parsed)
			{
				//if (clock_timestamp_flag)
				//	ct_type = read_bits(buf,&pos,&startbit,2);
				//if (ct_type!=0)
				//....
				m_programes[prog_index].second[stream_index].pes_vdet.interlaced_indicator=(u_short)pic_struct+1;
				m_programes[prog_index].second[stream_index].pes_parsed=true;
				m_pes[prog_index][stream_index].first_video_pic_struct_parsed=true;
			}
		}
	}

	return pos-start;
}

// This function is NOT called by initial parser
// SPS must be parsed before this function is called.
// If SPS is not available yet, we must get appropriate info from SPS parsed by initial parser.
bool CParser::avc_slice_header(char* buf, int pos,int len, h264_ext_info_t* pesext)
{
	bool first_vcl_nalu=false;
	BYTE startbit=0;
	int start=pos;

	if (!m_dmt_sps_received)
	{
		// Try to get SPS info from initial parser
		if ( m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps != NULL)
		{
			m_dmt_sps.max_frame_num = m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps->max_frame_num;
			m_dmt_sps.separate_colour_plane_flag = m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps->separate_colour_plane_flag;
			m_dmt_sps.video_nal_hrd_parameters_present_flag =  m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps->video_nal_hrd_parameters_present_flag;
			m_dmt_sps.video_pic_struct_present_flag = m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps->video_pic_struct_present_flag;
			m_dmt_sps.video_vcl_hrd_parameters_present_flag =  m_pes[m_cur_prog.vid_prog_index][m_cur_prog.vid_stream_index].sps->video_vcl_hrd_parameters_present_flag;
			m_dmt_sps_received=true;
		}
	}

	UINT first_mb_in_slice = expcolomb_ui(buf,&pos,&startbit);
	UINT slice_type = expcolomb_ui(buf,&pos,&startbit);
	UINT pic_parameter_set_id = expcolomb_ui(buf,&pos,&startbit);
	
	if (m_dmt_sps.separate_colour_plane_flag)
	{
		BYTE colour_plane_id = read_bits(buf,&pos,&startbit,2);
	}

	UINT frame_num = read_bits(buf,&pos,&startbit,m_dmt_sps.max_frame_num);

	// Find acess unit by finding first vcl
	if (m_dmt_slice.last_pic_parameter_set_id != pic_parameter_set_id || m_dmt_slice.last_frame_num != frame_num)
	{
		first_vcl_nalu=true;
	}

	// Check for i slice
	if (slice_type == 0x2 || slice_type == 0x4 || slice_type == 0x7 || slice_type == 0x9)
	{

		pesext->ext_video_key_frame_found=true;
		if(pesext->h264_access_unit_count>0)
			pesext->ext_video_key_frame_pos=pesext->h264_access_unit_pos[pesext->h264_access_unit_count];
		else
			pesext->ext_video_key_frame_pos=0;
	}

	// Store last id, so we can detect next frame
	m_dmt_slice.last_pic_parameter_set_id = (int)pic_parameter_set_id;
	m_dmt_slice.last_frame_num = (int)frame_num;

	return first_vcl_nalu;
}

int CParser::afd_h264(char *buf, int pos, int prog_index, int stream_index, int ts_end,h264_ext_info_t* pesinfo )
{
	BYTE afd_format = getAfd(&buf[pos]);
	if (pesinfo == NULL)
		m_programes[prog_index].second[stream_index].pes_vdet.afd=afd_format;
	else
		pesinfo->afd=afd_format;

	return 10;
}

//ES end =====================================================================================

// Descriptors ===============================================================================

BYTE CParser::getAfd(char* buf)
{
	BYTE active_format=0;
	BYTE active_format_flag = (buf[0]&0x40)>6;
	if (active_format_flag==0x1)
	{
		active_format = (BYTE)(buf[1]&0xf);
	}

	return active_format;
}

void CParser::teletext_descriptor(char* buf, int pos, int len, vector<teletext_ebu_t>* ttx_v)
{
	char lang[4]= {0x0,0x0,0x0,0x0};
	teletext_ebu_t ttx_s;
	for (int i=pos; i<pos+len; i=i+5 )
	{
		lang[0]=(BYTE)buf[i];
		lang[1]=(BYTE)buf[i+1];
		lang[2]=(BYTE)buf[i+2];
		lang[3]=(BYTE)0x0; 
		memcpy(ttx_s.language,lang,4);
		ttx_s.type = (buf[i+3]&0xf8)>>3;
		ttx_s.magazine_number =buf[i+3]&0x7; 
		ttx_s.page_number= buf[i+4];
		ttx_v->	push_back(ttx_s);
	}
}

void CParser::subtitling_descriptor(char* buf, int pos, int len, vector<subtitling_t>* sub_v)
{
	subtitling_t sub_s;
	for (int i=pos; i<pos+len; i=i+8)
	{
		sub_s.language[0]= (BYTE)buf[i];
		sub_s.language[1]= (BYTE)buf[i+1];
		sub_s.language[2]= (BYTE)buf[i+2];
		sub_s.language[3]= (BYTE)0x0;
		sub_s.type = (BYTE) buf[i+3];
		sub_s.composition_page_id =((buf[i+4]&0xff)<<8) | (BYTE)buf[i+5];
		sub_s.ancillary_page_id = ((buf[i+6]&0xff) <<8) | (BYTE)buf[i+7];
		sub_v->push_back(sub_s);
	}
}

void CParser::iso_693_language_decsriptor(char*buf, int pos, int len, char* code)
{
	code[0] = buf[pos];
	code[1] = buf[pos+1];
	code[2] = buf[pos+2];
}

void CParser::services_descriptor(char* buf, int len, u_short prog)
{
	service_desc_t service;
	service.prog=prog;
	ZeroMemory(&service.service_provider,MAX_SERVICE_NAME_LEN);
	ZeroMemory(&service.service_name,MAX_SERVICE_NAME_LEN);

	BYTE service_type = buf[0];
	BYTE service_provider_name_length = buf[1];
	
	int pos = 2;
	for (int i=0; i<service_provider_name_length;i++)
	{
		if (i<MAX_SERVICE_NAME_LEN)
			memcpy(&(service.service_provider[i]),&(buf[pos]),1);
		pos+=1;

	}
	BYTE service_name_length=buf[pos];
	pos+=1;

	for (int j=0; j<service_name_length;j++)
	{
		if (j<MAX_SERVICE_NAME_LEN)
			memcpy(&(service.service_name[j]),&(buf[pos]),1);
		pos+=1;
	}

	// Add to services list
	m_global_services.push_back(service);
}


void CParser::short_event_descriptor(char* buf, int len, u_short prog, short_event_desc_t* desc, bool start_new)
{

	ZeroMemory(&desc->event_name,MAX_EVENT_NAME_LEN);
	ZeroMemory(&desc->event_text,MAX_EVENT_TEXT_LEN);

	BYTE event_name_length = buf[3];
	
	int pos = 2;
	for (int i=0; i<event_name_length;i++)
	{
		if (i<MAX_EVENT_NAME_LEN)
			memcpy(&(desc->event_name[i]),&(buf[pos]),1);
		pos+=1;

	}
	BYTE event_text_length=buf[pos];
	pos+=1;

	for (int j=0; j<event_text_length;j++)
	{
		if (j<MAX_SERVICE_NAME_LEN)
			memcpy(&(desc->event_text[j]),&(buf[pos]),1);
		pos+=1;
	}
}

void CParser::parse_network_name_desc(char* buf, int len, network_name_desc_t* dsc)
{
	ZeroMemory(dsc->network_name,MAX_DESC_NAME_LEN);
	int pos=2;
	for ( int i=0 ; i< len; i++)
	{
		dsc->network_name[i] = buf[pos+i];
	}
}

void CParser::parse_service_list_desc(char* buf, int len, vector<service_list_desc_t>* dsclist)
{
	for (int i=2; i<len+2; )
	{
		service_list_desc_t dsc;
		dsc.service_id = (buf[i] < 8) | (BYTE)buf[i+1];
		dsc.service_type = buf[i+2];
		dsclist->push_back(dsc);
		i+=3;
	}
}

void CParser::parse_cable_delivery_system_desc(char* buf, int len, cable_delivery_system_desc_t* dsc)
{
	char* ofs = buf+2;

	dsc->frequency = ((ofs[0] & 0xff)<<24) | ((ofs[1] & 0xff)<<16) | ((ofs[2] & 0xff)<<8) |  (BYTE) ofs[3];
	dsc->FEC_outer = ofs[5] &0xf; 
	dsc->modulation = ofs[6];
	dsc->symbol_rate = ((ofs[7] & 0xff)<<20) | ((ofs[8] & 0xff)<<12) | ((ofs[9] & 0xff)<<4) |  ((ofs[10]&0xf0)>>4);
	dsc->FEC_inner = ofs[10] & 0xf;
}


void CParser::parse_satellite_delivery_system_desc(char* buf, int len, satellite_delivery_system_desc_t* dsc)
{
	char* ofs = buf+2;
	
	dsc->frequency = ((ofs[0] & 0xff)<<24) | ((ofs[1] & 0xff)<<16) | ((ofs[2] & 0xff)<<8) |  (BYTE) ofs[3];
	dsc->orbital_position = ((ofs[4] & 0xff)<<8) |  (BYTE) ofs[5];
	dsc->west_east_flag = (ofs[6]&0x80)>>7;
	dsc->polarization = (ofs[6]&0x60)>>5;
	dsc->rolloff = (ofs[6]&0x18)>>3;
	dsc->modulation_system = (ofs[6]&0x4)>>2; 
	dsc->modulation_type = ofs[6]&0x3;
	dsc->symbol_rate =  ((ofs[7] & 0xff)<<20) | ((ofs[8] & 0xff)<<12) | ((ofs[9] & 0xff)<<4) | (BYTE) ((ofs[10]&0xf0)>>4);
	dsc->FEC_inner = ofs[10] & 0xf;
}

void CParser::parse_terrestial_delivery_system_desc(char* buf, int len, terrestial_delivery_system_desc_t* dsc)
{
	char* ofs = buf+2;
	dsc->centre_frequency =((ofs[0] & 0xff)<<24) | ((ofs[1] & 0xff)<<16) | ((ofs[2] & 0xff)<<8) |  (BYTE) ofs[3]; 

	int byte_pos = 4;
	BYTE bit_ofs=0;
	dsc->bandwidth = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,3);
	dsc->priority =  (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,1);
	dsc->time_scalling_indicator = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,1);
	dsc->MPE_FEC_indicator = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,1);
	read_bits(ofs,&byte_pos,&bit_ofs,2);
	dsc->constellation =(BYTE) read_bits(ofs,&byte_pos,&bit_ofs,2);
	dsc->hierarchy_information = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,3);
	dsc->code_rate_HP_stream = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,3);
	dsc->code_rate_LP_stream = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,3);
	dsc->guard_interval =(BYTE) read_bits(ofs,&byte_pos,&bit_ofs,2);
	dsc->transmission_mode = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,2);
	dsc->other_frequency_use = (BYTE) read_bits(ofs,&byte_pos,&bit_ofs,1);
}



// descriptors end ==========================================================================

u_short CParser::continuity_count(char* buffer, int ts_start)
{
	return (u_short)(buffer[ts_start+3] & 0xf);
}

void CParser::clear_audiodet(pesaudiodet_t* det)
{
	det->bitrate=0;
	det->channels=0;
	det->mpeg_layer=0;
	det->sampfreq=0;
	det->mpeg_version=0;
}

void CParser::clear_videodet(pesvideodet_t* det)
{
	det->aspect_x=0;
	det->aspect_y=0;
	det->level=0;
	det->profile=0;
	det->bitrate=0;
	det->height=0;
	det->timeperfieldNom=0;
	det->timeperfieldDenom=1;
	det->width=0;
	det->interlaced_indicator=0;
	det->afd=0;
}


void CParser::clear_parsed_data()
{
	//CAutoLock cAutoLock(&m_pLock);

	int prog_count = m_programes.size();
	for(int i=0; i<prog_count; i++)
	{
		int pid_count=m_programes[i].second.size();
		for (int j=0; j<pid_count; j++)
		{
			m_programes[i].second[j].subtitling_descriptors_v.clear();
			m_programes[i].second[j].teletext_descriptors_v.clear();
			m_programes[i].second.clear();
		}
	}

	CParser::clear_tables();
	CParser::m_all_data_parsed=false;
}

void CParser::clear_tables()
{
	ZeroMemory((psi_table_t*)&m_pat,sizeof(psi_table_t));
	ZeroMemory(&m_pmts,sizeof(pmt_table_t));
	ZeroMemory(&m_nit,sizeof(nit_table_t));
	ZeroMemory((psi_table_t*)&m_sdt,sizeof(psi_table_t));
	ZeroMemory((psi_table_t*)&m_dmt_eit,sizeof(psi_table_t));


	for (int i = 0; i<MAX_PROGS; i++ )
	{
		for (int j=0; j<MAX_PID_COUNT_PER_PROG; j++)
		ZeroMemory(&(m_pes[i][j]),sizeof(es_info_t));
	}
}


bool CParser::is_pes_type_supported(u_short type) //mix of type in private types
{
	switch (type)
	{
		case 0x02: //mpeg2 video
		case 0x01: //mpeg1 video
		case 0x1b: //h264 video
		case 0x03: // mpeg-1 audio
		case 0x04: // mpeg-2 audio
		case 0x11: // aac audio
		case 0x81: // ac3 audio
			//TODO: 0x83, 0x84, 0xA1; 0x80. 0x85, 0x86, 0xA2
			return true;
         
		default:
			return false;
	}
}

MAYOR_TYPE CParser::pes_mayor_type(u_short type)
{
	switch (type)
	{
		case 0x02: //mpeg2 video
		case 0x01: //mpeg1 video
		case 0x1b: //h264 video
		return MAYOR_TYPE_VIDEO;
		
		case 0x03: // mpeg-1 audio
		case 0x04: // mpeg-2 audio
		case 0x11: // aac audio
		case 0x81: // ac3 audio
			//TODO: 0x83, 0x84, 0xA1; 0x80. 0x85, 0x86, 0xA2
		return MAYOR_TYPE_AUDIO;
         
		default:
			return MAYOR_TYPE_UNKNOWN;
	}
}

USHORT CParser::setCurrentStream(int pid, BYTE type)
{
	int progs = m_programes.size();
	USHORT PAT=0;
	int prog_num = -1;
	bool found=false;
	for (int i=0; i<progs; i++)
	{
		int pidcount = m_programes[i].second.size();
		for (int j=0; j<pidcount; j++)
		{
			if (m_programes[i].second[j].pid==pid)
			{
				if (type == VIDEO_STREAM)
				{
					m_cur_prog.vid_prog_index=i;
					m_cur_prog.vid_stream_index=j;
					prog_num = m_programes[i].first;
					found=true;
					break;
				}
				else if (type == AUDIO_STREAM)
				{
					m_cur_prog.aud_prog_index=i;
					m_cur_prog.aud_stream_index=j;
					prog_num = m_programes[i].first;
					found=true;
					break;
				}
			}
		}
		if (found)
		{
			int patsize = m_pat.list.size();
			for (int i=0; i< patsize;i++)
			{
				if (m_pat.list[i].first == prog_num)
					return PAT;
			}
		}
	}
	return PAT;
}

HRESULT CParser::parse_rtp(char* buf, int len, rtp_header* hdr)
{
    HRESULT res=S_OK;

	BYTE x = (buf[0]&0x10)>>4;
	BYTE cc = buf[0]&0xf;

	hdr->header_length = 12 + (cc*4);
	if (x==1)
	{
		USHORT ext_length = (buf[hdr->header_length+2] & 0xff)<<8 | (BYTE)(buf[hdr->header_length+3]);
		hdr->header_length += 4+(ext_length*4); 
	}
	hdr->seq_number = ((buf[2] & 0xff)<<8) |  (BYTE) buf[3];
	hdr->timestamp = ((buf[4] & 0xff)<<24) | ((buf[5] & 0xff)<<16) | ((buf[6] & 0xff)<<8) |  (BYTE) buf[7];
	hdr->ssrc = ((buf[8] & 0xff)<<24) | ((buf[9] & 0xff)<<16) | ((buf[10] & 0xff)<<8) |  (BYTE) buf[11];

	return res;
}

// Interop helper functions:

void CParser::programesCount(int* count)
{
	*count= m_programes.size();
}

void CParser::pidCount(int prog_index, int* count)
{
	*count= m_programes[prog_index].second.size();
}

void CParser::parsed_data(int prog_index, int pid_index, interop_piddetails_t* iop_det )
{
	memcpy(iop_det->language,m_programes[prog_index].second[pid_index].language,4);
	iop_det->pes_supported=m_programes[prog_index].second[pid_index].pes_supported;
	iop_det->pid=m_programes[prog_index].second[pid_index].pid;
	iop_det->stream_type=m_programes[prog_index].second[pid_index].stream_type;
	iop_det->subtitling_descriptor_count=m_programes[prog_index].second[pid_index].subtitling_descriptors_v.size();
	iop_det->teletext_descriptor_count=m_programes[prog_index].second[pid_index].teletext_descriptors_v.size();
	iop_det->pcr_pid = m_programes[prog_index].second[pid_index].PCR_PID;
	iop_det->data_ok = m_programes[prog_index].second[pid_index].has_data;
}

void CParser::get_audio_det(int prog_index, int pid_index, pesaudiodet_t* audio_det)
{
	audio_det->bitrate = m_programes[prog_index].second[pid_index].pes_adet.bitrate;
	audio_det->channels = m_programes[prog_index].second[pid_index].pes_adet.channels;
	audio_det->mpeg_layer = m_programes[prog_index].second[pid_index].pes_adet.mpeg_layer;
	audio_det->mpeg_version = m_programes[prog_index].second[pid_index].pes_adet.mpeg_version;
	audio_det->sampfreq = m_programes[prog_index].second[pid_index].pes_adet.sampfreq;
}

void CParser::get_video_det(int prog_index, int pid_index, pesvideodet_t* video_det)
{
	video_det->aspect_x = m_programes[prog_index].second[pid_index].pes_vdet.aspect_x;
	video_det->aspect_y = m_programes[prog_index].second[pid_index].pes_vdet.aspect_y;
	video_det->bitrate = m_programes[prog_index].second[pid_index].pes_vdet.bitrate;
	video_det->height = m_programes[prog_index].second[pid_index].pes_vdet.height;
	video_det->interlaced_indicator = m_programes[prog_index].second[pid_index].pes_vdet.interlaced_indicator;
	video_det->level = m_programes[prog_index].second[pid_index].pes_vdet.level;
	video_det->profile = m_programes[prog_index].second[pid_index].pes_vdet.profile;
	video_det->timeperfieldNom = m_programes[prog_index].second[pid_index].pes_vdet.timeperfieldNom;
	video_det->timeperfieldDenom = m_programes[prog_index].second[pid_index].pes_vdet.timeperfieldDenom;
	video_det->width = m_programes[prog_index].second[pid_index].pes_vdet.width;
	video_det->afd =  m_programes[prog_index].second[pid_index].pes_vdet.afd;
}

void CParser::get_sub_desc(int prog_index, int pid_index, int sub_index, subtitling_t* sub)
{
	memcpy(sub->language,m_programes[prog_index].second[pid_index].subtitling_descriptors_v[sub_index].language,4);
	sub->type=m_programes[prog_index].second[pid_index].subtitling_descriptors_v[sub_index].type;
	sub->composition_page_id = m_programes[prog_index].second[pid_index].subtitling_descriptors_v[sub_index].composition_page_id;
	sub->ancillary_page_id = m_programes[prog_index].second[pid_index].subtitling_descriptors_v[sub_index].ancillary_page_id;
}

void CParser::get_ttx_desc(int prog_index, int pid_index, int ttx_index, teletext_ebu_t* ttx )
{
	memcpy(ttx->language,m_programes[prog_index].second[pid_index].teletext_descriptors_v[ttx_index].language,4);
	ttx->magazine_number = m_programes[prog_index].second[pid_index].teletext_descriptors_v[ttx_index].magazine_number;
	ttx->page_number = m_programes[prog_index].second[pid_index].teletext_descriptors_v[ttx_index].page_number;
	ttx->type=m_programes[prog_index].second[pid_index].teletext_descriptors_v[ttx_index].type;
}

u_short CParser::get_pat_pid(int prog_num)
{
	int size = m_pat.list.size();
	u_short pid=0;
	for (int i=0; i<size; i++)
	{
		if (prog_num==m_pat.list[i].first)
		{
			pid = m_pat.list[i].second.first;
			break;
		}
	}
	return pid;
}


u_short CParser::get_current_next_event(int prog, char* current, char* next)
{
	CAutoLock cAutoLock(&m_pLock); // Current and next event are accessed by application and demux thread.
	
	return 0;
}

void CParser::get_pmt_types(int prog_index, int pid_index, u_short* pid, int* type)
{
	*pid = m_programes[prog_index].second[pid_index].pid;
	*type = m_programes[prog_index].second[pid_index].stream_type;
}

int CParser::services_count()
{
	return m_global_services.size();
}

void CParser::get_service_descriptor(int index, unsigned char* provider, unsigned char* name, u_short* prog)
{
	*prog = m_global_services[index].prog;
	memcpy(name, &(m_global_services[index].service_name),MAX_SERVICE_NAME_LEN);
	memcpy(provider,&(m_global_services[index].service_provider),MAX_SERVICE_NAME_LEN);
}

piddetails_t* CParser::get_details(int pid)
{
	int progs = m_programes.size();

	for (int i=0; i<progs; i++)
	{
		int pidcount = m_programes[i].second.size();
		for (int j=0; j<pidcount; j++)
		{
			if (m_programes[i].second[j].pid==pid)
				return &(m_programes[i].second[j]);
		}	
	}
	return NULL;
}

DWORD CParser::toDsMpeg2Profile(DWORD profile)
{
   return 6 - profile;
}

DWORD CParser::toDsMpeg2Level(DWORD level)
{
    switch (level)
    { 
        case 6:
            level = 3;
            break;

        case 8:
              level = 2;
              break;
        case 10:
            level = 1;
            break;
		
		default: level=2;
    }
    return level;
}

WORD CParser::toDsMpeg2Layer(WORD layer)
{
	switch (layer)
	{
		case 1:
			layer = 1;
			break;

		case 2:
			layer = 2;
			break;

		case 3:
			layer = 4;
			break;

		default : layer = 2;
	
	}
	return layer;
}

bool CParser::findPacketStructure(char* buf, int len)
{
	int possiblestart=-1 ;
	int possiblestopoff=-1;
	int plength=-1;
	int counter=0;
	int possiblehdrlen=0;
	int rst=0;

	possiblestart =  findbyte(buf,MAX_PACKET_SIZE,SYNC_BYTE);
	if (possiblestart!=-1 && possiblestart+MIN_PACKET_LENGTH < len)
		possiblestopoff = findbyte(&buf[possiblestart+MIN_PACKET_LENGTH],MAX_PACKET_SIZE, SYNC_BYTE);
	if (possiblestopoff!=-1)
	{	
	    //1. Is it standard ts or ts with timecode (ALIGNED)
		plength = possiblestopoff +MIN_PACKET_LENGTH ;
		possiblehdrlen = possiblestart;

		for ( int j=plength+possiblehdrlen; j< len ; j+=plength)
		{
			if (buf[j] == SYNC_BYTE)
				counter +=1;
			else
				rst+=1;
			if (counter >=8)
			{
				if (possiblehdrlen == 4)
				{
					m_ts_structure.type = TS_TIMECODE;
					m_ts_structure.packet_len = plength-possiblehdrlen;
				}
				else
				    m_ts_structure.packet_len = plength;

				m_ts_structure.start_ofs = 0;
			    return true; 
			}
			if (rst>2)
				break;
		}

		//2. standard ts - not aligned
		rst=0;
		counter=0;
		for ( int j=plength+possiblehdrlen; j< len ; j+=plength)
		{
			if (buf[j] == SYNC_BYTE)
				counter +=1;
			else
			{
				rst+=1;
			    counter = 0;
				j = findbyte(&buf[j],MAX_PACKET_SIZE, SYNC_BYTE);
				if (j==-1)
					break;
				int tmp = findbyte(&buf[j+MIN_PACKET_LENGTH],MAX_PACKET_SIZE, SYNC_BYTE);
				if (tmp==-1)
					break;
				plength = tmp +MIN_PACKET_LENGTH ;
				possiblehdrlen = j;
			}
			if(rst>2)
				break;
			if (counter >8)  // RTP won't match this
			{
				m_ts_structure.type = TS_STANDARD;
				m_ts_structure.packet_len = plength;
				m_ts_structure.start_ofs = possiblehdrlen;
			    return true; 
			}
		}

		//3 Is it RTP ??? (aligned)
		counter = 0;
		rtp_header rhdr;
		parse_rtp(buf,len,&rhdr);

		if ( buf[rhdr.header_length] == SYNC_BYTE)
		{
		    // looking good
			// start counting packets
			plength = MIN_PACKET_LENGTH; // Rtp shouldn't have larger packet than 188!

			for ( int i = rhdr.header_length+plength; i< len; i+=plength)
			{
			    if (buf[i] == SYNC_BYTE)
					counter+=1;
				else
					break;
			}

			if ( counter <=8) // MTU for udp is normally 1500: 7 packets 
			{
				m_ts_structure.type = TS_OVERRTP;
				m_ts_structure.packet_len = plength;
				m_ts_structure.start_ofs=0;  
				m_ts_structure.rtphdr_len = rhdr.header_length;
				m_ts_structure.packets_in_mtu = counter+1;
				m_first_rtp_hdr = rhdr;
				return true;
			}
		}

		// 4 TODO: Is it mpeg TS with some unknown header 
	}

	return false;
}

int CParser::findbyte(char* buf, int len, BYTE byte)
{
	for (int i=0; i<len; i++)
	{
		if (buf[i] == byte)
			return i;
	}
	return -1;
}

int CParser::findshort(char*buf, int len, short bytes)
{
	len -=1;

	for (int i=0; i<len; i++)
	{
		if (buf[i] == ((char*)(&bytes))[0] && buf[i+1]==((char*)(&bytes))[1] )
			return i;
	}
	return -1;
}