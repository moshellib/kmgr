#ifndef __SIKLU_K60_IMAGE__
#define __SIKLU_K60_IMAGE__

#ifndef UINT32
typedef unsigned int UINT32;
#endif
#ifndef UINT8
typedef unsigned char UINT8;
#endif

/*********************************************************
* Siklu K60 image structure :
	
	CRC is calculated on all image ( from burn_address to the begining of the CRC)
**********************************************************/

typedef struct
{
	UINT32 burn_address;
	UINT32 run_address;
	UINT32 header_version;
	UINT32 data_length;
	UINT8  sw_version[64];
	UINT8  reserved[48]; // for future use. Align header size to 128 bytes.
} T_SIKLU_K60_HEADER;


#endif
