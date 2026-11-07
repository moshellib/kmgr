
/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//    DESCRIPTION:                                                             //
//                                                                             //
//    AUTHOR:                                                                  //
//                                                                             //
//    HISTORY:                                                                 //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

// include files
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "hostProtocolStructs.h"
#include "hostCommLayer.h"
#include "hostDriver.h"
#include "defs85100.h"
#include "diagnostics.h"

//pthread_mutex_t brcm_mutex = PTHREAD_MUTEX_INITIALIZER;

int hdrv_read(void *buff, int len) {
	extern int relay_read(void *buff, int len);
	int result;
	if(len < 0 || (len && buff == NULL)) return KLOG_RETURN_ERROR(-1);
	result = relay_read(buff, len);
	if(result < 0) return KLOG_RETURN_ERROR(result);
	return result;
}

int hdrv_write(void *buff, int len) {
	extern int relay_write(void *buff, int len);
	int result;
	if(len < 0 || (len && buff == NULL)) return KLOG_RETURN_ERROR(-1);
	result = relay_write(buff, len);
	if(result < 0) return KLOG_RETURN_ERROR(result);
	if(result != len) klog(KLOG_SUBJECT_KMGR, LL_WARNG, "hdrv_write: incomplete relay write; written=%d expected=%d", result, len);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostOpen                                                   //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: all the things we want to do when we open the application     //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100HostOpen(void)
{
	// nothing to open
	
    return(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostClose                                                  //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: all the things we want to do when we close the application    //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100HostClose(void)
{
    return(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostDeviceReset                                          //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION:   host need to implement based on his Hw					   //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////

void bcm85100HostDeviceReset(BYTE deviceIndex)
{  
	(void)deviceIndex;
	// we need to ask K60s to reset K60b, if at all.....
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSleep                                                  //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION:                                                               //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostSleep(UINT32 sleepTimeMicro)
{
	usleep(sleepTimeMicro);
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostTickGet                                                //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION:                                                               //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
UINT32 bcm85100HostTickGet(void)
{
	struct timespec ts;
	if(clock_gettime(CLOCK_REALTIME, &ts) < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostTickGet: clock read failed: %s", strerror(errno));
		return 0;
	}
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostApiMutexGet                                            //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION:                                                               //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostApiMutexGet(BYTE deviceIndex)
{
	(void)deviceIndex;
	//pthread_mutex_lock(&brcm_mutex);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostApiMutexRelease                                      //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION:                                                               //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostApiMutexRelease(BYTE deviceIndex)
{
	(void)deviceIndex;
	//pthread_mutex_unlock(&brcm_mutex);
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostUartDataSend                                           //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: writes a buffer to the uart                                   //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostUartDataSend(BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen)
{
	if(buffLen && pBuff == NULL) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostUartDataSend: missing buffer; device=%u bytes=%lu", deviceIndex, (unsigned long)buffLen);
		return;
	}
	while(buffLen) {
		UINT32 len = buffLen <= 100 ? buffLen : 100;
		int written = hdrv_write(pBuff, len);
		if(written != (int)len) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostUartDataSend: relay send failed; device=%u written=%d expected=%lu", deviceIndex, written, (unsigned long)len);
			return;
		}
		buffLen -= len;
		pBuff += len;
		if(buffLen) {
			usleep(10000);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostUartDataReceive                                        //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: reads a buffer from the uart                                  //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostUartDataReceive(BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen, UINT16 timeoutMilli, UINT32 *pReadBytes)
{
	(void)deviceIndex;
	UINT32 t, total = 0;
	if(pReadBytes) *pReadBytes = 0;
	if(buffLen && pBuff == NULL) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostUartDataReceive: missing buffer; device=%u bytes=%lu", deviceIndex, (unsigned long)buffLen);
		return;
	}
	UINT32 start_time = bcm85100HostTickGet();
	while((total < buffLen) && (bcm85100HostTickGet() - start_time <= (UINT32) timeoutMilli)) {
		int r = hdrv_read(&pBuff[total], buffLen - total);
		if(r < 0) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostUartDataReceive: relay read failed; device=%u received=%lu", deviceIndex, (unsigned long)total);
			break;
		}
		if(r > 0) {
			t = r;
			total += t;
		} else {
			t = 0;
		}
		if(t == 0) {
			usleep(100);
		}
	}
	if(pReadBytes) *pReadBytes = total;
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostUartReceiverFlush                                      //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: flushing the uart rx                                          //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostUartReceiverFlush(BYTE deviceIndex)
{
	(void)deviceIndex;
	char buf[256];
	int result;
	while((result = hdrv_read(&buf, sizeof(buf))) > 0);
	if(result < 0) klog(KLOG_SUBJECT_KMGR, LL_ERROR, "bcm85100HostUartReceiverFlush: relay flush failed; device=%u", deviceIndex);
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiDataSend                                            //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: writes a buffer to the spi                                    //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostSpiDataSend(BYTE deviceIndex, BYTE *pTxBuff, UINT32 buffLen)
{
	(void) deviceIndex;
	(void) pTxBuff;
	(void) buffLen;
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiDataReceive                                         //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: reads a buffer from the spi                                   //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostSpiDataReceive(BYTE deviceIndex, BYTE *pTxBuff, BYTE *pRxBuff, UINT32 buffLen)
{
	(void) deviceIndex;
	(void) pTxBuff;
	(void) pRxBuff;
	(void) buffLen;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiModeSet                                           //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: sets SPI mode (four possible modes: 00,01,10,11)              //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostSpiModeSet(BYTE deviceIndex, INT32 mode)
{
	(void) deviceIndex;
	(void) mode;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiMaxRateSet                                        //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: limits future SPI transactions to rate <= maxRateHz           //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostSpiMaxRateSet(BYTE deviceIndex, UINT32 maxRateHz)
{
	(void) deviceIndex;
	(void) maxRateHz;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiIsDataReady                                       //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: Return current value of SPI data ready signal                 //
//                                                                             //
//  RETURNS: TRUE if "1", FALSE if "0"                                         //
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100HostSpiIsDataReady(BYTE deviceIndex)
{
	(void) deviceIndex;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostSpiDataReadyWait                                     //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: Wait for SPI data ready to become "1" for this device/if      //
//                                                                             //
//  RETURNS: TRUE if reached timeout, FALSE if received interrupt              //
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100HostSpiDataReadyWait(BYTE deviceIndex, UINT32 timeoutMilli)
{
	(void) deviceIndex;
	(void) timeoutMilli;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostMIIDataWrite                                         //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: writes a buffer to the MII                                    //
//                                                                             //
//  RETURNS:                                                                   //
/////////////////////////////////////////////////////////////////////////////////
void bcm85100HostMIIDataWrite(BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen)
{
	(void) deviceIndex;
	(void) pBuff;
	(void) buffLen;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostMIIDataRead                                          //
//                                                                             //
//  PARAMETERS:                                                                //
//                                                                             //
//  DESCRIPTION: reads a buffer from the MII                                   //
//                                                                             //
//  RETURNS: TRUE if reached timeout, FALSE if received interrupt              //
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100HostMIIDataRead(BYTE deviceIndex, BYTE *pBuff, UINT32 *pBuffLen, UINT32 timeoutMilli)
{
	(void) deviceIndex;
	(void) pBuff;
	(void) pBuffLen;
	(void) timeoutMilli;
    return TRUE;
}


