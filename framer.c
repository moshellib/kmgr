#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gl_types.h"
#include "framer.h"
#include "kdefs.h"

static void init_csm_tx(csm_tx_t *csm_tx) {
    memset(csm_tx, 0, sizeof(*csm_tx));
}

static void update_csm_tx(csm_tx_t *csm_tx, UINT8 byte) {
    csm_tx->csm = ((csm_tx->csm << 5) | (csm_tx->csm >> 11)) ^ byte;
}

static void init_csm_rx(csm_rx_t *csm_rx) {
    memset(csm_rx, 0, sizeof(*csm_rx));
}

static void update_csm_rx(csm_rx_t *csm_rx, UINT8 byte) {
    csm_rx->csm = ((csm_rx->csm << 5) | (csm_rx->csm >> 11)) ^ byte;
}

static UINT32 check_csm_rx(csm_rx_t *csm_rx, UINT8 byte) {
    csm_rx->last_bytes = (csm_rx->last_bytes << 8) | byte;
    return csm_rx->csm == csm_rx->last_bytes;
}

#define FRAMER_STATE_PS0   0  // waiting for PDU start
#define FRAMER_STATE_PS1   1  // waiting for PDU start, last char is esc
#define FRAMER_STATE_CHR   2  // known sub-channel
#define FRAMER_STATE_ESC   3  // known sub-channel, last char is esc
#define FRAMER_STATE_CS0   4  // known sub-channel, waiting for 1'st csm byte
#define FRAMER_STATE_CS1   5  // known sub-channel, waiting for 2'nd csm byte


void framer_ctx_init(framer_ctx_t *ctx, UINT8 tx_sub_channels, UINT8 rx_sub_channels, UINT16 segment_max_size, UINT8 escape_char, csm_tx_t *csms_tx, csm_rx_t *csms_rx) {
	memset(ctx, 0, sizeof(ctx));
	ctx->rx_state = FRAMER_STATE_PS0;
 	ctx->tx_sub_channels = tx_sub_channels;
	ctx->rx_sub_channels = rx_sub_channels;
 	ctx->segment_max_size = segment_max_size;
	ctx->escape_char = escape_char;
	ctx->last_tx_sub_channel = ctx->last_rx_sub_channel = 255; // some invalid value
	ctx->csms_tx = csms_tx;
	ctx->csms_rx = csms_rx;
}

void framer_ctx_reset(framer_ctx_t *ctx) {
	ctx->rx_state = FRAMER_STATE_PS0;
}

UINT32 framer_rx_convert(framer_ctx_t *ctx, UINT8 *data_in, UINT32 length_in, UINT8 *pdu_flags, UINT8 *sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out) {
    // rx convert must consume as much as it can as long as:
    // 1. it does not exceed length out limitation
    // 2. all bytes it provides belong to same sub-channel
    // 3. in case of a start-pdu, it is located only in the beginning of the data it provides
    // 4. in case of an end-pdu, it is located only in the end of the data it provides
    // the caller must not used data before it accepts an end pdu...
	UINT32 pos_in = 0, pos_out = 0, out = 0;
	UINT8 byte, pf = 0;
	if(ctx == NULL || (length_in && data_in == NULL) || (max_length_out && data_out == NULL)) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "framer_rx_convert: invalid buffers");
		if(len_out) *len_out = 0;
		if(pdu_flags) *pdu_flags = 0;
		return 0;
	}
	while((pos_in < length_in) && (out == 0)) {
		byte = data_in[pos_in++];
		switch(ctx->rx_state) {
		case FRAMER_STATE_PS0: {
			if(byte == ctx->escape_char) {
				ctx->rx_state = FRAMER_STATE_PS1;
			}
			break;
        }
		case FRAMER_STATE_PS1: {
            if(ESC_2_SC(byte) < MAX_SUBCHANNELS) { // a per sub-channel character
			    if((ESC_2_SC(byte) < ctx->rx_sub_channels) && (ESC_2_3LB(byte) == ESC_3LB_SC_PDU_START)) {
				    ctx->last_rx_sub_channel = ESC_2_SC(byte);
				    ctx->rx_state = FRAMER_STATE_CHR;
				    pos_out = 0;
				    if(ctx->csms_rx) {
                        init_csm_rx(&ctx->csms_rx[ESC_2_SC(byte)]);
                    }
				    pf = PDU_FLAG_START;
			    } else if((ESC_2_SC(byte) < ctx->rx_sub_channels) && (ESC_2_3LB(byte) == ESC_3LB_SWITCH_CHANNEL)) {
				    ctx->last_rx_sub_channel = ESC_2_SC(byte);
				    ctx->rx_state = FRAMER_STATE_CHR;
				    pos_out = 0;
			    } else {
				    ctx->rx_state = FRAMER_STATE_PS0;
			    }
            } else { // a non per sub-channel character
#if FRAMER_ALLOW_HANDLERS			    	
                if((byte >= EVENT_BASE) && (byte < ESC_ESC) && (ctx->handler != NULL)) {
				    ctx->handler(byte-EVENT_BASE);
			    }
#endif // FRAMER_ALLOW_HANDLERS
			    ctx->rx_state = FRAMER_STATE_PS0;
            }
			break;
        }
		case FRAMER_STATE_CHR: {
			if(byte == ctx->escape_char) {
				ctx->rx_state = FRAMER_STATE_ESC;
			} else {
                if(pos_out >= max_length_out) {
                    pos_in--;
                    out = 1;
                } else {
				    data_out[pos_out++] = byte;
					if(ctx->csms_rx)
						update_csm_rx(&ctx->csms_rx[ctx->last_rx_sub_channel], byte);
                }
			}
			break;
        }
		case FRAMER_STATE_ESC: {
            if(ESC_2_SC(byte) < MAX_SUBCHANNELS) { // a per sub-channel character
			    if((ESC_2_SC(byte) < ctx->rx_sub_channels) && (ESC_2_3LB(byte) < ESC_3LB_UNUSED_0)) {
				    if((pos_out != 0) && ((ESC_2_SC(byte) != ctx->last_rx_sub_channel) || (ESC_2_3LB(byte) == ESC_3LB_SC_PDU_START))) {
					    // we have converted data and we either start a new pdu or switched a channel, should return the current data, and not consume current byte
					    // we actually keep pos_in poiting to current byte and exit while staying in state FRAMER_STATE_ESC, so that we can go on in next entry
					    out = 1;
					    pos_in--;
				    } else if (ESC_2_3LB(byte) == ESC_3LB_SC_PDU_START) {
					    ctx->last_rx_sub_channel = ESC_2_SC(byte);
					    ctx->rx_state = FRAMER_STATE_CHR;
					    if(ctx->csms_rx) {
					    	//if(ctx->last_rx_sub_channel == 1) fprintf(stderr, "csm rx init %d\r\n", ESC_2_SC(byte));
	                        init_csm_rx(&ctx->csms_rx[ESC_2_SC(byte)]);
	                    }
			        } else if(ESC_2_3LB(byte) == ESC_3LB_SC_PDU_END) {
                        //  a pdu reached its end - we must not convert data after an end pdu....
				        pf |= PDU_FLAG_END;
					    ctx->rx_state = FRAMER_STATE_PS0;
					    out = 1;
			        } else if(ESC_2_3LB(byte) == ESC_3LB_SC_PDU_END_CSM) {
                        //  a pdu reached its end - we must not convert data after an end pdu, but still need to veryify csm....
					    ctx->last_rx_sub_channel = ESC_2_SC(byte);
					    ctx->rx_state = FRAMER_STATE_CS0;
                    } else {
					    ctx->last_rx_sub_channel = ESC_2_SC(byte);
					    ctx->rx_state = FRAMER_STATE_CHR;
			        }
                }
            } else { // a non per sub-channel character
                if(byte == ESC_ESC) {
                    if(pos_out >= max_length_out) {
                        pos_in--;
                        out = 1;
                    } else {
				        data_out[pos_out++] = ctx->escape_char;
						if(ctx->csms_rx)
							update_csm_rx(&ctx->csms_rx[ctx->last_rx_sub_channel], ctx->escape_char);
			            ctx->rx_state = FRAMER_STATE_CHR;
                    }
			    }
#if FRAMER_ALLOW_HANDLERS			    	
                else if((byte >= EVENT_BASE) && (byte < ESC_ESC) && (ctx->handler != NULL)) {
				    ctx->handler(byte-EVENT_BASE);
		            ctx->rx_state = FRAMER_STATE_CHR;
			    }                   
#endif // FRAMER_ALLOW_HANDLERS
            }
			break;
        }
		case FRAMER_STATE_CS0: {
            if(ctx->csms_rx != NULL) {
                check_csm_rx(&ctx->csms_rx[ctx->last_rx_sub_channel], byte); // first byte, we do not care about the result
            }
            ctx->rx_state = FRAMER_STATE_CS1;
            break;
        }
		case FRAMER_STATE_CS1: {
            pf |= PDU_FLAG_END;
            if(ctx->csms_rx == NULL) {
                pf |= PDU_FLAG_CSM_FAIL;
            } else {
                pf |= check_csm_rx(&ctx->csms_rx[ctx->last_rx_sub_channel], byte) ? PDU_FLAG_CSM_PASS : PDU_FLAG_CSM_FAIL;
            }
		    ctx->rx_state = FRAMER_STATE_PS0;
            out = 1;
            break;
        }
        default:
            ctx->rx_state = FRAMER_STATE_PS0;
		}
	}
	if(sub_channel) *sub_channel = ctx->last_rx_sub_channel;
	if(len_out) *len_out = pos_out;
	if(pdu_flags) *pdu_flags = pf;
	return pos_in;
}

UINT32 framer_tx_convert_single(framer_ctx_t *ctx, UINT8 pdu_flags, UINT8 *data_in, UINT32 length_in, UINT8 sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out) {
	UINT32 pos_in = 0;
	UINT32 pos_out = 0;
	UINT32 tmp1, tmp2, tmp3;
	if(ctx == NULL || len_out == NULL || data_out == NULL || (length_in && data_in == NULL) || sub_channel >= ctx->tx_sub_channels || ctx->segment_max_size < 4) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "framer_tx_convert_single: invalid buffers, segment size or subchannel=%u", sub_channel);
		if(len_out) *len_out = 0;
		return 0;
	}
	if(max_length_out < 4) {
		*len_out = 0;
		return 0; // no point in converting anything...
	}
	if((pdu_flags & PDU_FLAG_END) && (pdu_flags & PDU_FLAG_CSM_PASS) && ctx->csms_tx && (max_length_out < 6 || ctx->segment_max_size < 6)) {
		*len_out = 0;
		klog(KLOG_SUBJECT_KMGR, LL_WARNG, "framer_tx_convert_single: insufficient space for checksum trailer");
		return 0;
	}
	data_out[pos_out++] = ctx->escape_char;
	if(pdu_flags & PDU_FLAG_START) {
	    if(ctx->csms_tx) {
            ctx->csms_tx[sub_channel].csm = 0;
            init_csm_tx(&ctx->csms_tx[sub_channel]);
        }
        data_out[pos_out++] = SC3LB_2_ESC(sub_channel, ESC_3LB_SC_PDU_START);
    } else {
        data_out[pos_out++] = SC3LB_2_ESC(sub_channel, ESC_3LB_SWITCH_CHANNEL);
    }
    tmp3 = (pdu_flags & PDU_FLAG_CSM_PASS) && (ctx->csms_tx != NULL) ? 4 : 2;      // suffix length in case needed (end or csm+end) 
    while(pos_in != length_in) {
    	tmp1 = data_in[pos_in] == ctx->escape_char ? 2 : 1;                        // number of bytes that this char will consume on output
    	tmp2 = (pos_in == length_in - 1) && (pdu_flags & PDU_FLAG_END) ? tmp3 : 0; // spare bytes we on end, we must not consume last byte if we dont have space for suffix
    	if(pos_out + tmp1 > ctx->segment_max_size - tmp2) break;                   // must not generate more than a single limmited segment
    	if(pos_out + tmp1 > max_length_out - tmp2) break;                          // segment must not excced buffer limitation
	    if(ctx->csms_tx) {
    	    update_csm_tx(&ctx->csms_tx[sub_channel], data_in[pos_in]);
        }
    	if((data_out[pos_out++] = data_in[pos_in++]) == ctx->escape_char) {
    		data_out[pos_out++] = ESC_ESC;
    	}
    }
    if((pdu_flags & PDU_FLAG_END) && (pos_in == length_in)) {
	    data_out[pos_out++] = ctx->escape_char;
        if((pdu_flags & PDU_FLAG_CSM_PASS) && (ctx->csms_tx)) {
            data_out[pos_out++] = SC3LB_2_ESC(sub_channel, ESC_3LB_SC_PDU_END_CSM);
	        data_out[pos_out++] = ctx->csms_tx[sub_channel].csm >> 8;
            data_out[pos_out++] = ctx->csms_tx[sub_channel].csm & 0xFF;
        } else {
            data_out[pos_out++] = SC3LB_2_ESC(sub_channel, ESC_3LB_SC_PDU_END);
        }
    }
    *len_out = pos_out;
    return pos_in;
}

UINT32 framer_tx_convert(framer_ctx_t *ctx, UINT8 pdu_flags, UINT8 *data_in, UINT32 length_in, UINT8 sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out) {
	UINT32 pos_in = 0;
	UINT32 pos_out = 0;
	UINT32 tmp, seg_len_out;
	UINT8 pf = pdu_flags;
	if(ctx == NULL || len_out == NULL || data_out == NULL || (length_in && data_in == NULL)) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "framer_tx_convert: invalid buffers");
		if(len_out) *len_out = 0;
		return 0;
	}
	do {
		tmp = framer_tx_convert_single(ctx, pf, &data_in[pos_in], length_in - pos_in, sub_channel, &seg_len_out, &data_out[pos_out], max_length_out -pos_out);
		pos_in += tmp;
		pos_out += seg_len_out;
		pf &= (~PDU_FLAG_START);
	} while(tmp && (pos_out < max_length_out) && (pos_in < length_in));
	*len_out = pos_out;
	return pos_in;
}

#if FRAMER_ALLOW_HANDLERS

void framer_register_handler(framer_ctx_t *ctx, framer_event_handler_t handler) {
	ctx->handler = handler;
}

void framer_unregister_handler(framer_ctx_t *ctx) {
	ctx->handler = NULL;
}

void framer_handler(framer_ctx_t *ctx, UINT8 id) {
	(void)ctx;
	(void)id;
}

#endif // FRAMER_ALLOW_HANDLERS
