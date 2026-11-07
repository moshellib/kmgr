/**********************************************************************
* HEADER_START                                                        *
*                                                                     *
*      Name:           SERIALLINUX.C                                  *
*      Project:        Developper's HC08 Bootloader Master            *
*      Description:    Serial communication routines file             *
*      Platform:       Linux                                          *
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

#include "sysdeplinux.h"
#include "serial.h"
#include <unistd.h>
#include <termios.h>
#include "../gl_types.h"
#include "../kdefs.h"

typedef unsigned char BYTE;

///////////////////////////////////////////////////////////////
// module vars

static int port = -1;

///////////////////////////////////////////////////////////////
// open and set serial tty

typedef struct 
{
	char * str;
	speed_t spd;
} speed_item_t;

static speed_item_t spdtbl[] = {
	{ "600", B600 }, 
	{ "1200", B1200 }, 
	{ "2400", B2400 }, 
	{ "4800", B4800 }, 
	{ "9600", B9600 }, 
	{ "19200", B19200 }, 
	{ "38400", B38400 }, 
	{ "57600", B57600 }, 
	{ "115200", B115200 }, 
	{ "230400", B230400 }, 
	{ "460800", B460800 }, 
	{ NULL, 0 }, 
};

int _init_uart(char* nm, char* speed)
{
	struct termios ti;
	speed_t spd = B9600;
	char* pmod;
	if(nm == NULL) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_init_uart: missing device path");
		return -1;
	}

	if((pmod = strrchr(nm, ':')) != NULL)
	{
		char sm = 0;
		*pmod++ = 0;
		sscanf(pmod, "%c", &sm);
		serial_mode = sm;
	}

	if((port = open(nm, O_RDWR | O_NOCTTY)) < 0)
	{
		fprintf(stderr, "Can't open serial port %s %s(%d)\n", nm, strerror(errno), errno);
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_init_uart: failed to open device=%s: %s", nm, strerror(errno));
		return -1;
	}

	// speed specified ?
	if(speed)
	{
		int i;

		// translate speed string to speed identifier
		for(i=0; spdtbl[i].str; i++)
			if(!strcmp(speed, spdtbl[i].str))
				break;

		// not found in xlat table
		if(!spdtbl[i].str)
		{
			fprintf(stderr, "Invalid speed specified (%s)\n", speed);
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_init_uart: unsupported baud=%s device=%s", speed, nm);
			close(port);
			port = -1;
			return -1;
		}

		spd = spdtbl[i].spd;
	}

	tcflush(port, TCIOFLUSH);

	if (tcgetattr(port, &ti) < 0)
	{
		fprintf(stderr, "Can't get port settings. %s(%d)\n", strerror(errno), errno);
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_init_uart: failed to get settings for device=%s: %s", nm, strerror(errno));
		close(port);
		port = -1;
		return -1;
	}

	cfmakeraw(&ti);
	cfsetospeed(&ti, spd);
	cfsetispeed(&ti, spd);
	// I want to ignore break condition, but IGNBRK does not work in my case (?), 
	// so I enable SIGINT generation on break (which is then ignored since no CTTY is asigned)
//	ti.c_iflag |= IGNBRK;
	ti.c_iflag &= ~IGNBRK;
	ti.c_iflag |= BRKINT;
	// no modem status check
	ti.c_cflag |= CLOCAL;
	// no hardware flow control
	ti.c_cflag &= ~CRTSCTS;
	// return any characters available
	ti.c_cc[VMIN] = 0;
	// but timeout 500ms if no character come
	ti.c_cc[VTIME] = 5;

	if (tcsetattr(port, TCSANOW, &ti) < 0)
	{
		fprintf(stderr, "Can't set port settings. %s(%d)\n", strerror(errno), errno);
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_init_uart: failed to set settings for device=%s baud=%s: %s", nm, speed ? speed : "9600", strerror(errno));
		close(port);
		port = -1;
		return -1;
	}

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "_init_uart: ready device=%s baud=%s", nm, speed ? speed : "9600");
	return port;
}

int _close_uart()
{
	int result;
	if(port < 0) return 0;
	result = close(port);
	port = -1;
	if(result < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_close_uart: close failed: %s", strerror(errno));
		return -1;
	}
	return 0;
}

// send 10 zero bits

int _send_break10(void)
{
	struct termios ti;
	BYTE zero = 0;
	int ret;

	if (tcgetattr(port, &ti) < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_send_break10: failed to get settings: %s", strerror(errno));
		return -1;
	}

	ti.c_cflag |= PARENB;
	ti.c_cflag &= ~PARODD;
	if (tcsetattr(port, TCSANOW, &ti) < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_send_break10: failed to enable parity: %s", strerror(errno));
		return -1;
	}

	// write hex 0
	ret = wb(&zero, 1);
	if(ret > 0)
		flush_uart(FDRAIN, 0);

	// give him a chance to finish 
	// 1ms is enough, but you never know...
	usleep(10000);

	ti.c_cflag &= ~PARENB;
	if (tcsetattr(port, TCSADRAIN, &ti) < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_send_break10: failed to restore parity: %s", strerror(errno));
		return -1;
	}

	return ret;
}

// flush output and kill input

int _flush_uart(int out, int in)
{
	int o=0, i=0;

	if(out==FDISCARD) o = tcflush(port, TCOFLUSH);
	else if(out) o = tcdrain(port);
	if(o < 0) klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_flush_uart: output flush/drain failed: %s", strerror(errno));
	if(in) i = tcflush(port, TCIFLUSH);
	if(i < 0) klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_flush_uart: input flush failed: %s", strerror(errno));

	return o<0 || i<0 ? -1 : 0;
}

// write block, return -1=error, or number of bytes written 

int _wb(const void* data, unsigned len)
{
	int ret = write(port, data, len);
	if(ret < 0)
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_wb: serial write failed; requested=%u: %s", len, strerror(errno));
	else if((unsigned int)ret != len)
		klog(KLOG_SUBJECT_KMGR, LL_WARNG, "_wb: partial serial write; written=%d requested=%u", ret, len);
	
#ifdef _DEBUG
	{
		unsigned i;
		DBG("wb(");
		for(i=0; i<len; i++)
			DBG1("%x,", ((BYTE*)data)[i]);
		DBG1(") returned %d\n", ret);
	}
#endif

	return ret;
}

// read block, return -1=error, or number of bytes read

int _rb(void* dest, unsigned len)
{
	int ret, rd=0;
	
	while((ret=read(port, ((BYTE*)dest) + rd, len)) > 0)
	{
		len -= ret;
		rd += ret;

		if(len <= 0)
			break;
	}

	// sucess: return total bytes read
	if(ret >= 0)
		ret = rd;
	else
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "_rb: serial read failed; received=%d: %s", rd, strerror(errno));

#ifdef _DEBUG
	{
		int i;
		DBG1("rb returned %d", ret);
		if(ret > 0)
			DBG(" data:");
		for(i=0; i<ret; i++)
			DBG1("%x,", ((BYTE*)dest)[i]);
		DBG("\n");
	}
#endif

	return ret;
}


