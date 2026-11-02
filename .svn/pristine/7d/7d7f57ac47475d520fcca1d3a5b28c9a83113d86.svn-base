#ifndef _RFCTL_CMN_H
#define _RFCTL_CMN_H

// sub-channels passing data for host to k60

#define N_H2K_SUBC      3
#define H2K_SUBC_MODC   0
#define H2K_SUBC_MGMT   1
#define H2K_SUBC_RELAY  2

// sub-channels passing data for k60 to host

#define N_K2H_SUBC          9
#define K2H_SUBC_MODC       0
#define K2H_SUBC_RF_STATE   1
#define K2H_SUBC_SSRFCP     2
#define K2H_SUBC_RELAY      3
#define K2H_SUBC_SSRFCP_HC  4   //K60s Health Check Sub-Channel
#define K2H_SUBC_LOG_SHRT   5
#define K2H_SUBC_LOG_BCKG   6
#define K2H_SUBC_LOG_MGMT   7
#define K2H_SUBC_LOG_VSHR   8


// 128(msb) : indicates whether it's tx(1) or rx(0) modulation change request.
// 64       : indicates whether ack is needed from cavium (only for tx in case of modulation decrease)
#define TX_MODC_REQ_WITH_ACK    0xC0
#define TX_MODC_REQ_NO_ACK      0x80
#define MODULATION_PROFILE_MASK 0x3F


#endif // _RFCTL_CMN_H
