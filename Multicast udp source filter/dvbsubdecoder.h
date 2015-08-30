//Copyright (c) 2011, Uroš Steklasa
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//	* Redistributions may not be sold, nor may they be used in a commercial product or activity.
//	* Redistributions of source code and/or in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Uroš Steklasa "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//version 0.90

#ifndef H_DVBSUB
#define H_DVBSUB

#include "bits.h"

#define COMPLETE_DISPLAY_SET 0x0
#define DISPLAY_SET_NOT_COMPLETE 0x1
#define NOT_DVB_SUBTITLE 0x2

#define MAX_REGIONS 16
#define MAX_OBJECTS 50
#define MAX_CLUTS MAX_REGIONS

#define MIN_SUB_WIDTH 720
#define MIN_SUB_HEIGHT 576
//#define IMAGE_PIXELS SUB_WIDTH*SUB_HEIGHT
//#define IMAGE_BUFFER_SIZE IMAGE_PIXELS*4
//#define ONE_LINE_BUFFER SUB_WIDTH

#define SEGMENT_PAGE_COMPOSITION	 0x10
#define SEGMENT_REGION_COMPOSITION   0x11
#define SEGMENT_CLUT_DEFENITION		 0x12
#define SEGMENT_OBJECT_DATA			 0x13
#define SEGMENT_DISPLAY				 0x14
#define SEGMENT_END_OF_DISPLAY		 0x80
#define DVBSUB_STUFFING              0xff

#define PAGE_STATE_NORMAL_CASE       0x0
#define PAGE_STATE_ACQUISITION_POINT 0x1
#define PAGE_STATE_MODE_CHANGE       0x2

#define COLORSPACE_AYUV 0x10
#define COLORSPACE_ARGB 0x11

typedef unsigned char  BYTE;


struct region_pos_t
{
	BYTE region_id;
	u_short hor_address;
	u_short ver_address;
};

struct page_t
{
	BYTE page_version;
	BYTE page_state;
	BYTE timeout;
	struct region_pos_t regions[MAX_REGIONS];
	BYTE region_count;
};

struct object_met_t
{
	u_short object_id;
	BYTE type;
	BYTE provider_flag;
	u_short hor_position;
	u_short ver_position;
	BYTE foreground_px_code;
	BYTE background_px_code;

	object_met_t()
	{
		object_id=0;
	}

};

struct object_t
{
	u_short object_id;
	BYTE version_number;

	object_t()
	{
		object_id=0;
	}
};

struct region_t
{
	bool used;
	BYTE region_id;
	BYTE version_number;
	bool fill_flag;
	u_short width;
	u_short height;
	BYTE level_of_compatibility;
	BYTE depth;
	BYTE CLUT_id;
	BYTE _8bpx_code;
	BYTE _4bpx_code;
	BYTE _2bpx_code;
	object_met_t object_metrics[MAX_OBJECTS];
	BYTE object_count;

	region_t()
	{
		region_id=0;
	}
};

struct _32BIT_PIXEL
{
    BYTE A;
	BYTE B;
	BYTE C;

	BYTE T;
};

struct YCBCR_PALETTE
{
	BYTE		Cb;
	BYTE		Cr;
	BYTE		Y;

	BYTE		T;		// HDMV rule : 0 transparent, 255 opaque (compatible DirectX)

	operator _32BIT_PIXEL()
	{
	    _32BIT_PIXEL conv;
		conv.A = Cb;
		conv.B= Cr;
		conv.C= Y;
		conv.T=T;
		return conv;
	}

};

struct RGB_PALLETE
{
	BYTE	blue;
	BYTE	green;	
	BYTE	red;

	BYTE	T;

	operator _32BIT_PIXEL()
	{
	    _32BIT_PIXEL conv;
		conv.A = blue;
		conv.B= green;
		conv.C= red;
		conv.T=T;
		return conv;
	}
};

struct CLUT_t
{
	bool used;
	BYTE CLUT_id;
	BYTE version_number;
	u_short size; // 4, 16, 256
	_32BIT_PIXEL pallete[256];
};

struct img_dimensions_t
{
	int width;
	int height;
	int arx;
	int ary;

	img_dimensions_t()
	{
	    width =MIN_SUB_WIDTH;
		height =MIN_SUB_HEIGHT;
		arx=1;
		ary=1;
	};
};

struct display_parameters_t
{
	int intended_width;
	int intended_height;
	int m_display_window_horizontal_position_minimum;
	int m_display_window_horizontal_position_maximum;
	int m_display_window_vertical_position_minimum;
	int m_display_window_vertical_position_maximum;

	void display_parameters()
	{
		int intended_width=-1;
		int intended_height=-1;
		int m_display_window_horizontal_position_minimum=-1;
		int m_display_window_horizontal_position_maximum=-1;
		int m_display_window_vertical_position_minimum=-1;
		int m_display_window_vertical_position_maximum=-1;
	}
};

struct dirty_img_part_t
{
	int top;
	int bottom;
	int left;
	int right;

};

struct img_overlay_offset_t
{
    int x;
	int y;

	img_overlay_offset_t()
	{
	    x=0;
		y=0;
	};
};

class CDvbSub:CBits
{
	private:

		BYTE m_colorspace;
		bool m_first_page_acquiered;
		bool m_page_clear;

		page_t m_current_page;
		region_t m_regions[MAX_REGIONS];
		int m_next_free_region;
		CLUT_t m_cluts[MAX_CLUTS];
		CLUT_t * m_last_clut;
		int m_next_free_clut;	

		BYTE SubtitlingSegment(char* segment, int* len, u_short req_paqge_id, u_short ancillary_id, BYTE* time_out);
		void DisplayCompositionSegment(char* segment_data, int len);
		BYTE PageCompositionSegment(char* segment_data, int len, BYTE* time_out);
		void RegionCompositionSegment(char* segment_data, int len);
		void CLUTDefinitionSegment(char* segment_data, int len);
		void ObjectDataSegment(char* segment_data, int len);
		void pixel_data_sub_block(char* sub_block, int len,region_t* region, region_pos_t* region_pos, CLUT_t* clut, object_met_t* object_met, object_t* object, int line_start);
		bool decode_2bit_pixel_code_string(char* sub_block, int* byte_offset, BYTE* offset, _32BIT_PIXEL* string, int* string_len,CLUT_t* clut, region_t* region);
		bool decode_4bit_pixel_code_string(char* sub_block, int* byte_offset, BYTE* offset, _32BIT_PIXEL* string, int* string_len,CLUT_t* clut, region_t* region);
		bool decode_8bit_pixel_code_string(char* sub_block, int* byte_offset, BYTE* offset, _32BIT_PIXEL* string, int* string_len,CLUT_t* clut, region_t* region);
		
		region_t* find_free_region(BYTE region_id);
		bool find_free_CLUT(BYTE id, CLUT_t** clut);
		region_pos_t* get_region_position(BYTE region_id);
		region_t* find_region(u_short object_id);
        CLUT_t* find_CLUT(BYTE id);
		_32BIT_PIXEL get_pixel_from_clut(BYTE code, CLUT_t* clut, BYTE bitppx, BYTE level_of_comp);

		void plot(_32BIT_PIXEL* pixel, int pixels, int line, int xpos);
		void fill_region(_32BIT_PIXEL code, int x, int y, int width, int height);
		RGB_PALLETE YCBCRtoRGB(YCBCR_PALETTE ycbcrpallete);
		YCBCR_PALETTE RGBtoYCBCR(RGB_PALLETE rgbpallete);

		_32BIT_PIXEL m_256_clut(BYTE code);
		_32BIT_PIXEL m_16_clut(BYTE code);
		_32BIT_PIXEL m_4_clut(BYTE code);

		CLUT_t m_default_clut_256;
		CLUT_t m_default_clut_16;
		CLUT_t m_default_clut_4;

		_32BIT_PIXEL* m_img;
		img_dimensions_t m_img_dim;
		dirty_img_part_t m_img_dirty;
		display_parameters_t m_dds;
		img_overlay_offset_t m_ovr_offset;

		BYTE m_dds_version_num;

	public:

		CDvbSub();
		~CDvbSub();

		BYTE PreProcessor(char* subPES,int subPESlen, u_short req_paqge_id, u_short ancillary_id, _32BIT_PIXEL** output, BYTE* time_out);
		void GetCurrentPage(_32BIT_PIXEL** img);
        _32BIT_PIXEL* GetCurrentPage( BOOL* is_empty_img);
		void GetCurrentPageClone(_32BIT_PIXEL* img);
		void clear_img();
		void set_output_color_space(BYTE);
		int get_image_size();
		void get_image_size(int *pWidth, int *pHeight);
		void get_display_properties(int* intended_width, int* intended_height, int*,int*,int*,int*);
		void report_preferred_image_size(int width, int height, int arx, int ary, bool center);

};

#endif