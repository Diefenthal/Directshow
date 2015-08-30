#ifndef H_BITS
#define H_BITS

#include "proj.h"

class CBits
{
	public:

	UINT expcolomb_ui(char *buf, int* byte_pos, BYTE* bit_offset);

	int expcolomb_se(char *buf, int* byte_pos, BYTE* bit_offset);

	BYTE read_bit(char* input, int* byte_pos, BYTE* bit_pos);

	UINT read_bits(char*buf, int* byte_pos, BYTE* bit_offset, int bits);

	void move_for_bits(int* pos, BYTE* startbit, UINT bits);

	BYTE reduction_4_to_2(BYTE input);
	BYTE reduction_8_to_2(BYTE input);
	BYTE reduction_8_to_4(BYTE input);

	UINT LatmGetValue(char* buf, int* byte_pos, BYTE* bit_offset);

};

#endif