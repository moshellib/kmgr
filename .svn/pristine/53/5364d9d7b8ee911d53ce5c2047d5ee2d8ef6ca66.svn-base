/*
 * wdr85100.h
 *
 *  Created on: May 19, 2014
 *      Author: ilane
 */

#ifndef WDR85100_H_
#define WDR85100_H_

#define BCM85100_FW_MAX_SIZE_IN_BYTES        256*1024
#define BCM85100_MAX_CONFIG_FILE_LEN          28*1024

extern int wdr85100_com_init();
extern int wdr85100_load_config(unsigned char* binFileBuf, int binFileLen, unsigned char* mcFwBuf, int mcFwLen);
extern int wdr85100_config(unsigned char* binFileBuf, int binFileLen);

#endif /* WDR85100_H_ */
