#ifndef H_STRUCTS
#define H_STRUCTS

struct buffer_t {
  char* buf;
  int length;
  int index; //buffer index
};

struct bufferES_t {
  char* buf;
  int length;
  __int64 present_time;
  bool has_pts;
  bool discontinuity;
  unsigned int video_arx;
  unsigned int video_ary;
  unsigned int video_width;
  unsigned int video_height;
  unsigned char video_afd;
  unsigned short stream_type;
  bool starts_with_i_frame; 
  bool has_i_frame; 
  unsigned int video_frames;
  unsigned int video_i_frame_pos;
  __int64 stream_time;
  bool pid_format_change;
};

struct unit_t 
{
	buffer_t* buffer;
	unit_t* ref;

};

struct unitES_t 
{
	bufferES_t* buffer;
	unitES_t* ref;

};

#endif