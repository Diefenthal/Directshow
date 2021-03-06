//Copyright (c) 2011, Uro� Steklasa
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//	* Redistributions may not be sold, nor may they be used in a commercial product or activity.
//	* Redistributions of source code and/or in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Uro� Steklasa "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//version 0.99

#include "dvbsubdecoder.h"

const BYTE  _2_to_4_bit_map_table[]=
{
	0,
	7,
	8,
	15
};

const BYTE  _2_to_8_bit_map_table[]=
{
	0,
	119,
	136,
	255
};

const BYTE  _4_to_8_bit_map_table[]=
{
	0,
	17,
	34,
	51,
	68,
	85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	220,
	238,
	255
};

// constructor
CDvbSub::CDvbSub()
{
	m_first_page_acquiered=false;
	m_page_clear=true;

	m_dds_version_num=0;

	m_next_free_region = -1;
	for (int i=0; i< MAX_REGIONS;i++)
	{
		ZeroMemory(&(m_regions[i]), sizeof (region_t));
	}

	m_last_clut=NULL;

	m_next_free_clut = -1;
	for (int j=0; j< MAX_CLUTS; j++)
	{
		m_cluts[j].used=false;
		m_cluts[j].CLUT_id=0;
		m_cluts[j].size=0;
		m_cluts[j].version_number=0;
	}

	m_colorspace = COLORSPACE_ARGB;

	m_default_clut_256.size=256;
	m_default_clut_256.used=true;
	m_default_clut_16.size=16;
	m_default_clut_16.used=true;
	m_default_clut_4.size=4;
	m_default_clut_4.used=true;

	// default CLUTS
	for (int i=0; i<256; i++)
		m_default_clut_256.pallete[i] = m_256_clut(i);

	for (int i=0; i<16; i++)
		m_default_clut_16.pallete[i] = m_16_clut(i);

	for (int i=0; i<4; i++)
		m_default_clut_4.pallete[i] = m_4_clut(i);

	m_img=NULL;
	ZeroMemory(&m_img_dirty,sizeof(m_img_dirty));

	//allocate default size
	int pixels = m_img_dim.width*m_img_dim.height;	
	m_img = new _32BIT_PIXEL[pixels];
	for (int i=0; i< pixels; i++)
		m_img[i].T = 0;

}
//
//// descructor
CDvbSub::~CDvbSub()
{
	if (m_img !=NULL)
	{
		delete[] m_img;
		m_img = NULL;
	}
}

BYTE CDvbSub::PreProcessor(char* subPES,int subPESlen, u_short req_paqge_id, u_short ancillary_id, _32BIT_PIXEL** output, BYTE* time_out)
{
	if (output != NULL)
		*output = &(m_img[0]);
	
	BYTE data_identifier = subPES[0];
	BYTE subtitle_stream_id = subPES[1];

	if (data_identifier!=0x20 || subtitle_stream_id!=0x0)
		return NOT_DVB_SUBTITLE;

	int i=2;
	int len;
	BYTE segment_result=DISPLAY_SET_NOT_COMPLETE;
	while(subPES[i]==0xf && i<subPESlen)
	{
		segment_result = SubtitlingSegment(&(subPES[i]),&len,req_paqge_id,ancillary_id, time_out);
		i+=len;
		if (segment_result==COMPLETE_DISPLAY_SET)
			break;
	}

	if (subPES[i] != 0xff)
	{
		//OutputDebugString(TEXT("End marker not found"));
	}

	return segment_result;
}

void CDvbSub::GetCurrentPage(_32BIT_PIXEL** output)
{
	*output = &(m_img[0]);
}

// Correct in client !!!!!!!!!
void CDvbSub::GetCurrentPageClone(_32BIT_PIXEL* img)
{
	memcpy(img,m_img, m_img_dim.height*m_img_dim.width*4 );
}

BYTE CDvbSub::SubtitlingSegment(char* segment, int* len, u_short req_page_id, u_short ancillary_id, BYTE* time_out)
{

	BYTE result= DISPLAY_SET_NOT_COMPLETE;

	BYTE segment_type = segment[1];
	u_short page_id = (segment[2]&0xff)<<8 | (BYTE)segment[3];
	u_short segment_len = (segment[4]&0xff)<<8 | (BYTE)segment[5];
	*len = (int)segment_len+6;
	int i=6;

	BYTE page_state;

	if (page_id == req_page_id || page_id == ancillary_id)
	{
		switch(segment_type)
		{
			case SEGMENT_END_OF_DISPLAY:
				result = COMPLETE_DISPLAY_SET;
			break;

			case SEGMENT_DISPLAY:
				DisplayCompositionSegment(&(segment[6]),(int)segment_len);
				break;	
			
			case SEGMENT_PAGE_COMPOSITION:
				page_state = PageCompositionSegment(&(segment[6]), (int)segment_len, time_out);
				if (!m_first_page_acquiered && (page_state ==PAGE_STATE_ACQUISITION_POINT || page_state == PAGE_STATE_MODE_CHANGE) )
					m_first_page_acquiered=true;
				break;

			case SEGMENT_REGION_COMPOSITION:
				RegionCompositionSegment(&(segment[6]),(int)segment_len);
				break;

			case SEGMENT_CLUT_DEFENITION:
				CLUTDefinitionSegment(&(segment[6]),(int)segment_len);
				break;

			case SEGMENT_OBJECT_DATA:
				//if (m_first_page_acquiered)
					ObjectDataSegment(&(segment[6]),(int)segment_len);
				break;

			case DVBSUB_STUFFING:
				while (segment[i]==0xff)
				{
					*len+=1;
				}
				break;
			
			default:

				break;
		}
	}

	return result;
}

void CDvbSub::DisplayCompositionSegment(char* segment_data, int len)
{
	BYTE dds_version_num = segment_data[0] >> 4;
	BYTE flag;

	if (dds_version_num = m_dds_version_num)
		return; // nothing has changed

	int current_img_size =m_img_dim.width * m_img_dim.height;

	flag = (segment_data[0]&0x8) >> 3;

 	m_dds.intended_width= ( (segment_data[1] <<8) | (BYTE)segment_data[2])+1;
	m_dds.intended_height = ( (segment_data[3] <<8) | (BYTE)segment_data[4])+1;

	if (flag)
	{
		m_dds.m_display_window_horizontal_position_minimum = ((segment_data[5] <<8) | (BYTE)segment_data[6])+1;
		m_dds.m_display_window_horizontal_position_maximum = ((segment_data[7] <<8) | (BYTE)segment_data[8])+1;
		m_dds.m_display_window_vertical_position_minimum = ((segment_data[9] <<8) | (BYTE)segment_data[10])+1;
		m_dds.m_display_window_vertical_position_maximum = ((segment_data[11] <<8) | (BYTE)segment_data[12])+1;

		m_img_dim.width = m_dds.intended_width - m_dds.m_display_window_horizontal_position_maximum - m_dds.m_display_window_horizontal_position_minimum;
		m_img_dim.width = m_dds.intended_height - m_dds.m_display_window_vertical_position_maximum - m_dds.m_display_window_vertical_position_minimum;
	}
	else
	{
	    m_img_dim.width = m_dds.intended_width;
		m_img_dim.height= m_dds.intended_height;
	}
	
	int pixels = m_img_dim.width * m_img_dim.height;
	//Reallocate
	if (current_img_size != pixels)
	{
	    delete[] m_img;
		m_img = new _32BIT_PIXEL[pixels];
		for (int i=0; i< pixels; i++) // clear new imge
			m_img[i].T = 0;
	}
}

BYTE CDvbSub::PageCompositionSegment(char* segment_data, int len, BYTE* time_out)
{
	
	*time_out =  (BYTE)segment_data[0];
	BYTE page_version_number = (segment_data[1]&0xf0) >> 4;
	BYTE page_state = (segment_data[1]&0xc) >> 2;
	
	ZeroMemory(&m_current_page,sizeof(page_t));

	m_current_page.timeout = *time_out;
	m_current_page.page_state = page_state;

	//if (page_state == 1 || page_state == 2)
	//{
	////	//clear page
	//  	clear_img();
	//}

	//if (m_current_page.page_version == page_version_number)
	//	return page_state;

	m_current_page.page_version = page_version_number;
	m_current_page.region_count=0;

	bool clear_prev_dirty_img=false;
	int region=0;
	u_short hor_address;
	u_short ver_address;
	for (int i=2; i< len;i=i+6)
	{
		if (m_current_page.regions[region].region_id!=(BYTE)segment_data[i])
           clear_prev_dirty_img=true;

		m_current_page.regions[region].region_id=(BYTE)segment_data[i];
	    
		hor_address = (segment_data[i+2]&0xff)<<8 | (BYTE)segment_data[i+3];
        ver_address =  (segment_data[i+4]&0xff)<<8 | (BYTE)segment_data[i+5];
		
		if (m_current_page.regions[region].hor_address != hor_address)
		{
		    m_current_page.regions[region].hor_address = hor_address;
            clear_prev_dirty_img=true; // Region was moved horizontally
		}
		if ( m_current_page.regions[region].ver_address !=  ver_address)
		{
		     m_current_page.regions[region].ver_address =  ver_address;
            clear_prev_dirty_img=true;// Region was moved vertically
		}
		region+=1;
		if(region >=MAX_REGIONS)
			break;
	}

	if (m_current_page.region_count!=region) // Regions were removed or added
	{
	    m_current_page.region_count=region;
        clear_prev_dirty_img=true;
	}

	if (clear_prev_dirty_img ||  region == 0 )
		clear_img(); // Only dirty regions

	//Clear dirty region parameters.
	 m_img_dirty.top = 0;
	 m_img_dirty.bottom = m_img_dim.height;
	 m_img_dirty.left =0;
	 m_img_dirty.right = m_img_dim.width;

	return page_state;
}

void CDvbSub::RegionCompositionSegment(char* segment_data, int len)
{
	BYTE region_id = (BYTE)segment_data[0];
	BYTE version_number = ((segment_data[1])&0xf0) >> 4;
	
	region_t* region = find_free_region(region_id);
	
	//if (region->used && region->version_number==version_number)
	//	return;
	//
	region->used=true;
	region->region_id = region_id;
	region->version_number = version_number;

	if ( (segment_data[1]&0x8) >> 3 == 1)
		region->fill_flag=true;
	else
		region->fill_flag=false;

	region->width = (segment_data[2]&0xff)<<8 | (BYTE)segment_data[3];
	region->height = (segment_data[4]&0xff)<<8 | (BYTE)segment_data[5];
	region->level_of_compatibility = (segment_data[6]&0xe0) >> 5;
	region->depth = (segment_data[6]&0x1c)>>2;
	region->CLUT_id = (BYTE)segment_data[7];
	region->_8bpx_code = (BYTE)segment_data[8];
	region->_4bpx_code = (segment_data[9]&0xf0)>>4;
	region->_2bpx_code = (segment_data[9]&0xc)>>2;
	region->object_count = 0;

	int object=0;
	for (int i = 10; i<len; i+=6)
	{
		region->object_metrics[object].object_id = (segment_data[i]&0xff)<<8 | (BYTE)segment_data[i+1];
		region->object_metrics[object].type = (segment_data[i+2]&0xc0)>>6;
		region->object_metrics[object].provider_flag = (segment_data[i+2]&0x30)>>4;
		region->object_metrics[object].hor_position = (segment_data[i+2]&0xf)<<8 | (BYTE)segment_data[i+3];
		region->object_metrics[object].ver_position = (segment_data[i+4]&0xf)<<8 | (BYTE)segment_data[i+5];

		if (region->object_metrics[object].type==0x01 || region->object_metrics[object].type==0x02)
		{
			region->object_metrics[object].foreground_px_code = (BYTE)segment_data[i+5];
			region->object_metrics[object].background_px_code = (BYTE)segment_data[i+6];
			i+=2;
		}

		object+=1;
		region->object_count = object;

		if (object>=MAX_OBJECTS)
			break;
	}

	CLUT_t* clut = NULL;
	_32BIT_PIXEL bckg;
	
	// Check if region is in curerent page and fill background or clear page if all objects are removed.		
	for (int i=0; i< m_current_page.region_count; i++)
	{
		if (m_current_page.regions[i].region_id == region->region_id)
		{
			if (region->fill_flag==true)
			{
				clut = find_CLUT(region->CLUT_id);
				if (clut!=NULL)
				{
					if (region->depth == 0x03)
						bckg = get_pixel_from_clut(region->_8bpx_code,clut,8,region->level_of_compatibility);
					else if ( (region->depth == 0x02 || region->depth == 0x03) && (region->level_of_compatibility==0x2 || region->level_of_compatibility==0x1 ) )
						bckg = get_pixel_from_clut(region->_4bpx_code,clut,4,region->level_of_compatibility);
					else if (region->level_of_compatibility==0x1 )
						bckg = get_pixel_from_clut(region->_2bpx_code,clut,2,region->level_of_compatibility);

					fill_region(bckg, m_current_page.regions[i].hor_address, m_current_page.regions[i].ver_address, region->width, region->height);
		        }
			}

			if (object==0)
				clear_img();

			if (i==0)
			{
                 // store dirty region for current page
			     m_img_dirty.top = m_current_page.regions[i].ver_address;
				 m_img_dirty.bottom = m_current_page.regions[i].ver_address+region->height;
				 m_img_dirty.left = m_current_page.regions[i].hor_address;
				 m_img_dirty.right = m_current_page.regions[i].hor_address+ region->width;
			}
			else
			{
				// expand dirty range if necessary
				 m_img_dirty.top =min(m_img_dirty.top, m_current_page.regions[i].ver_address);
				 m_img_dirty.bottom = max(m_img_dirty.bottom,m_current_page.regions[i].ver_address+region->height);
				 m_img_dirty.left =min(m_img_dirty.left, m_current_page.regions[i].hor_address);
				 m_img_dirty.right = max(m_img_dirty.right,m_current_page.regions[i].hor_address+ region->width);
			}
	    }
	}
}

void CDvbSub::CLUTDefinitionSegment(char* segment_data, int len)
{
	// Find free CLUT or CLUT for modification
	CLUT_t* clut=NULL;
	BYTE id = (BYTE)segment_data[0];
	BYTE version_number = (segment_data[1]& 0xf0) >> 4;

	find_free_CLUT(id,&clut);
	
	// check CLUT version
	if (clut->used && clut->version_number == version_number )
	{
		return; // Nothing to update
	}

	clut->used=true;
	clut->CLUT_id=id;
	clut->version_number = version_number;
	clut->size=0;

	m_last_clut =clut;

	YCBCR_PALETTE ycbcr_pallete; // Subtitles are YUV encoded
	
	BYTE entry_id=0;
	BYTE full_range_flag=0;
	BYTE _2bit_entry_flag=0;
	BYTE _4bit_entry_flag=0;
	BYTE _8bit_entry_flag=0;	
	
	for (int i=2; i<len; i+=2)
	{
		entry_id = (BYTE)segment_data[i];
		full_range_flag = (BYTE)(segment_data[i+1]&0x1);
		_2bit_entry_flag = (segment_data[i+1]&0x80) >> 7;
		_4bit_entry_flag = (segment_data[i+1]&0x40) >> 6;
		_8bit_entry_flag = (segment_data[i+1]&0x20) >> 5;

		if (full_range_flag==1)
		{
			ycbcr_pallete.Y= segment_data[i+2];
			ycbcr_pallete.Cr= segment_data[i+3];
			ycbcr_pallete.Cb= segment_data[i+4];
			ycbcr_pallete.T= segment_data[i+5];

			i+=4;
		}
		else
		{
			ycbcr_pallete.Y= (segment_data[i+2]&0xfc)>>2;
			ycbcr_pallete.Cr= (segment_data[i+2]&0x3)<<2 | ((segment_data[i+3]&0xc0)>>6) ;
			ycbcr_pallete.Cb= (segment_data[i+3]&0x3c)>>2 ;
			ycbcr_pallete.T= (segment_data[i+3]&0x3);

			i+=2;
		}

		if (ycbcr_pallete.Y==0)
		    ycbcr_pallete.T = 255; 
		else if (ycbcr_pallete.T == ycbcr_pallete.Y+1)
            ycbcr_pallete.T=0; //
		////else

		if (m_colorspace == COLORSPACE_ARGB)
		    clut->pallete[entry_id] = YCBCRtoRGB(ycbcr_pallete);
		else
		{
            clut->pallete[entry_id]= ycbcr_pallete;
		}

       clut->pallete[entry_id].T = 255 - ycbcr_pallete.T; // Directshow !!!
          
	   clut->size = max(entry_id+1,clut->size);
	}
	
	if (clut->size != 4 && clut->size!=16 && clut->size!=256)
	{
		if (_8bit_entry_flag)
			clut->size=256;
		else if (_4bit_entry_flag)
			clut->size=16;
		else if (_2bit_entry_flag)
			clut->size=4;
	}
}


void CDvbSub::ObjectDataSegment(char* segment_data, int len)
{

	object_t object;

	object.object_id = (segment_data[0]&0xff)<< 8 | (BYTE)segment_data[1];
	object.version_number = (segment_data[2]&0xf0) >> 4; 
	BYTE coding_method = (segment_data[2]&0xc) >> 2;

	if (coding_method == 0x0) // Only pixel based method is supported
	{
		u_short top_field_data_block_len = (segment_data[3]&0xff)<<8 | (BYTE)segment_data[4];
		u_short bottom_field_data_block_len = (segment_data[5]&0xff)<<8 | (BYTE)segment_data[6];
		

		region_t* region = find_region(object.object_id);
		if (region == NULL)
			return;

		region_pos_t* region_pos = get_region_position(region->region_id);
	    if (region_pos==NULL)
		    return ;  

		CLUT_t* clut = find_CLUT(region->CLUT_id);
		
		// Use previous CLUT
		if (clut == NULL)
		{
			if (m_last_clut !=NULL)
				clut = & m_cluts[0];
		}

		//Use default clut if CLUT wasn't received.
		if (clut == NULL)
		{
			if (region->depth == 0x01)
				clut = &m_default_clut_4;
		
			else if (region->depth==0x02)
				clut = &m_default_clut_16;

			else if (region->depth==0x03)
				clut = &m_default_clut_256;
			else
				return;
		}

		
		for (int j=0 ; j<region->object_count; j++)
		{
			if (region->object_metrics[j].object_id == object.object_id)
			{
				pixel_data_sub_block(&(segment_data[7]),top_field_data_block_len,region, region_pos, clut, &(region->object_metrics[j]), &object,0);
				if (bottom_field_data_block_len==0)
					 pixel_data_sub_block(&(segment_data[7]),top_field_data_block_len,region, region_pos, clut, &(region->object_metrics[j]), &object,1);
				else
					pixel_data_sub_block(&(segment_data[7+top_field_data_block_len]),bottom_field_data_block_len,region, region_pos, clut,  &(region->object_metrics[j]),  &object,1);

				m_page_clear=false;
			}
		}
	}
}

void CDvbSub::pixel_data_sub_block(char* sub_block, int len, region_t* region, region_pos_t* region_pos,  CLUT_t* clut, object_met_t* object_met, object_t* object, int line)
{
	int xpos =0;
	BYTE bit_offset=0;
	int byte_pos = 0;

	_32BIT_PIXEL* object_string = new _32BIT_PIXEL[m_img_dim.width];
	int string_pos=0;
	BYTE data_type;	
    bool end_code=false;

	while ( byte_pos < len )
	{
		data_type = (BYTE)read_bits(sub_block,&byte_pos,&bit_offset,8);
		end_code=false;
		
		if (data_type == 0x10)
		{
			do
			{
				end_code = decode_2bit_pixel_code_string(sub_block, &byte_pos, &bit_offset,&(object_string[0]),&string_pos,clut,region);
			}
			while(end_code ==false);

			while (bit_offset != 0)
			{
				read_bits(sub_block,&byte_pos,&bit_offset,2);
			}
		}

		else if (data_type == 0x11)
		{
			do
			{
				end_code = decode_4bit_pixel_code_string(sub_block, &byte_pos, &bit_offset, &(object_string[0]),&string_pos,clut,region);
			}
			while(end_code ==false);

			if (bit_offset != 0)
			{
				read_bits(sub_block,&byte_pos,&bit_offset,4);
			}
		}

		else if (data_type == 0x12)
		{
			do
			{
				end_code = decode_8bit_pixel_code_string(sub_block, &byte_pos, &bit_offset, &(object_string[0]),&string_pos,clut, region);
			}
			while(end_code ==false);

		}

		else if (data_type == 0x20)
		{
			// 2->4 map //16
			for (int i=0; i<8;i++)
			{
				object_string[string_pos] = clut->pallete[_2_to_4_bit_map_table[read_bits(sub_block,&byte_pos,&bit_offset,2)]];
				string_pos+=1;
			}
		}

		else if (data_type == 0x21)
		{
			// 2->8 map //32
			for (int i=0; i<16;i++)
			{
				object_string[string_pos] = clut->pallete[_2_to_8_bit_map_table[read_bits(sub_block,&byte_pos,&bit_offset,2)]];
				string_pos+=1;
			}
		}

		else if (data_type == 0x22)
		{
			// 4->8 map //128
			for (int i=0; i<32;i++)
			{
				object_string[string_pos] = clut->pallete[_4_to_8_bit_map_table[read_bits(sub_block,&byte_pos,&bit_offset,4)]];
				string_pos+=1;
			}
		}

		else if (data_type == 0xf0) //new line
		{
			if (string_pos < m_img_dim.width)
				plot(&(object_string[0]),string_pos, region_pos->ver_address + object_met->ver_position+line, region_pos->hor_address+ object_met->hor_position);
			line+=2;
			string_pos=0;  //reset
			continue;
		}

		else  if (data_type ==0)
		{
			continue;
		}

		else
		{
			break;
		}
	}

	delete object_string;
}

bool CDvbSub::decode_2bit_pixel_code_string(char* sub_block, int* byte_pos,  BYTE* offset, _32BIT_PIXEL* string, int* string_pos, CLUT_t* clut, region_t* region)
{
	
	BYTE pseudo_code;
	bool end_found=false;

	pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,2);

	if ( pseudo_code != 0x0)
	{
		string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
		*string_pos+=1;
	}
	else
	{
		BYTE switch_1 = (BYTE)read_bits(sub_block,byte_pos,offset,1);
		if (switch_1 == 1)
		{
			BYTE run_length_3_10 = (BYTE) read_bits(sub_block,byte_pos,offset,3) ;
			pseudo_code = (BYTE) read_bits(sub_block,byte_pos,offset,2) ;
			for (int i=0; i< run_length_3_10 + 3; i++)
			{
				string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
				*string_pos+=1;
			}
		}
		else
		{
			BYTE switch_2 = (BYTE)read_bits(sub_block,byte_pos,offset,1);
			if (switch_2==1)
			{
				pseudo_code=0;
				string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
			}

			if (switch_2 == 0)
			{
				BYTE switch_3 = (BYTE)read_bits(sub_block,byte_pos,offset,2) ;
				
				if (switch_3 == 0)
				{
					end_found =true;
				}

				else if (switch_3 == 1)
				{
					for (int j=0; j< 2; j++)
					{
						string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
						*string_pos+=1;
					}
				}

				else if (switch_3 == 2)
				{
					BYTE run_length_12_27 = (BYTE)read_bits(sub_block,byte_pos,offset,4) ;
					pseudo_code = (BYTE) read_bits(sub_block,byte_pos,offset,2) ;
					for (int k=0; k< run_length_12_27 + 12; k++)
					{
						string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
						*string_pos+=1;
					}
				}
				else if (switch_3 == 3)
				{
					BYTE run_length_29_284 = (BYTE)read_bits(sub_block,byte_pos,offset,8) ;
					pseudo_code = (BYTE) read_bits(sub_block,byte_pos,offset,2) ;
					for (int l=0; l< run_length_29_284 + 29; l++)
					{
						string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,2,region->level_of_compatibility);
						*string_pos+=1;
					}
				}
			}
		}
	}
	return end_found;
}

bool CDvbSub::decode_4bit_pixel_code_string(char* sub_block, int* byte_pos, BYTE* offset, _32BIT_PIXEL* string, int* string_pos, CLUT_t* clut, region_t* region)
{
	BYTE pseudo_code;
	bool end_found=false;

	pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,4);
	if ( pseudo_code != 0x0)
	{
		string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,4,region->level_of_compatibility);
		*string_pos+=1;
	}
	else
	{
		BYTE switch_1 = (BYTE)read_bit(sub_block,byte_pos,offset);  
		if (switch_1 == 0)
		{
			BYTE next = (BYTE)read_bits(sub_block,byte_pos,offset,3);
			if (next != 0)
			{
				BYTE run_length_3_9 = next;
				for (int i=0; i<run_length_3_9 +2 ;i++ )
				{
					string[*string_pos]=get_pixel_from_clut(0,clut,4,region->level_of_compatibility);
					*string_pos+=1;
				}
			}
			else
			{
				end_found=true;
			}
		}
		else
		{
			BYTE switch_2 = (BYTE)read_bits(sub_block,byte_pos,offset,1);
			if (switch_2==0)
			{
				BYTE run_length_4_7 = (BYTE)read_bits(sub_block,byte_pos,offset,2);
				pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,4);
				for (int k=0; k<run_length_4_7 +4 ;k++)
				{
					string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,4,region->level_of_compatibility);
					*string_pos+=1;
				}
			}
			else
			{
				BYTE switch_3 = (BYTE)read_bits(sub_block,byte_pos,offset,2);
				if (switch_3 == 2)
				{
					BYTE run_length_9_24 = (BYTE)read_bits(sub_block,byte_pos,offset,4);
					pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,4);
					for (int l=0; l<run_length_9_24 +9 ;l++)
					{
						string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,4,region->level_of_compatibility);
						*string_pos+=1;
					}
				}
				else if (switch_3 == 3)
				{
					BYTE run_length_25_280 = (BYTE)read_bits(sub_block,byte_pos,offset,8);
					pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,4);
					for (int m =0; m<run_length_25_280 +25; m++)
					{
						string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,4,region->level_of_compatibility);
						*string_pos+=1;
					}
				}
				else if (switch_3 == 0)
				{
						string[*string_pos]=get_pixel_from_clut(0,clut,4,region->level_of_compatibility);
						*string_pos+=1;
				}
				else if (switch_3==1)
				{
						string[*string_pos]=get_pixel_from_clut(0,clut,4,region->level_of_compatibility);
						*string_pos+=1;
                        string[*string_pos]=get_pixel_from_clut(0,clut,4,region->level_of_compatibility);
						*string_pos+=1;
				}
			}
		}
	}

	return end_found;
}

bool CDvbSub::decode_8bit_pixel_code_string(char* sub_block, int* byte_pos, BYTE* offset, _32BIT_PIXEL* string, int* string_pos, CLUT_t* clut, region_t* region)
{
	bool end_found=false;
	BYTE pseudo_code;

	pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,8);

	if (pseudo_code !=0)
	{
		string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,8,region->level_of_compatibility);
		*string_pos+=1;
	}
	else
	{
		BYTE switch_1 = (BYTE)read_bits(sub_block,byte_pos,offset,1);
		if (switch_1==0)
		{
			BYTE next = (BYTE)read_bits(sub_block,byte_pos,offset,7);
			if (next!=0)
			{
				BYTE run_length_1_127 =next;
				for (int i=0;i<run_length_1_127;i++)
				{
					string[*string_pos]=get_pixel_from_clut(0,clut,8,region->level_of_compatibility);
					*string_pos+=1;
				}
			}
			else
			{
				end_found=true;
			}
		}
		else
		{
			BYTE run_length_3_127 = (BYTE)read_bits(sub_block,byte_pos,offset,7);
			pseudo_code = (BYTE)read_bits(sub_block,byte_pos,offset,8);
			for (int j=0; j<run_length_3_127; j++)
			{
			    string[*string_pos]=get_pixel_from_clut(pseudo_code,clut,8,region->level_of_compatibility);
				*string_pos+=1;
			}
		}
	}

	return end_found; 
}

// line: line of an object.
// xpos: horizontal position of a first pixel in a buffer
void CDvbSub::plot(_32BIT_PIXEL* pixel_buf, int pixels,  int line, int xpos) // line and xpos of page pixel buffer
{
	for (int i=0; i<pixels;i++)
	{
		m_img[ m_ovr_offset.x+ (m_img_dim.width*(line))+xpos+i]=pixel_buf[i];
	}
}

void CDvbSub::fill_region(_32BIT_PIXEL code, int x, int y, int width, int height)
{
	int y_start = y + m_ovr_offset.y;
	int y_stop = y_start + height;
	int x_start = x +m_ovr_offset.x;
	int x_stop = x_start + width;
	
	for ( int i=y_start; i<y_stop; i++)
	{
		for (int j=x_start; j<x_stop; j++)
		{
			m_img[(m_img_dim.width*i)+j] = code;
		}
	}
}

//clear current page (only writtable part)
void CDvbSub::clear_img()
{
	if (m_page_clear)
		return;
	
	int y_start = m_ovr_offset.y+m_img_dirty.top;
	int y_stop =  m_img_dirty.bottom;
	int x_start =m_ovr_offset.x + m_img_dirty.left;
	int x_stop = m_img_dirty.right;

	for ( int i=y_start; i<y_stop; i++)
	{
		for (int j=x_start; j<x_stop; j++)
		{
			m_img[(m_img_dim.width*i)+j].T=0;
	       //m_img[(m_img_dim.width*i)+j].A=0;
	       //m_img[(m_img_dim.width*i)+j].B=0;
	       // m_img[(m_img_dim.width*i)+j].C=0;
		}
	}
	m_page_clear=true;
}

int CDvbSub::get_image_size()
{
	return m_img_dim.height*m_img_dim.width;
}

void CDvbSub::get_image_size(int* pWidth, int *pHeight)
{
	*pWidth = m_img_dim.width;
	*pHeight = m_img_dim.height;
}

void CDvbSub::get_display_properties(int* intended_width, int* intended_height, int* hor_min,int* hor_max,int* ver_min,int*ver_max)
{
	*intended_width = m_dds.intended_width;
	*intended_height = m_dds.intended_height;
	*hor_min = m_dds.m_display_window_horizontal_position_minimum;
	*hor_max = m_dds.m_display_window_horizontal_position_maximum;
	*ver_min = m_dds.m_display_window_vertical_position_minimum;
	*ver_max = m_dds.m_display_window_vertical_position_maximum;
}

void CDvbSub::report_preferred_image_size(int width, int height, int arx, int ary, bool center)
{
	// Check if video size is different than current dimensions

	if (width == m_img_dim.width  && height ==m_img_dim.height)
		return;  // No changes

	// Reallocate buffer and clear image
	if (m_img != NULL)
	{
		delete[] m_img;
		m_img=NULL;
	}

	m_img_dim.width=max(MIN_SUB_WIDTH,width);
	m_img_dim.height=max(MIN_SUB_HEIGHT,height);
	m_img_dim.arx=arx;
	m_img_dim.ary=ary;

	if (center)
	{
	m_ovr_offset.x = (m_img_dim.width - MIN_SUB_WIDTH)/2;
	m_ovr_offset.y = (m_img_dim.height - MIN_SUB_HEIGHT)/2;
	}

	int pixels = m_img_dim.width*m_img_dim.height;	
	m_img = new _32BIT_PIXEL[pixels];
	for (int i=0; i< pixels; i++)
		m_img[i].T = 0;

}

// TODO: SDTV, full HDTV see  http://www.equasys.de/colorconversion.html
// Avoid floating-point arithmetic !
RGB_PALLETE CDvbSub::YCBCRtoRGB(YCBCR_PALETTE ycbcrpallete)
{
	RGB_PALLETE pallete;

	//pallete.blue = ycbcrpallete.Cb;
	//pallete.green = ycbcrpallete.Cr;
	//pallete.red =ycbcrpallete.Y;
	//pallete.T = 255 -ycbcrpallete.T;
	
	//if (ycbcrpallete.Y ==0)
	//{
	//	ycbcrpallete.Cb=128;
	//	ycbcrpallete.Cr=128;
	//	ycbcrpallete.T=0xff;
	//}
	//

	double Y = (double)ycbcrpallete.Y;
	double Cb = (double)ycbcrpallete.Cb;
	double Cr = (double)ycbcrpallete.Cr;

	int r = (int) (Y + 1.40200 * (Cr - 128));
	int g = (int) (Y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128));
	int b = (int) (Y + 1.77200 * (Cb - 128));

	pallete.red = max(0, min(255, r));
	pallete.green = max(0, min(255, g));
	pallete.blue = max(0, min(255, b));
	pallete.T = ycbcrpallete.T;
	return pallete;
}

YCBCR_PALETTE CDvbSub::RGBtoYCBCR(RGB_PALLETE rgb)
{
	YCBCR_PALETTE ycb;
	ycb.Y = (short)((19595 * rgb.red + 38470 * rgb.green + 7471 * rgb.blue ) >> 16);
    ycb.Cb = (short)( ( 36962 * ( rgb.blue - ycb.Y ) ) >> 16);
    ycb.Cr = (short)(( 46727 * ( rgb.red - ycb.Y ) ) >> 16);
	return ycb;
}

void CDvbSub::set_output_color_space(BYTE csp)
{
    m_colorspace = csp;
}

_32BIT_PIXEL CDvbSub::get_pixel_from_clut(BYTE code, CLUT_t* clut, BYTE bitppx, BYTE level_of_comp)
{
	// Check clut size and perform reduction if necessary
	_32BIT_PIXEL pl;

	if (clut->size == 4)
	{
			if (bitppx ==2 )
				return clut->pallete[code];
			else if (bitppx ==4 )
			{
				return (clut->pallete[reduction_4_to_2(code)]);
			}
			else if (bitppx == 8)
			{
		        return (clut->pallete[reduction_8_to_2(code)]);
			}
	}

	else if (clut->size==16)
	{

			if (bitppx ==2 )
			{
			    return clut->pallete[_2_to_4_bit_map_table[code]];
			}
			else if (bitppx ==4 )
			{
				return clut->pallete[code];
			}
			else if (bitppx == 8)
			{
				return (clut->pallete[reduction_8_to_4(code)]);
			}
	}

	else if (clut->size==256)
	{
		if (bitppx ==2 )
		{
			return clut->pallete[_2_to_4_bit_map_table[code]];
		}
		else if (bitppx ==4 )
		{
			return clut->pallete[_4_to_8_bit_map_table[code]];
		}
		else if (bitppx == 8)
		{
			return clut->pallete[code];
		}
	}
	else
	{
		// reserved 
		pl.A=0;
		pl.T=0;
		pl.B=0;
		pl.C=0;
	}

	return pl;

}

_32BIT_PIXEL CDvbSub::m_256_clut(BYTE code)
{
	RGB_PALLETE result;
	YCBCR_PALETTE ycb ;
	
	BYTE b1 = (code & 0x80) >> 7;
	BYTE b2 = (code & 0x40) >> 6;
	BYTE b3 = (code & 0x20) >> 5;
	BYTE b4 = (code & 0x10) >> 4;
	BYTE b5 = (code & 0x08) >> 3;
	BYTE b6 = (code & 0x04) >> 2;
	BYTE b7 = (code & 0x02) >> 1;
	BYTE b8 = (code&0x1);

	if (b1==0 && b5==0)
	{
		if (b2==0 && b3==0 && b4==0)
		{
			if (b6==0 && b7==0 && b8==0)
				result.T=0;
			else
			{
				result.T=64;
				result.red = 255*b8;
				result.green=255*b7;
				result.blue = 255*b6;
			}
		}
		else
		{
			result.T=255;
			result.red = (85*b8) + (170*b4);
			result.green = (85*b7) + (170*b3);
			result.blue = (85*b6) + (170*b2);
		}
	}
	if (b1==0 && b5==1)
	{
		result.T=128;
		result.red = (85*b8) + (170*b4);
		result.green = (85*b7) + (170*b3);
		result.blue = (85*b6)+ (170*b2);
	}
	if (b1==1 && b5==0)
	{
		result.T = 255;
		result.red = (43*b8) + (85*b4) + 127;
		result.green = (43*b7) + (85*b3) + 127;
		result.blue = (43*b6) + (85*b2) + 127;
	}
	if (b1==1 && b5==1)
	{
		result.T = 255;
		result.red = (43*b8) + (85*b4);
		result.green = (43*b7) + (85*b3);
		result.red = (43*b6) + (85*b2);
	}

	if (m_colorspace == COLORSPACE_AYUV)
	{
	     ycb = RGBtoYCBCR(result);
		 return (_32BIT_PIXEL)ycb;
	}
	else
	{
         return (_32BIT_PIXEL)result;
	}
}

_32BIT_PIXEL CDvbSub::m_16_clut(BYTE code)
{
	RGB_PALLETE result;
	YCBCR_PALETTE ycb ;
	
	BYTE b1 = (code & 0x08) >> 3;
	BYTE b2 = (code & 0x04) >> 2;
	BYTE b3 = (code & 0x02) >> 1;
	BYTE b4 = (code&0x1);

	if (b1==0)
	{
		if (b2==0 && b3==0 && b4==0)
		{
			result.T=0;
		}
		else
		{
			result.T = 255;
			result.red = 255*b4;
			result.green=255*b3;
			result.blue = 255*b2;
		}
	}
	if (b1==1)
	{
		result.T = 255;
		result.red = 128 *b4;
		result.green =128*b3;
		result.blue =128*b2;
	}

	if (m_colorspace == COLORSPACE_AYUV)
	{
	     ycb = RGBtoYCBCR(result);
		 return (_32BIT_PIXEL)ycb;
	}
	else
	{
         return (_32BIT_PIXEL)result;
	}
}


_32BIT_PIXEL CDvbSub::m_4_clut(BYTE code)
{
	RGB_PALLETE result;
	YCBCR_PALETTE ycb ;

	BYTE b1 = (code & 0x02) >> 1;
	BYTE b2 = (code&0x1);

	if (b1==0 && b2==0)
		result.T=0;

	if (b1==0 && b2 ==1)
	{
		result.T=255;
		result.red=255;
		result.green=255;
		result.blue=255;
	}

	if (b1==1 && b2==0)
	{
		result.T=255;
		result.red=0;
		result.green=0;
		result.blue=0;
	}

	if (b1==1 && b2==1)
	{
		result.T=255;
		result.red=128;
		result.green=128;
		result.blue=128;
	}

	if (m_colorspace == COLORSPACE_AYUV)
	{
	     ycb = RGBtoYCBCR(result);
		 return (_32BIT_PIXEL)ycb;
	}
	else
	{
         return (_32BIT_PIXEL)result;
	}
}


region_pos_t* CDvbSub::get_region_position(BYTE region_id)
{
	for (int i=0; i<MAX_REGIONS; i++)
	{
		if ( m_current_page.regions[i].region_id == region_id)
			return &(m_current_page.regions[i]);
	}
	return NULL;
}

// Find free region
region_t* CDvbSub::find_free_region(BYTE region_id)
{
	region_t* result;
	bool found =false;

	for (int i=0; i< MAX_REGIONS; i++)
	{
		if (m_regions[i].used && m_regions[i].region_id == region_id)
		{
			found =true;
			result = &(m_regions[i]);
			break;
		}
	}

	if (!found)
	{
		m_next_free_region = (m_next_free_region+1)%MAX_REGIONS;
		result = &(m_regions[m_next_free_region]);
	}

	return result;
}

region_t* CDvbSub::find_region(u_short object_id)
{
	region_t* region = NULL;

	for ( int i=0; i<MAX_REGIONS; i++)
	{
		for (int j=0; j< m_regions[i].object_count; j++)
		{
			if (m_regions[i].object_metrics[j].object_id == object_id)
			{
				region = &(m_regions[i]);
				break;
			}
		}
	}

	return region;
}

// Find free CLUT
bool CDvbSub::find_free_CLUT(BYTE id, CLUT_t** clut)
{
	bool found =false;
	for (int i=0; i<MAX_CLUTS; i++)
	{
		if (m_cluts[i].used && m_cluts[i].CLUT_id == id)
		{
			found =true;
			*clut = &(m_cluts[i]);
			break;
		}
	}

	if (!found)
	{
		m_next_free_clut = (m_next_free_clut+1)%(MAX_CLUTS); // next free clut
		*clut = &(m_cluts[m_next_free_clut]);
	}

	return found;
}

CLUT_t* CDvbSub::find_CLUT(BYTE id)
{
	for (int i=0; i<MAX_CLUTS; i++)
	{
		if (m_cluts[i].used && m_cluts[i].CLUT_id == id)
		{
			return &m_cluts[i];
		}
	}
	return NULL;
}