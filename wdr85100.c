/*
 * wdr85100.c - wrapper for broadcom's 85100 driver, needed because of different definition of INT32 and UINT32
 *
 *  Created on: May 19, 2014
 *      Author: ilane
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "brcm/bcm85100_api.h"
#include "brcm/hostDriver.h"

#include "kdefs.h"

#define BCM85100_DEVICE_INDEX						0
#define BCM85100_MODEM_INDEX						0
#define BCM85100_LONG_TIMEOUT_IN_MILLISECONDS       1000
#define BCM85100_CONFIGURATIOM_DELAY_IN_MICRO		10000

int wdr85100_com_init()
{
	BCM85100_ERROR_MSG_ENUM		retVal;
	/******* Setup Communication Layer *******/

	// calls the BCM85100 host open function
	retVal = BCM85100_API_Open();
	if(retVal != BCM85100_SUCCESS_MSG_E) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_Open, retVal = %d", retVal);
	} else {
		// configures the communication media between host CPU to MC CPU to UART interface
		retVal = BCM85100_API_ComConfigure(BCM85100_MODEM_INDEX, BCM85100_COM_MEDIA_NULL_E, BCM85100_COM_MEDIA_UART_E, 0);
		if(retVal != BCM85100_SUCCESS_MSG_E) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_ComConfigure, retVal = %d", retVal);
		} else {
			// configures the host layer API calls time-out
			retVal = BCM85100_API_ComTimeoutSet(BCM85100_MODEM_INDEX, BCM85100_LONG_TIMEOUT_IN_MILLISECONDS);
			if(retVal != BCM85100_SUCCESS_MSG_E) {
				klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_ComTimeoutSet, retVal = %d", retVal);
			}
		}
	}
    if(retVal == BCM85100_SUCCESS_MSG_E)
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "wdr85100_com_init: communication initialized");
	return retVal;
}

int wdr85100_config(unsigned char* binFileBuf, int binFileLen)
{
	BCM85100_ERROR_MSG_ENUM		retVal;

	// download to the MC the off-line configuration from pre-saved .bin file
	retVal = BCM85100_API_ConfigFileFromHostLoad(BCM85100_DEVICE_INDEX, BCM85100_MODEM_INDEX, binFileBuf, binFileLen);
	if(retVal != BCM85100_SUCCESS_MSG_E)
	{
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_ConfigFileFromHostLoad first time, retVal = %d. try again..", retVal);
		retVal = BCM85100_API_ConfigFileFromHostLoad(BCM85100_DEVICE_INDEX, BCM85100_MODEM_INDEX, binFileBuf, binFileLen);
		if(retVal != BCM85100_SUCCESS_MSG_E)
		{
		    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_ConfigFileFromHostLoad second time, retVal = %d. try again..", retVal);
		    retVal = BCM85100_API_ConfigFileFromHostLoad(BCM85100_DEVICE_INDEX, BCM85100_MODEM_INDEX, binFileBuf, binFileLen);
		}
	}


	if(retVal == BCM85100_SUCCESS_MSG_E)
	{
	    klog(KLOG_SUBJECT_KMGR, LL_NTICE, "wdr85100_config succedded, retVal = %d", retVal);

		// the user need to implement a sleep function on his system
		bcm85100HostSleep(BCM85100_CONFIGURATIOM_DELAY_IN_MICRO); // wait until configuration is finished

		// dump modem's RX power
		// this is a sample code of running some APIs before passing driver to k60s...
#if 0
		BCM85100_POWER_MANAG_STRUCT   pPowerM;
		retVal = BCM85100_API_RxPowerGet(BCM85100_DEVICE_INDEX, BCM85100_MODEM_INDEX, &pPowerM);
		if(retVal != BCM85100_SUCCESS_MSG_E) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "BCM85100_API_RxPowerGet failed with %d", retVal);
		} else {
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "BCM85100_API_RxPowerGet:");
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "inPower      = %d", pPowerM.inPower);
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "inbandPower  = %d", pPowerM.inbandPower);
		}
#endif
	}
    if(retVal != BCM85100_SUCCESS_MSG_E)
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wdr85100_config: all attempts failed; rc=%d config_bytes=%d", retVal, binFileLen);
	return retVal;
}

int wdr85100_load_config(BYTE* binFileBuf, int binFileLen, BYTE* mcFwBuf, int mcFwLen)
{
	// Normal operation parameters
	BCM85100_MODEM_VERSION_STRUCT ver;

	// General parameters
	BCM85100_ERROR_MSG_ENUM		retVal;


	/******* Booting Phase *******/

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "Booting k60b...");

	// initiates the MC-CPU boot from the host - this API also calling host's reset function "bcm85100HostDeviceReset" that the customer need to implement
	retVal = BCM85100_API_FirmwareFromHostBoot(BCM85100_DEVICE_INDEX, mcFwBuf, mcFwLen, BCM85100_DEVICE_RESET_ALL_E);
	if(retVal != BCM85100_SUCCESS_MSG_E) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "BCM85100_API_FirmwareFromHostBoot failed for the first time, retVal = %d. try again..", retVal);
        sleep(1);
        retVal = BCM85100_API_FirmwareFromHostBoot(BCM85100_DEVICE_INDEX, mcFwBuf, mcFwLen, BCM85100_DEVICE_RESET_ALL_E);
        if(retVal != BCM85100_SUCCESS_MSG_E)
        {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "BCM85100_API_FirmwareFromHostBoot failed for the second time, retVal = %d. try again..", retVal);
            sleep(1);
            retVal = BCM85100_API_FirmwareFromHostBoot(BCM85100_DEVICE_INDEX, mcFwBuf, mcFwLen, BCM85100_DEVICE_RESET_ALL_E);
            if(retVal != BCM85100_SUCCESS_MSG_E)
            {
                goto FINISH;
            }
        }
	}
	// Get firmware version to verify the connection with firmware
	retVal = BCM85100_API_VersionGet(BCM85100_DEVICE_INDEX, &ver);
	if(retVal != BCM85100_SUCCESS_MSG_E) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_VersionGet, retVal = %d", retVal);
		goto FINISH;
	}

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "BCM85100_API_VersionGet: %d.%d.%d", ver.majorVersion, ver.minorVersion, ver.buildVersion);

	/******* Configuration Phase *******/

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "Configuring k60b...");
	retVal = wdr85100_config(binFileBuf, binFileLen);
	// download to the MC the off-line configuration from pre-saved .bin file
    if(retVal != BCM85100_SUCCESS_MSG_E) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wdr85100_config failed for the first time, retVal = %d. try again..", retVal);
        sleep(1);
        retVal = wdr85100_config(binFileBuf, binFileLen);
        if(retVal != BCM85100_SUCCESS_MSG_E)
        {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wdr85100_config failed for the second time, retVal = %d. try again..", retVal);
            sleep(1);
            retVal = wdr85100_config(binFileBuf, binFileLen);
            if(retVal != BCM85100_SUCCESS_MSG_E)
            {
                goto FINISH;
            }
        }
	}

	// the user need to implement a sleep function on his system
	bcm85100HostSleep(BCM85100_CONFIGURATIOM_DELAY_IN_MICRO); // wait until configuration is finished



	/******* Closing *******/

FINISH:
	klog(KLOG_SUBJECT_KMGR, retVal == BCM85100_SUCCESS_MSG_E ? LL_DEBUG : LL_ERROR,
		 "wdr85100_load_config: completed rc=%d firmware_bytes=%d config_bytes=%d", retVal, mcFwLen, binFileLen);

#if 0 // we do not want to close as we may have to reconfigure
	// calls to the BCM85100 host close function
	if(BCM85100_API_Close() != BCM85100_SUCCESS_MSG_E) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to BCM85100_API_Close");
	}
#endif

	return retVal;
}
