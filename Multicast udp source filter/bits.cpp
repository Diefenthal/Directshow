#include "bits.h"

unsigned int CBits::expcolomb_ui(char *buf, int* byte_pos, BYTE* bit_offset)
{
	int pos=*byte_pos;
	BYTE bit = *bit_offset;
	unsigned int answer = 0;
	int leading_zero = -1;
	BYTE bits=0;
	for ( BYTE b=0x0; !b; leading_zero++)
	{
		b=read_bit(buf,&pos,&bit);
		if (bits>31)
			break;
		bits++;
	}
	
    answer = (UINT)pow((double)2,(double)leading_zero) -1 + read_bits(buf,&pos,&bit,leading_zero);
	BYTE len = 1+leading_zero*2;
	move_for_bits(byte_pos,bit_offset,len);

	return answer;
}

int CBits::expcolomb_se(char *buf, int* byte_pos, BYTE* bit_offset)
{
	unsigned int codeNum = expcolomb_ui(buf,byte_pos,bit_offset);
	int answer = (-1)*(int)pow((double)codeNum,(double)(codeNum+1))* ((codeNum+1)/2);
	return answer;
}

UINT CBits::LatmGetValue(char* buf, int* byte_pos, BYTE* bit_offset)
{
	BYTE bytesForValue = read_bits(buf,byte_pos,bit_offset,2);
	UINT value=0;
	for (UINT i=0 ; i<= bytesForValue; i++)
	{
		value*= 256;
		UINT valueTmp = read_bits(buf,byte_pos,bit_offset,8);
		value += valueTmp;
	}
	return value;
}

BYTE CBits::read_bit(char* input, int* byte_pos, BYTE* bit_pos)
{
	char temp=input[*byte_pos];
	BYTE res = (temp >> (0x7-*bit_pos)) & 0x1;
	move_for_bits(byte_pos,bit_pos,1);
	return res;
}

unsigned int CBits::read_bits(char*buf, int* byte_pos, BYTE* bit_offset, int bits)
{
	unsigned int answer=0;
	for (int b=0; b<bits; b++)
	{
		answer = (answer << 1) ^ read_bit(buf,byte_pos,bit_offset) ; // 0 XOR x 
	}
	return answer;
}

void CBits::move_for_bits(int* pos, BYTE* startbit, UINT bits)
{
	for (UINT b=0; b<bits; b++ )
	{
		*startbit+=1;
		if (*startbit>0x7)
		{
			*startbit=0;
			*pos+=1;
		}
	}
}

BYTE CBits::reduction_4_to_2(BYTE input)
{
	BYTE bi1 = (input&0xf)>>3;
	BYTE bi2 = (input&0x7)>>2;
	BYTE bi3 = (input&0x3)>>1;
	BYTE bi4 = (input&1);

	BYTE output = bi1<<1 | (bi2|bi3|bi4);
	return output;
}

BYTE CBits::reduction_8_to_2(BYTE input)
{
	BYTE bi1 = (input & 0x80) >> 7;
	BYTE bi2 = (input & 0x40) >> 6;
	BYTE bi3 = (input & 0x20) >> 5;
	BYTE bi4 = (input & 0x10) >> 4;
	//BYTE bi5 = (input & 0x08) >> 3;
	//BYTE bi6 = (input & 0x04) >> 2;
	//BYTE bi7 = (input & 0x02) >> 1;
	//BYTE bi8 = (input&0x1);

	BYTE output = bi1<<1 | (bi2|bi3|bi4);
	return output;
}

BYTE CBits::reduction_8_to_4(BYTE input)
{
	return (input>>4);
}