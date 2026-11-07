#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <limits.h>

#ifdef _OCTEON
#include "modem.h"
#include "adaptive.h"
#include "event.h"
#endif


#include "gl_types.h"
#include "kdefs.h"
#include "framer.h"
#include "cmnstruct.h"

static UINT8 BrcmCurrentModulation = BRCM_MODULATION_BPSK1;
static UINT8 lastRxModulation = BRCM_MODULATION_BPSK1;
static UINT8 lastTxModulation = BRCM_MODULATION_BPSK1;

typedef struct kio_buff_s {
	int head;
	int cmt;
	int tail;
	unsigned char buff[KIO_BUFF_SIZE];
	unsigned char flags[KIO_BUFF_SIZE];
} kio_buff_t;

struct {
	int tty_fd;
	int flags;
	framer_ctx_t framer_ctx_host;
	csm_tx_t framer_ctx_tx_csms[N_H2K_SUBC];
	csm_rx_t framer_ctx_rx_csms[N_K2H_SUBC];
	kio_buff_t rx_subc[N_K2H_SUBC + 2]; // k60 sub-channels + raw + handlers
	kio_buff_t tx_subc[N_H2K_SUBC + 1]; // k60 sub-channels + raw
} kio_data;


rf_operational_cb_t host_rf_operational_cb = NULL;

void rf_operational_set_cb(rf_operational_cb_t rf_operational_cb) {
    host_rf_operational_cb = rf_operational_cb;
}


static int kio_buff_in(kio_buff_t * b, void *data, int len, int cmt, int pdu_flags) {
	int i, pos, last;
	last = b->head == 0 ? (int)sizeof(b->buff) - 1 : b->head - 1;
	for(i = 0, pos = b->tail; (i < len) && (pos != last); i++, pos = (pos == sizeof(b->buff) - 1) ? 0 : pos + 1) {
		b->buff[pos] = ((unsigned char*)data)[i];
		if(i == 0) {
			b->flags[pos] = pdu_flags & PDU_FLAG_START;
		} else if (i == len - 1) {
			b->flags[pos] = pdu_flags & (PDU_FLAG_END | PDU_FLAG_CSM_PASS);
		} else {
			b->flags[pos] = 0;
		}
	}
	b->tail = pos;
	if(cmt && (i == len)) {
		b->cmt = pos;
	}
	return i;
}

static int kio_buff_commit(kio_buff_t * b) {
	b->cmt = b->tail;
	return 0;
}

static int kio_buff_revert(kio_buff_t * b) {
	b->tail = b->cmt;
	return 0;
}

static int kio_buff_out(kio_buff_t * b, void *data, int len, int *pdu_flags) {
	int i, pos, flags = 0, go_on = 1;
	for(i = 0, pos = b->head; (i < len) && (pos != b->cmt) && go_on; i++, pos = (pos == sizeof(b->buff) - 1) ? 0 : pos + 1) {
		if(b->flags[pos] & PDU_FLAG_START) {
			if(i == 0) {
				flags |= PDU_FLAG_START;
			} else {
				break; // we want to exit loop before advancing pos
			}
		}
		((unsigned char*)data)[i] = b->buff[pos];
		if(b->flags[pos] & PDU_FLAG_END) {
			flags  |= (b->flags[pos] & (PDU_FLAG_END | PDU_FLAG_CSM_PASS));
			go_on = 0; // we want to exit loop after advancing pos
		}
	}
	b->head = pos;
	if(pdu_flags) {
		*pdu_flags = flags;
	}
	return i;
}

void kio_event_handler(unsigned char event_num) {
	if(kio_buff_in(&kio_data.rx_subc[K2H_SUBC_EVNT], &event_num, 1, 1 /* commit */, PDU_FLAG_START | PDU_FLAG_END | PDU_FLAG_CSM_PASS) < 1) {
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "dropped characters (event) since buffer is full"); // should not happen
	}
}

void kio_mod_change(UINT8 *data, UINT32 length) {
	unsigned char modc, tx_modc = 0, rx_modc;
	UINT8 modulation, MinTxRxModulation = BRCM_MODULATION_BPSK1, i, rx_received = 0, tx_received = 0;

	if(length == 0) return;

	// we only care about the last modulation change received per rx/tx
	for ( i=0 ; i<length ; i++ )
	{
	    modc = data[i];
	    if ( (modc & TX_MODC_REQ_WITH_ACK) || (modc & TX_MODC_REQ_NO_ACK) )
	    {
	        tx_modc = data[i];
	        tx_received++;
	    }
	    else
	    {
	        rx_modc = data[i];
	        rx_received++;
	    }
	}

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "kio modulation change received. length: %d", length);

    if ( tx_received>0 )
    {
        klog(KLOG_SUBJECT_KMGR, LL_INFOR, "kio tx modulation change received %d", tx_modc);
        modulation = (tx_modc & MODULATION_PROFILE_MASK); //2 msb are reserved to indicate rx/tx modc and whether an ack is needed
        lastTxModulation = modulation;
        klog(KLOG_SUBJECT_KMGR, LL_INFOR, "kio last-rx-modulation:%d tx-modulation received:%d", lastRxModulation, modulation);
        MinTxRxModulation = lastRxModulation > modulation ? lastRxModulation : modulation;
    }

    if ( rx_received>0 )
    {
        klog(KLOG_SUBJECT_KMGR, LL_INFOR, "kio rx modulation change received %d", rx_modc);
        modulation = (rx_modc & MODULATION_PROFILE_MASK); //2 msb are reserved to indicate rx/tx modc and whether an ack is needed
        lastRxModulation = modulation;
        klog(KLOG_SUBJECT_KMGR, LL_INFOR, "kio last-tx-modulation:%d rx-modulation received:%d", lastTxModulation, modulation);
        MinTxRxModulation = lastTxModulation > modulation ? lastTxModulation : modulation;
    }

#ifdef _OCTEON
    T_MODEM_MODULATION_ID   modulation_id ;
    modulation_id.s.repetition = E_MODEM_1_REPETITION;
    modulation_id.s.subchan = E_MODEM_1_SUBCHAN;
    modulation_id.s.rate = E_MODEM_1_2_RATE;

	if ( MinTxRxModulation != BrcmCurrentModulation)
	{
	    switch (MinTxRxModulation){
	    case BRCM_MODULATION_QAM64:
	        modulation_id.s.modulation = E_MODEM_QAM64_MODULATION;
	        break;
	    case BRCM_MODULATION_QAM32:
	        modulation_id.s.modulation = E_MODEM_QAM32_MODULATION;
	        break;
	    case BRCM_MODULATION_QAM16:
	        modulation_id.s.modulation = E_MODEM_QAM16_MODULATION;
	        break;
	    case BRCM_MODULATION_QPSK1:
	        modulation_id.s.modulation = E_MODEM_QPSK_MODULATION;
	        break;
	    case BRCM_MODULATION_QPSK2:
	        modulation_id.s.modulation = E_MODEM_QPSK_MODULATION;
	        modulation_id.s.rate = E_MODEM_2_3_RATE;
	        break;
	    case BRCM_MODULATION_QPSK3:
	        modulation_id.s.modulation = E_MODEM_QPSK_MODULATION;
	        modulation_id.s.rate = E_MODEM_4_5_RATE;
	        break;
	    case BRCM_MODULATION_BPSK1:
	        modulation_id.s.modulation = E_MODEM_BPSK_MODULATION;
	        break;
	    case BRCM_MODULATION_BPSK2:
	        modulation_id.s.modulation = E_MODEM_BPSK_MODULATION;
	        modulation_id.s.rate = E_MODEM_2_3_RATE;
	        break;
	    default:
	        modulation_id.s.modulation = E_MODEM_BPSK_MODULATION;
	    }

	    ModulationEvent(  modulation_id.s.modulation , modulation_id.s.subchan , modulation_id.s.repetition , modulation_id.s.rate ) ;
	    BrcmCurrentModulation = MinTxRxModulation;
	}

#endif
	if ( tx_received>0 ) // send event in case of tx modc
    {
        // next we acknowledge the change
         kio_write(H2K_SUBC_MODC, &tx_modc, sizeof(tx_modc), 1 /* commit */, 0 /* PDU flags */);
    }
}

void kio_rf_state_change(UINT8 *data, UINT32 length) {
    unsigned char rf_state;
    if(host_rf_operational_cb != NULL)
    {
        if(length == 0) return;
        rf_state = data[length - 1];
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>>>>>>>>>>>>>rf operational change to  %d", rf_state);
        if (rf_state)
            host_rf_operational_cb(E_TX_STATE_UP);
        else
            host_rf_operational_cb(E_TX_STATE_DOWN);
    }

}

void *kio_thread_main(void *arg) {
    (void)arg;
    struct termios tio;
	long baud_rate = 9600;
	char *endptr, *str;
	long BAUD;

#if 0
	pthread_t me = pthread_self();
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "kio me is 0x%llX", (unsigned long long)me);
    pthread_detach(me);
#endif

    memset(&kio_data, 0, sizeof(kio_data));
	framer_ctx_init(&kio_data.framer_ctx_host, N_H2K_SUBC /* tx-subc */, N_K2H_SUBC /* rx-subc */, 14, 27, kio_data.framer_ctx_tx_csms, kio_data.framer_ctx_rx_csms);
	framer_register_handler(&kio_data.framer_ctx_host, kio_event_handler);

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;

	kio_data.tty_fd=open(env_tty, O_RDWR | O_NONBLOCK);
	if(kio_data.tty_fd < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kio_thread_main: failed to open device=%s: %s", env_tty, strerror(errno));
		return NULL;
	}
	str = env_baud;
	baud_rate = strtol(str,&endptr,10);
	switch (baud_rate)
	{
	case 115200:
		BAUD = B115200;
		break;
	case 38400:
		BAUD = B38400;
		break;
	case 19200:
		BAUD  = B19200;
		break;
	case 9600:
	default:
		BAUD  = B9600;
		break;
	case 4800:
		BAUD  = B4800;
		break;
	case 2400:
		BAUD  = B2400;
		break;
	case 1800:
		BAUD  = B1800;
		break;
	case 1200:
		BAUD  = B1200;
		break;
	case 600:
		BAUD  = B600;
		break;
	case 300:
		BAUD  = B300;
		break;
	case 200:
		BAUD  = B200;
		break;
	case 150:
		BAUD  = B150;
		break;
	case 134:
		BAUD  = B134;
		break;
	case 110:
		BAUD  = B110;
		break;
	case 75:
		BAUD  = B75;
		break;
	case 50:
		BAUD  = B50;
		break;
	}  //end of switch baud_rate
	cfsetospeed(&tio,BAUD);            // 115200 baud
	cfsetispeed(&tio,BAUD);            // 115200 baud

	if(tcsetattr(kio_data.tty_fd,TCSANOW,&tio) < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kio_thread_main: failed to configure device=%s baud=%s: %s", env_tty, env_baud, strerror(errno));
		close(kio_data.tty_fd);
		return NULL;
	}

    while (1) {
        UINT8 rxbuff[32];
        INT32 rxlen;
    	int i, ops;
    	ops = 0;

    	if(kio_data.flags & KIO_FLAG_FLUSH_RX) {
    		kio_data.flags &= ~KIO_FLAG_FLUSH_RX;
    		while(read(kio_data.tty_fd,rxbuff,1)>0); // clear existing characters...
        	for(i=0; i<(int)(sizeof(kio_data.rx_subc)/sizeof(kio_data.rx_subc[0])); i++) {
        		kio_data.rx_subc[i].tail = kio_data.rx_subc[i].cmt = kio_data.rx_subc[i].head;
        	}
    	}
    	// handle incoming data on serial port
    	rxlen = kio_data.flags & KIO_FLAG_NO_READ ? 0 : read(kio_data.tty_fd, &rxbuff, sizeof(rxbuff));
		if(rxlen < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
			klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kio_thread_main: serial read failed for device=%s: %s", env_tty, strerror(errno));
			close(kio_data.tty_fd);
			return NULL;
		}
    	if (rxlen > 0) {
    		// new data is available on the serial port, handle it
    		++ops;
    		if(kio_data.flags & KIO_FLAG_RAW) {
    			// raw mode, we pass characters without framer conversion
    			if(kio_buff_in(&kio_data.rx_subc[K2H_SUBC_RAW], rxbuff, rxlen, 1 /* commit */, 0 /* pdu_flags */) != rxlen) {
    				klog(KLOG_SUBJECT_KMGR, LL_ERROR, "dropped characters (raw) since buffer is full");
    			}
    		} else {
    	    	UINT8 rx_pdu_flags, cbuff[sizeof(rxbuff)], rx_subc;
    	    	UINT32 consumed = 0, clen;
    	    	while((INT32)consumed != rxlen) {
    	    		UINT32 c = framer_rx_convert(&kio_data.framer_ctx_host, rxbuff + consumed, rxlen - consumed/* length_in */, &rx_pdu_flags, &rx_subc /* sub_channel */, &clen, cbuff, sizeof(cbuff));
                    consumed += c;
                    if (rx_subc == K2H_SUBC_MODC)
                    {
                        // a special case for modulation change, it is handled by kio which will eventually be running as kernel driver
                        if (clen)
                            kio_mod_change(cbuff, clen);
                    }
                    else if (rx_subc == K2H_SUBC_RF_STATE)
                    {
                        // a special case for rf state change (up/down), it is handled by kio which will eventually be running as kernel driver
                        if (clen)
                            kio_rf_state_change(cbuff, clen);
                    }
        			else
        			{
        				if(rx_subc < N_K2H_SUBC) {
            				if(rx_pdu_flags & PDU_FLAG_END) {
            					if(rx_pdu_flags & PDU_FLAG_CSM_PASS) {
            						int buff_in_res = kio_buff_in(&kio_data.rx_subc[rx_subc], cbuff, clen, 1 /* commit */, rx_pdu_flags);
            						if (buff_in_res != (int) clen)
            						    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kio_buff_in()  buff_in_res: %d. clen: %d", buff_in_res, clen );
            					} else {
       								klog(KLOG_SUBJECT_KMGR, LL_ERROR, "discarded PDU with bad checksum on sub-channel %d", rx_subc);
       								kio_buff_revert(&kio_data.rx_subc[rx_subc]);
            					}
            				} else {
            					// data arrived without pdu end
            					if(clen) {
            						kio_buff_in(&kio_data.rx_subc[rx_subc], cbuff, clen, 0 /* commit */, rx_pdu_flags);
            					}
            				}
        				} else {
        					if(clen) {
            					klog(KLOG_SUBJECT_KMGR, LL_ERROR, "dropping %d bytes on unsupported sub-channel %d", clen, rx_subc);
        					}
        				}
        			}
    	    	}
    		}
    	}
    	// handle pushing of framed data to k60s
    	for(i=0; i<(int)(sizeof(kio_data.tx_subc)/sizeof(kio_data.tx_subc[0])); i++) {
    		if(kio_data.tx_subc[i].head != kio_data.tx_subc[i].tail) {
    			UINT8 tx_buff[12]; // around 1ms of of data at 115200 bps
    			int tx_pdu_flags, len = kio_buff_out(&kio_data.tx_subc[i], tx_buff, sizeof(tx_buff), &tx_pdu_flags);
    			ops++;
    			if(i == H2K_SUBC_RAW) {
					int del;
    				del = write(kio_data.tty_fd, tx_buff, len);
                    if ( del != len )
                        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "H2K_SUBC_RAW  del: %d. len: %d", del, len );
					(void)del;
    			} else {
    				UINT8 cbuff[8*sizeof(tx_buff)];
    				UINT32 tx_len;
    				UINT32 consumed = framer_tx_convert(&kio_data.framer_ctx_host, (UINT8)tx_pdu_flags, tx_buff, len /* length_in */, i /* sub_channel */, &tx_len, cbuff, sizeof(cbuff));
    				if(tx_len) {
						int del;
    					del = write(kio_data.tty_fd, cbuff, tx_len);
    					if ( del != (int)tx_len )
    					    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "del: %d. tx_len: %d", del, tx_len );
						(void)del;
    				}
    				if((INT32)consumed != len) {
    					// should not happen, cbuff is big enough
    					klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR in tx, not all consumed");
    				}
    			}
    		}
    	}

    	if(ops == 0) {
    		usleep(1000);
    	}

    }

	close(kio_data.tty_fd);
    return NULL;
}


UINT32 kio_read(UINT32 subc, void *buf, UINT32 length) {
	UINT32 r;
	if(subc >= N_K2H_SUBC + 2 || (length && buf == NULL) || length > INT_MAX) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kio_read: invalid subchannel=%u length=%u", subc, length);
		return 0;
	}
	if((r = kio_buff_out(&kio_data.rx_subc[subc], buf, length, NULL)) > 0) {
		return r;
	}
	return 0;
}


UINT32 kio_write(UINT32 subc, void *buf, UINT32 length, UINT32 commit, UINT32 pdu_flags) {
	UINT32 written;
	if(subc >= N_H2K_SUBC + 1 || (length && buf == NULL) || length > INT_MAX) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kio_write: invalid subchannel=%u length=%u", subc, length);
		return 0;
	}
	written = kio_buff_in(&kio_data.tx_subc[subc], buf, length, (int)commit, (int)pdu_flags);
	if(written != length)
		klog(KLOG_SUBJECT_KMGR, LL_WARNG, "kio_write: buffer full; subchannel=%u written=%u requested=%u", subc, written, length);
	return written;
}


UINT32 kio_commit(UINT32 subc) {
	if(subc >= N_H2K_SUBC + 1) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kio_commit: invalid subchannel=%u", subc);
		return (UINT32)-1;
	}
	return kio_buff_commit(&kio_data.tx_subc[subc]);
}

UINT32 kio_control(UINT32 flag, UINT32 value) {
	int c;
	if(value) {
		kio_data.flags |= flag;
	} else {
		kio_data.flags &= (~flag);
	}
	if(flag == KIO_FLAG_RAW) {
		while(read(kio_data.tty_fd,&c,1)>0); // clear existing characters...
		if(value == 0) {
			framer_ctx_reset(&kio_data.framer_ctx_host);
		}
	}
	return 0;
}


