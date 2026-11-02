#ifndef _FRAMER_H
#define _FRAMER_H

//#define FRAMER_ALLOW_HANDLERS   0

typedef enum {
    ESC_3LB_SWITCH_CHANNEL = 0,
    ESC_3LB_SC_PDU_START   = 1,
    ESC_3LB_SC_PDU_END     = 2,
    ESC_3LB_SC_PDU_END_CSM = 3,
    ESC_3LB_UNUSED_0       = 4,
    ESC_3LB_UNUSED_1       = 5,
    ESC_3LB_UNUSED_2       = 6,
    ESC_3LB_UNUSED_3       = 7,
    ESC_3LB_LAST           = 8
} esc_3lb_t;

#define MAX_SUBCHANNELS     30
#define ESC_ESC             0xFF
#define EVENT_BASE          (MAX_SUBCHANNELS * ESC_3LB_LAST)
#define EVENT_NUM           (ESC_ESC - EVENT_BASE)
#define EVENT_NUM_BOOTSTRAP_V1               (EVENT_NUM - 1)
#define EVENT_NUM_BOOTSTRAP_V2               (EVENT_NUM - 2)
#define EVENT_NUM_BOOTSTRAP_V3               (EVENT_NUM - 3)
#define EVENT_NUM_BRCM_LOAD                  (EVENT_NUM - 4)
#define EVENT_NUM_BRCM_INIT_BW_250_LOAD      (EVENT_NUM - 5)
#define EVENT_NUM_BRCM_INIT_BW_500_LOAD      (EVENT_NUM - 6)
#define EVENT_NUM_BRCM_INIT_BW_750_LOAD      (EVENT_NUM - 7)
#define EVENT_NUM_BRCM_INIT_BW_1250_LOAD     (EVENT_NUM - 8)
#define EVENT_NUM_BRCM_INIT_ALIGNMENT_LOAD   (EVENT_NUM - 9)
#define EVENT_NUM_BRCM_INIT_BW_250_BER_LOAD  (EVENT_NUM - 10)
#define EVENT_NUM_BRCM_INIT_BW_500_BER_LOAD  (EVENT_NUM - 11)
#define EVENT_NUM_BRCM_INIT_BW_750_BER_LOAD  (EVENT_NUM - 12)
#define EVENT_NUM_BRCM_INIT_BW_1250_BER_LOAD (EVENT_NUM - 13)
#define EVENT_NUM_UNUSED1                    (EVENT_NUM - 14)
#define EVENT_NUM_UNUSED2                    (EVENT_NUM - 15)



#define ESC_2_SC(byte)      (((byte) >> 3) & 0x1F)
#define ESC_2_3LB(byte)     ((byte) & 0x7)
#define SC3LB_2_ESC(sc, lb) ((((sc) & 0x1F) << 3) | ((lb) & 0x7))

#define PDU_FLAG_START       1 // tx+rx, segment start at begining of data
#define PDU_FLAG_END         2 // tx+rx, segment ends at end of data
#define PDU_FLAG_CSM_PASS    4 // on tx - add csm at end, on rx, csm check passed
#define PDU_FLAG_CSM_FAIL    8 // rx only - csm check failed


typedef void (*framer_event_handler_t)(unsigned char event_num);

typedef struct {
    UINT16 csm;                   
} csm_tx_t;

typedef struct {
    UINT16 last_bytes;
    UINT16 csm;                   
} csm_rx_t;

typedef struct {
    UINT8 escape_char;            // byte used as an escape sequence
    UINT8 tx_sub_channels;
    UINT8 rx_sub_channels;
    UINT8 last_tx_sub_channel;    // last active tx sub-channel
    UINT8 last_rx_sub_channel;    // last active rx sub channel
    UINT8 rx_state;               // reader state
    UINT16 segment_max_size;      // maximal segment size
    csm_tx_t *csms_tx;              // array for check-sum calculation at tx (per tx sub-channel)
    csm_rx_t *csms_rx;            // array for check-sum calculation at rx (per tx sub-channel)
#if FRAMER_ALLOW_HANDLERS
	framer_event_handler_t handler; // for characters event_base up to esc_esc
#endif // FRAMER_ALLOW_HANDLERS
} framer_ctx_t;

extern void framer_ctx_init(framer_ctx_t *ctx, UINT8 tx_sub_channels, UINT8 rx_sub_channels, UINT16 segment_max_size, UINT8 escape_char, csm_tx_t *csms_tx, csm_rx_t *csms_rx);
extern void framer_ctx_reset(framer_ctx_t *ctx);
extern UINT32 framer_rx_convert(framer_ctx_t *ctx, UINT8 *data_in, UINT32 length_in, UINT8 *pdu_flags, UINT8 *sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out);
extern UINT32 framer_tx_convert_single(framer_ctx_t *ctx, UINT8 pdu_flags, UINT8 *data_in, UINT32 length_in, UINT8 sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out);
extern UINT32 framer_tx_convert(framer_ctx_t *ctx, UINT8 pdu_flags, UINT8 *data_in, UINT32 length_in, UINT8 sub_channel, UINT32 *len_out, UINT8 *data_out, UINT32 max_length_out);

#if FRAMER_ALLOW_HANDLERS

extern void framer_register_handler(framer_ctx_t *ctx, framer_event_handler_t handler);
extern void framer_unregister_handler(framer_ctx_t *ctx);
extern void framer_handler(framer_ctx_t *ctx, UINT8 id);

#endif // FRAMER_ALLOW_HANDLERS

#endif /* _FRAMER_H */
