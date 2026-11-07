/**********************************************************************
* HEADER_START                                                        *
*                                                                     *
*      Name:           MAIN.C                                         *
*      Project:        Developper's HC08 Bootloader Master            *
*      Description:    MAIN file                                      *
*      Platform:       Linux/Win32                                    *
*      Revision:       1.0                                            *
*      Date:           Apr 8 2002                                     *
*      Author:         Michal Hanak                                   *
*      Company:        Freescale                                      *
*      Security:       General Business                               *
*                                                                     *
* =================================================================== *
* Copyright (c):       FREESCALE Inc., 2004, All rights reserved.     *
*                                                                     *
* =================================================================== *
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY              *
* EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR  *
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL FREESCALE OR             *
* ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT        *
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;        *
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)            *
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, *
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)       *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED *
* OF THE POSSIBILITY OF SUCH DAMAGE.                                  *
* =================================================================== *
*
* HEADER_END
*/


/*
	Changelog:
		30-May-2008		MC9S08(SG/SH)(16/32) family added (SDID 0x1A)
		12-Jun-2008		MC9S08(DZ/DV)(96/128) family added (SDID 0x19), BL_HCS08_LONG
		05-Jan-2009		MC9S08AC(32-60) family added (SDID 0x1D), BL_HCS08
		08-Jan-2009		MC9S08MP(12-16) family added (SDID 0x27), BL_HCS08
		29-Jul-2009		ColdFire support completely added & tested
        06-Aug-2009 	MC9S08AC(96/128) family added (SDID 0x1B), BL_HCS08_LARGE
                        MC9S08SE(4/8) family added (SDID 0x025), BL_HCS08
        17-Nov-2009 	MC9S08LL(8/16) family added (SDID 0x21), BL_HCS08
                        MC9S08(LL/LH)(36/64) family added (SDID 0x26), BL_HCS08_LONG
        23-Nov-2009     MC9S08SC4 family added (SDID 0x2B),	BL_HCS08
        24-Jul-2011     MC1323x family added (SDID 0x28),	BL_HCS08_LARGE
		15-Mar-2012		MC1231x / QE32 (SDID 0x1F) added,	BL_HCS08

*/

#include <unistd.h>
#include "sysdep.h"
#include "hc08sprg.h"
#include "serial.h"
#include "sikluK60Image.h"
#include "kdefs.h"

#define CRC32POLY0 0x04c11db7 // IEEE CRC-32 polynomial (non reflected)
#define CRC32POLY1 0x6BCC41E9 // 5^n for some n
#define CRC32POLY2 0xFFFFFFFE // ((UINT32)-1)<<1

unsigned int CRC32(UINT8 *byte_ptr, int n_bytes, unsigned int poly)
{
	unsigned int k;
	unsigned int crc;

	crc = 0xffffffff;
	while (n_bytes--)
	{
		crc ^= *byte_ptr++ << 24;
		for (k=0; k<8; k++)
			crc = (crc << 1) ^ ((crc & 0x80000000) ? poly : 0);
	}

	return(~crc);
}

unsigned short image_crc;

int SikluConvert(char * sFileName,char * outputFileName,UINT32 run_address, T_SIKLU_K60_HEADER *header_out);
int load_siklu_image(char * fileName);
int SikluGetRemoteVersion(char *version, int vsize);
// The global variables has been moved into prog.c file due to beeter compatibility with Windows version of code!

extern const char an2295Version[];

static int convert_and_load_image(char *file, T_SIKLU_K60_HEADER *header)
{
	char output_file[] = "/tmp/kmgr-image-XXXXXX";
	int output_fd;
	int result = 0;

	output_fd = mkstemp(output_file);
	if(output_fd < 0)
	{
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to create temporary firmware file: %s", strerror(errno));
		return -1;
	}
	close(output_fd);

	if(SikluConvert(file, output_file, 0x4080, header) < 0)
		result = -1;
	else if(load_siklu_image(output_file) < 0)
		result = -2;

	if(unlink(output_file) < 0)
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to remove temporary firmware '%s': %s", output_file, strerror(errno));

	return result;
}

int loader_entry(char *dev_name, char *speed, char *file, int force_burn)
{
	int i;
	int  __attribute__ ((unused)) short_trim = 0;
	T_SIKLU_K60_HEADER header;
	T_SIKLU_K60_HEADER remote_header;
	char dev_arg[256];

	image_crc = 0;
	memset(&image, 0, sizeof(image));
	snprintf(dev_arg, sizeof(dev_arg), "%s:d", dev_name);

	// Check the additional short trim option
	i = strlen(dev_arg);

	if(i)
		i--;

	if(dev_arg[i] == '*')
	{
		short_trim = 1;
		dev_arg[i] = 0;
	}

	// open serial port (speed specified ?)
	if(init_uart(dev_arg, speed) < 0)
		return 1;

	// read S19 file
	int conversion_rc = convert_and_load_image(file, &header);
	if(conversion_rc != 0) return conversion_rc;

	int mcu_info_rc;

	// read info
	mcu_info_rc = read_mcu_info();

	if(mcu_info_rc == 0)
	{
		printf("read MCU info: OK\r\n");
	}
	else
	{
		fprintf(stderr, "Can't read MCU info. Could be protocol error:   %d.\r\n", mcu_info_rc);
		fprintf(stderr, "(Or forgot to set single wire mode?)\r\n");
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Can't read MCU info. Could be protocol error:   %d.\r\n", mcu_info_rc);
		return 5;
	}
	
	// masquerade vector table
	if(setup_vect_tbl(ident.int_vect_tbl , ident.bl_tbl) < 0)
		return 6;

	// check if code goes to valid memory only - issue warning only, but continue! (r30323)
	check_image();

	if(SikluGetRemoteVersion((char *)&remote_header.sw_version, sizeof(remote_header.sw_version)) != 0) {
		sprintf((char*)remote_header.sw_version, "unreadable");
	}

	if((force_burn == 0) && (strcmp((char*)header.sw_version, (char*)remote_header.sw_version) == 0)) {
		printf("versions are the same, will use existing version...\r\n");
		printf("v: l=%s r=%s\r\n", header.sw_version, remote_header.sw_version);
	} else {
		printf("versions are different or force-burn, will burn new version in target...\r\n");
		printf("v: l=%s r=%s\r\n", header.sw_version, remote_header.sw_version);
		// program it!
		if(prg_mem() < 0)
		{
			fprintf(stderr, "Program memory failed.\r\n");
			return 8;
		}
		// exit program mode, start application
		unhook();
	}

	close_uart();
	//getchar();
	return 0;
}

int orig_main(int argc, char *argv[ ])
{
	int i;
	int  __attribute__ ((unused)) short_trim = 0;
	T_SIKLU_K60_HEADER header;
	T_SIKLU_K60_HEADER remote_header;

	image_crc = 0;
	memset(&image, 0, sizeof(image));

#if 0
	if (argc == 6 && strcmp(argv[1],"convert")==0)
	{
		// siklu convert.
		// usage : hc08sprg s3_file output_file run_address version_info
		UINT32 addr;
		sscanf(argv[4],"%x",&addr);
		if(SikluConvert(argv[2],argv[3],addr,argv[5]) < 0) return -1;
		return 0;
	}
#endif

#if 0
	printf("------------------------------------------------------------------------------\r\n");
	printf("hc08sprg - Developer's HC/S08/CFV1/V2/Kinetis Serial Bootloader\r\n");
	printf("%s\r\n", an2295Version);
	printf("------------------------------------------------------------------------------\r\n");
	printf("FC protocol versions supported:\r\n"
		"\t0x%02X (%s)\r\n"
		"\t0x%02X (%s)\r\n"
		"\t0x%02X (%s)\r\n"
		"\t0x%02X (%s)\r\n" 
		"\t0x%02X (%s)\r\n"
		"\t0x%02X (%s)\r\n"
		"\t0x%02X (%s)\r\n",
		BL_HC08,			Version_string(BL_HC08),
		BL_HC08_LARGE,		Version_string(BL_HC08_LARGE),
		BL_HCS08,			Version_string(BL_HCS08),
		BL_HCS08_LONG,		Version_string(BL_HCS08_LONG),
		BL_HCS08_LARGE,		Version_string(BL_HCS08_LARGE),
		BL_CFV1,			Version_string(BL_CFV1),
		BL_KINETIS,			Version_string(BL_KINETIS));
	printf("------------------------------------------------------------------------------\r\n\r\n");
#endif

	if(argc != 3 && argc != 4)
	{
		

		printf(
			"usage:  hc08sprg port[:|!][D|d|S|s|?][*] [speed] file\r\n"
			"        port:D ... dual wire mode [default]\r\n"
			"        port:d ... dual wire mode with verification supressed\r\n"
			"        port:S ... single wire mode\r\n"
			"        port:s ... single wire mode with verification supressed\r\n"
			"        port:? ... detect single/dual wire mode (use with caution)\r\n"
			"            !  ... batch mode, no questions\r\n"
			"            *  ... short trim\r\n"
			"        speed  ... speed in bps\r\n"
			"        file   ... S19 file\r\n\r\n");

		if ((argc == 2) && ((argv[1][0] == '?') || ((argv[1][1] == '?'))))
		{
			printf("Recognized S08/CFV1/Kinetis MCU families:\r\n\r\n");
			printf("SDID\tproto\tfamily\r\n");

			i=0;
			do
			{
				printf("0x%02X\t0x%02X\t%s\r\n",sdidprops[i].sdid,sdidprops[i].proto,sdidprops[i].name);
			} while(sdidprops[++i].sdid != SDID_UNDEF);

			printf("\r\n");
		}

		printf("See Freescale Application Note AN2295 and AN2295SW for updates.\r\n");

		return 0;
	}

	// Check the additional short trim option
	i = strlen(argv[1]);

	if(i)
		i--;

	if(argv[1][i] == '*')
	{
		short_trim = 1;
		argv[1][i] = 0;
	}

	// open serial port (speed specified ?)
	if(init_uart(argv[1], argc==4 ? argv[2] : NULL) < 0)
		return 1;

	// read S19 file
	int conversion_rc = convert_and_load_image(argv[argc-1], &header);
	if(conversion_rc != 0) return conversion_rc;
#if 0
	if(read_s19(argv[argc-1], &image) < 0)
		exit(2);

	// wait for reset
	if(hook_reset() < 0) 
		exit(3);

	// calibrate target's oscilator
	if(calibrate_speed(short_trim) < 0)
		exit(4);
#endif
	// read info
	if(!read_mcu_info())
	{
#if 0
		printf("\r\nBootloader protocol version: 0x%02X (%s, ", bl_version /*| (bl_rcs ? 0x80 : 0)*/, Version_string(bl_version));
		printf("read command ");
		if (!bl_rcs) printf("NOT ");
		printf("supported,\r\n");
		printf("Protocol secured:");
		if (!bl_crcs) 
			printf("none)\r\n) ");		
		else
			printf(" CRC-CCITT)\r\n");
		printf("Bootloader version string: %s\r\n", ident.targ_name);

		if (bl_version == BL_HC08) // HC08
		{
			unsigned i;
			printf("Available flash memory: 0x%04X-0x%04X\r\n", ident.mem_start[0], ident.mem_end[0]-1);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
			printf("Original vector table: 0x%04X\r\n",ident.int_vect_tbl);
			printf("Bootloader user table: 0x%04X\r\n",ident.bl_tbl);
			printf("Bootloader data (hex): ");
			for(i=0; i<sizeof(ident.priv_data); i++)
				printf("%02x ", ident.priv_data[i]);
			printf("\r\n");
		}
		else if (bl_version == BL_HCS08) // S08
		{
			unsigned i;
			printf("System device ID: 0x%03X [%s] rev. %u\r\n", ident.sdid, chipid ,ident.sdidrev);
			printf("Number of memory blocks: %u\r\n", ident.num_blocks);
			for(i=0; i<ident.num_blocks; i++)
				printf("Memory block #%u: 0x%04X-0x%04X\r\n", i+1, ident.mem_start[i], ident.mem_end[i]-1);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
			printf("Original vector table: 0x%04X-0xFFFF\r\n",ident.int_vect_tbl);
			printf("New vector table:      0x%04X-0x%04X\r\n",ident.bl_tbl, ident.bl_tbl+(0xFFFF-ident.int_vect_tbl));
		}
		else if (bl_version == BL_HC08_LARGE)	// large HC08
		{
			unsigned i;
			printf("Number of memory blocks: %u\r\n", ident.num_blocks);
			for(i=0; i<ident.num_blocks; i++)
				printf("Memory block #%u: 0x%04X-0x%04X\r\n", i+1, ident.mem_start[i], ident.mem_end[i]-1);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
			printf("Original vector table: 0x%04X\r\n",ident.int_vect_tbl);
			printf("Bootloader user table: 0x%04X\r\n",ident.bl_tbl);
		}
		else if ((bl_version == BL_HCS08_LARGE) || (bl_version == BL_HCS08_LONG))// large + long S08
		{
			unsigned i;
			printf("System device ID: 0x%03X [%s] rev. %u\r\n", ident.sdid, chipid ,ident.sdidrev);
			printf("Number of memory blocks: %u\r\n", ident.num_blocks);
			for(i=0; i<ident.num_blocks; i++)
				printf("Memory block #%u: 0x%06X-0x%06X\r\n", i+1, ident.mem_start[i], ident.mem_end[i]-1);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
			printf("Original vector table: 0x%04X\r\n",ident.int_vect_tbl);
			printf("Bootloader user table: 0x%04X\r\n",ident.bl_tbl);
		}
		else if (bl_version == BL_CFV1) // CFV1
		{
			unsigned i;
			printf("System device ID: 0x%03X [%s] rev. %u\r\n", ident.sdid, chipid ,ident.sdidrev);
			printf("Number of memory blocks: %u\r\n", ident.num_blocks);
			for(i=0; i<ident.num_blocks; i++)
				printf("Memory block #%u: 0x%08X-0x%08X\r\n", i+1, ident.mem_start[i], ident.mem_end[i]);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
            if (ident.int_vect_tbl != 0) 
            {
    			printf("Original vector table: 0x%08X\r\n-0x%08X\r\n",0, ident.int_vect_tbl-1);
    			printf("New vector table: 0x%08X\r\n-0x%08X\r\n",ident.bl_tbl,ident.bl_tbl+ident.int_vect_tbl-1);
            } else
    			printf("Vector table not relocated\r\n");
		}
		else if (bl_version == BL_KINETIS) // KINETIS
		{
			unsigned i;
			printf("System device ID: 0x%03X [%s] rev. %u\r\n", ident.sdid, chipid ,ident.sdidrev);
			printf("Kinetis Package: %s .\r\n", get_package_str(ident.sdid));
			printf("Number of memory blocks: %u\r\n", ident.num_blocks);
			for(i=0; i<ident.num_blocks; i++)
				printf("Memory block #%u: 0x%08X-0x%08X\r\n", i+1, ident.mem_start[i], ident.mem_end[i]);
			printf("Erase block size: %3d bytes\r\n", ident.erblk);
			printf("Write block size: %3d bytes\r\n", ident.wrblk);
        	printf("Original vector table: 0x%08X-0x%08X\r\n",ident.int_vect_tbl,ident.int_vect_tbl + 0x3FF);
			printf("New vector table: 0x%08X-0x%08X\r\n",ident.bl_tbl, ident.bl_tbl + 0x3FF);
		}
		else
		{
			fprintf(stderr, "This version [0x%02X] of protocol is not (yet) supported! Check AN2295SW for update!\r\n", bl_version);
			return 9;
		}
#endif
	}
	else 
	{
		fprintf(stderr, "Can't read MCU info. Could be protocol error.\r\n");
		fprintf(stderr, "(Or forgot to set single wire mode?)\r\n");
		return 5;
	}
	
	// masquerade vector table
	if(setup_vect_tbl(ident.int_vect_tbl , ident.bl_tbl) < 0)
		return 6;

	// check if code goes to valid memory only - issue warning only, but continue! (r30323)
	check_image();

        if(SikluGetRemoteVersion((char *)&remote_header.sw_version, sizeof(remote_header.sw_version)) != 0) {
		sprintf((char*)remote_header.sw_version, "unreadlable");
	}

	printf("\r\n");
	printf("local file version is:  %s\r\n", header.sw_version);
	printf("remote file version is: %s\r\n", remote_header.sw_version);

	if(strcmp((char*)header.sw_version, (char*)remote_header.sw_version) == 0) {
		printf("versions are the same, will use existing version...\r\n");
	} else {
		printf("versions are different, will burn new version in target...\r\n");
		// program it!
		if(prg_mem() < 0)
		{
			fprintf(stderr, "Program memory failed.\r\n");
			return 8;
		}
		// exit program mode, start application
		unhook();
	}

	close_uart();
	getchar();
	return 0;
}

void SikluWriteData(FILE * file,void * buffer,UINT32 length)
{
	unsigned char * data = (unsigned char *)buffer;
	CRC_AddByteArray(&image_crc,data,length);
	fwrite(data,1,length,file);
}

int SikluConvert(char * sFileName,char * outputFileName,UINT32 run_address, T_SIKLU_K60_HEADER *header_out)
{
	T_SIKLU_K60_HEADER header;
	UINT32 start_addr,end_addr,length, crc32[3];
	FILE * file = NULL;
	memset(&header,0,sizeof(header));
	// fill bl info.
	bl_version = BL_KINETIS;

	// read s3 file
	if(read_s19(sFileName, &image) < 0)
	{
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to read firmware '%s'; check kfs path, permissions and S-record format", sFileName);
		return -1;
	}
	// masquerade vector table
	if(setup_vect_tbl(0x00004080 , 0x0) < 0)
	{
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to prepare vector table for firmware '%s'", sFileName);
		return -1;
	}

	// find first and last valid byte
	for(start_addr=0; start_addr<MAX_ADDRESS; start_addr++)
		if(image.f[start_addr]) break;
	for(end_addr=MAX_ADDRESS; end_addr>start_addr; end_addr--)
		if(image.f[end_addr-1]) break;
	length = end_addr-start_addr;
	crc32[0] = CRC32(&image.d[start_addr], length, CRC32POLY0);
	crc32[1] = CRC32(&image.d[start_addr], length, CRC32POLY1);
	crc32[2] = CRC32(&image.d[start_addr], length, CRC32POLY2);
	//printf("image starts at 0x%X ends at 0x%X length is %d\r\n",start_addr,end_addr,length);
	header.header_version = FE32(1);
	header.burn_address = FE32(start_addr - sizeof(header));
	header.data_length = FE32(length);
	header.run_address = FE32(run_address);
	snprintf((char*)header.sw_version, sizeof(header.sw_version), "i1 %.5X %.8X %.8X %.8X", length, crc32[0], crc32[1], crc32[2]); // type i1, length + 3 different CRCs
	if(header_out) { // note that header out is with local endian while header is with target endian
		memset(header_out, 0, sizeof(*header_out));
		header_out->header_version = 1;
		header_out->burn_address = start_addr - sizeof(header);
		header_out->data_length = length;
		header_out->run_address = run_address;
		snprintf((char*)header_out->sw_version, sizeof(header_out->sw_version), "i1 %.5X %.8X %.8X %.8X", length, crc32[0], crc32[1], crc32[2]); // type i1, length + 3 different CRCs
	}
	
	file = fopen(outputFileName,"wb");
	if (!file)
	{
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to open converted firmware '%s' for writing: %s", outputFileName, strerror(errno));
		 return -1;
	}
	CRC_ResetCRC(&image_crc,0xFFFF);
	// write the header
	SikluWriteData(file,&header,sizeof(header));
	// write the image
	SikluWriteData(file,&image.d[start_addr],length);
	// write the crc
	//printf("CRC is 0x%x\r\n",image_crc);
	image_crc = FE16(image_crc);
	fwrite(&image_crc,1,sizeof(image_crc),file);
	image_crc = FE16(0); // make sure we are aligned to 4 bytes.
	fwrite(&image_crc,1,sizeof(image_crc),file);
	fclose(file);
	//printf("Done\r\n");
	return 0;
}

#define IMAGE_MAX_SIZE (512*1024)
unsigned char siklu_prog_buffer[IMAGE_MAX_SIZE];

int load_siklu_image(char * fileName)
{
	UINT32 size,address;
	T_SIKLU_K60_HEADER * header;

	FILE * file = fopen(fileName,"rb");
	if (!file)
	{
		printf("Can't open input file '%s' for reading\r\n", fileName);
		return -1;
	}
	size = fread(siklu_prog_buffer,1,IMAGE_MAX_SIZE,file);
	fclose(file);

	header = (T_SIKLU_K60_HEADER *)&siklu_prog_buffer;
	
	address = FE32(header->burn_address);
	//fprintf(stderr, "Burn address = 0x%X\r\n",address);

	memcpy(&image.d[address],siklu_prog_buffer,size);
	memset(&image.f[address],1,size);
	return 0;
}


int SikluGetRemoteVersion(char *version, int vsize) {
	char *p; 
        wb("i", 1);
	for(p = version; p < version + vsize; p++) {
		rb(p, 1);
		//fprintf(stderr, "got %d (%c)\r\n", *p, *p);
		if(p[0] == 0x0d) p[0] = 0;
		if(p[0] == 0x0a) {
			p[0] = 0;
			return 0;
		}
	}
	return 1;
}
