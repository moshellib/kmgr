#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>


#include "gl_types.h"
#include "kdefs.h"
#include "wdr85100.h"
#include "framer.h"
#include "kmgr.h"
#include "time.h"

#ifdef _OCTEON
#include "sk_log.h"
#endif

char prompt[128] = "$ ";

pthread_mutex_t rx_event_mutex;
pthread_mutex_t kmgr_issue_command_mutex;
modem_ready_cb_t host_modem_ready_cb = NULL;
reset_k60s_cb_t host_reset_k60s_cb = NULL;
inv70_tx_freq_cb_t host_inv70_tx_freq_cb = NULL;

void modem_set_cb(modem_ready_cb_t modem_ready_cb) {
    host_modem_ready_cb = modem_ready_cb;
}

void inv70_tx_freq_cb(inv70_tx_freq_cb_t inv70_tx_freq_cb) {
    host_inv70_tx_freq_cb = inv70_tx_freq_cb;
}

void reset_k60s_set_cb(reset_k60s_cb_t reset_k60s_cb) {
    host_reset_k60s_cb = reset_k60s_cb;
}

#define USAGE_PARAM_LIST_SIZE        128
#define USAGE_DESCRIPTION_SIZE       128
#define USAGE_PARAMETERS_SIZE        2048

#define MAX_CMDS 256

typedef struct {
    char *name;
    char *param_list;
    char *description;
    char *parameters;
} cmd_help_t;

struct {
    char buff[MAX_CMDS * 512];
    UINT32 pos;
    UINT32 k60s_pos_base;
    UINT16 local_cmds_base;
    UINT16 n_local_cmds;
    UINT16 k60s_cmds_base;
    UINT16 n_k60s_cmds;
    cmd_help_t cmds[MAX_CMDS];
} help_db;

static INT32 add_to_help_db(char *item, char **ptr) {
    if(item == NULL || help_db.pos >= sizeof(help_db.buff)) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "add_to_help_db: invalid item or help database exhausted; used=%u", help_db.pos);
        return -1;
    }
    UINT32 avail = sizeof(help_db.buff) - help_db.pos;
    UINT32 len = snprintf(&help_db.buff[help_db.pos], avail, "%s", item);
    if(len + 1 > avail) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "add_to_help_db: help text exceeds storage; required=%u available=%u", len + 1, avail);
        return -1;
    }
    if(ptr) *ptr = &help_db.buff[help_db.pos];
    help_db.pos += (len +  1);
    return 0;
}

static void event_handler(unsigned char event_num);

typedef struct {
    UINT16 subject;
    UINT16 pos;
    UINT16 toobig;
    char buff[512];
} log_ctx_t;

sem_t kmgr_wait_sem;
pthread_t kplr_thread;
static UINT32 k60s_consecutive_failures = 0;

log_ctx_t log_assemble[] = {
        { KLOG_SUBJECT_SHRT, 0, 0, { 0 } },
        { KLOG_SUBJECT_BCKG, 0, 0, { 0 } },
        { KLOG_SUBJECT_MGMT, 0, 0, { 0 } },
        { KLOG_SUBJECT_VSHR, 0, 0, { 0 } },
};

struct {
    UINT16 wr;
    UINT16 rd;
    UINT8 q[256];
} events_q;

typedef void(*subc_rx_f_t)(void *ctx, UINT8 *data, UINT32 length);

static UINT32 issue_command(char *buff, int length, kmgr_response_t *response);

int wait_sem_seconds(sem_t *sem, int seconds) {
    struct timespec ts;
    int s;
    if(sem == NULL || seconds < 0) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wait_sem_seconds: invalid semaphore or timeout=%d", seconds);
        return -2;
    }
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wait_sem_seconds: failed to read clock: %s", strerror(errno));
        return -1;
    }
    ts.tv_sec += seconds;
    while ((s = sem_timedwait(sem, &ts)) == -1 && errno == EINTR) {
        continue;       /* Restart if interrupted by handler */
    }
    if(s == -1) {
        if(errno == ETIMEDOUT) return 1;
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wait_sem_seconds: wait failed: %s", strerror(errno));
        return -2;
    }
    return 0;
}

int relay_read(void *buff, int len) {
    return kio_read(K2H_SUBC_RELAY, buff, len);
}

int relay_write(void *buff, int len) {
    UINT8 tx_pdu_flags = PDU_FLAG_START | PDU_FLAG_END | PDU_FLAG_CSM_PASS;
    INT32 written = 0, tmp;
    while(written < len) {
        tmp = kio_write(H2K_SUBC_RELAY, buff + written, len - written, 1 /* commit */, tx_pdu_flags);
        if(tmp > 0) {
            written += tmp;
        }
        if(written < len) {
            usleep(tmp * 70); // wait for more space to be available for transmission - draining rate of 115200 bps
        }
    }
    return written;
}

void rx_mod_change(void *vctx, UINT8 *data, UINT32 length) {
    (void)vctx;
    (void)data;
    (void)length;
    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "error: kmgr got modulation change data"); // modulation change data should be jandled in kio
}

void rx_event(void *vctx, UINT8 *data, UINT32 length) {
    (void)vctx;
    UINT16 i, next_wr;
    pthread_mutex_lock(&rx_event_mutex);
    for(i = 0; i < length; i++) {
        next_wr = events_q.wr + 1 >= (UINT16)sizeof(events_q.q) ? 0 : events_q.wr + 1;
        if(next_wr == events_q.rd) {
            klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "events queue is full");
        } else {
            events_q.q[events_q.wr] = data[i];
            events_q.wr = next_wr;
        }
    }
    pthread_mutex_unlock(&rx_event_mutex);
    sem_post(&kmgr_wait_sem);
}

void rx_log(void *vctx, UINT8 *data, UINT32 length) {
    log_ctx_t *log_assemble = (log_ctx_t *) vctx;
    UINT8 *pos = data, *end;
    do {
        end = (UINT8*)strchr((char*)pos, '\n');
        if(end) {
            UINT16 cur = end - pos + 1;
            if((!log_assemble->toobig) && (log_assemble->pos + cur < sizeof(log_assemble->buff))) {
                memcpy(&log_assemble->buff[log_assemble->pos], data, cur);
                log_assemble->pos = log_assemble->pos + cur - 2;
                log_assemble->buff[log_assemble->pos] = 0;
                klog(log_assemble->subject, log_assemble->buff[0] - '0', "%s", log_assemble->buff + 2);
            }
            log_assemble->pos = 0;
            log_assemble->toobig = 0;
            pos = end + 1;
        } else if (!log_assemble->toobig) {
            UINT16 cur = &data[length] - pos;
            if(log_assemble->pos + cur < sizeof(log_assemble->buff)) {
                memcpy(&log_assemble->buff[log_assemble->pos], data, cur);
                log_assemble->pos += cur;
            } else {
                log_assemble->toobig = 1;
            }
        }
    } while(end);
}

struct {
    UINT16 rx_subc;
    subc_rx_f_t rx_f;
    void *vctx;
} plr_rx_table[] = {
    { K2H_SUBC_MODC,     rx_mod_change, NULL             },    // modulation change
    { K2H_SUBC_EVNT,     rx_event,      NULL             },    // events
    { K2H_SUBC_LOG_SHRT, rx_log,        &log_assemble[0] },    // log - shrt
    { K2H_SUBC_LOG_BCKG, rx_log,        &log_assemble[1] },    // log - bckg
    { K2H_SUBC_LOG_MGMT, rx_log,        &log_assemble[2] },    // log - mgmt
    { K2H_SUBC_LOG_VSHR, rx_log,        &log_assemble[3] },    // log - vshr
};

static UINT32 issue_k60s_command(char *buff, int length, kmgr_response_t *response) {
    char response_line[RSP_STATUS_MAX_SIZE + 3], body[RSP_BODY_MAX_SIZE + 3];
    char c, *r_s, *r_l, *r_r, cmd_str[16];
    UINT32 pos, body_len, len, i, expected_response_num = 1;
    UINT32 r_cnt = 0;
    UINT8 tx_pdu_flags = PDU_FLAG_START | PDU_FLAG_END | PDU_FLAG_CSM_PASS;
    UINT8 k2h_subch = K2H_SUBC_SSRFCP;
    static UINT8 timeout_on_receive_cnt = 0;
    struct timespec ts, tout;
    strncpy(cmd_str, (char *)buff, 8);
    //In case of "echo" cmd (used for k60s HC), the response is received on a different sub-channel
    if(strstr(cmd_str, "echo") != NULL)
    {
        k2h_subch = K2H_SUBC_SSRFCP_HC;
    }

    //empty rx buffer before writing
    r_cnt = kio_read(k2h_subch, &c, KIO_BUFF_SIZE);
    if ( r_cnt > 0 )
    {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "read %d characters before writing", r_cnt);
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "current buf: %s", buff);
        timeout_on_receive_cnt = 0;
    }

    int kio_write_res = kio_write(H2K_SUBC_MGMT, buff, length, 1 /* commit */, tx_pdu_flags);

    if (kio_write_res != length)
    	klog(KLOG_SUBJECT_KMGR, LL_ERROR,  "wanted length: %d, actual: %d", length, kio_write_res);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR clock_gettime failed");
        goto error;
    }
    tout.tv_nsec = ts.tv_nsec;
    tout.tv_sec = ts.tv_sec + atoi(env_timeout);


    //increase timeout. switch to relay mode can take more than 1sec
    if(strstr(cmd_str, "k60b r") != NULL)
    {
        tout.tv_sec += 1;
    }

    if ( timeout_on_receive_cnt == 1 || timeout_on_receive_cnt == 2 )
    {
        expected_response_num = 2;
    }
    else if ( timeout_on_receive_cnt > 2 ) // overrun may have occurred
    {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "reset timeout_on_receive_cnt. ttyS0 overrun may have occurred");
        timeout_on_receive_cnt = 0;
    }

    // next we need to wait for response...
    for (i=0 ; i < expected_response_num ; i++)
    {
        if ( i > 0)
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "timeout_on_receive_cnt > 0 . try polling next response");

        pos = 0;
        do {
            if(kio_read(k2h_subch, &c, 1)) {
                if(pos >= sizeof(response_line) - 2) {
                    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR out of sync due to long response line");
                    return 0;
                }
                response_line[pos++] = c;
            } else {
                clock_gettime(CLOCK_REALTIME, &ts);
                if( ts.tv_sec < tout.tv_sec ||
                    (ts.tv_sec == tout.tv_sec && ts.tv_nsec < tout.tv_nsec))
                {
                    c = 0;
                    usleep(100);
                } else {
                    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR: TIMEOUT on response(sync) receive");
                    goto timed_out;
                }
            }
        } while(c != '\n');
        response_line[pos] = 0;
        r_s = strchr(response_line, ' ');
        r_l = strrchr(response_line, ' ') + 1;
        r_r = strrchr(response_line, '\r');
        if((r_s == NULL) || (r_s + 1 != r_l) || (r_r == NULL) || (r_l >= r_r)) {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR out of sync due to bad response line...");
            goto error;
        }
        *r_s = *r_r = 0;
        body_len = atoi(r_l);
        if(body_len + 2 > sizeof(body)) {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR out of sync due to big body...");
            goto error;
        }
        if(body_len) {
            pos = 0;
            while(pos < body_len + 2) {
                len = kio_read(k2h_subch, body + pos, body_len + 2 - pos);
                if(len) {
                    pos += len;
                } else {
                    clock_gettime(CLOCK_REALTIME, &ts);
                    if( ts.tv_sec < tout.tv_sec ||
                        (ts.tv_sec == tout.tv_sec && ts.tv_nsec < tout.tv_nsec))
                    {
                        usleep(100);
                    } else {
                        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR: TIMEOUT on body receive");
                        goto timed_out;
                    }
                }
            }
            if((body[body_len] != '\r') || (body[body_len + 1] != '\n')) {
                klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ERROR out of sync due to bad termination body...");
                goto error;
            }
        }
        if(response) {
            response->status = &response->buff[0];
            response->status_length = r_s - response_line;
            strcpy(response->status, response_line);
            response->body = &response->buff[response->status_length] + 1;
            response->body_length = body_len;
            memcpy(response->body, body, response->body_length);
            response->body[response->body_length] = 0;
    //        klog(KLOG_SUBJECT_KMGR, LL_ERROR, ">>>>>>>>>>>cmd:%s response:%s", buff, response->body);
        }
    }
    if ( timeout_on_receive_cnt > 0 )
    {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "reset timeout_on_receive_cnt. received two responses. in sync again");
        timeout_on_receive_cnt = 0;
    }

    return 0;
    timed_out:
    if(response) {
        response->status = &response->buff[0];
        response->status_length = sprintf(response->status, "TIMEDOUT");
        response->body = NULL;
        response->body_length = 0;
    }
    timeout_on_receive_cnt += 1;
    return 0;
    error:
    if(response) {
        response->status = &response->buff[0];
        response->status_length = sprintf(response->status, "ERROR");
        response->body = NULL;
        response->body_length = 0;
    }
    return 0;
}

UINT32 issue_k60s_text_command(char *cmd, kmgr_response_t *response) {
    return issue_k60s_command(cmd, strlen(cmd), response);
}


static INT32 populate_k60s_help() {
    kmgr_response_t response, response2;
    char *current, *start;
    help_db.k60s_cmds_base = help_db.n_local_cmds;
    help_db.pos = help_db.k60s_pos_base;
    help_db.n_k60s_cmds = 0;

    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "learning help from k60s");
    issue_k60s_text_command("info\r\n", &response);
    if(strcmp(response.status, "OK") != 0) {
        klog(KLOG_SUBJECT_KMGR, LL_WARNG, "populate_k60s_help: MCU info unavailable; status=%s", response.status);
        return -1;
    }
    for(current = start = response.body; *current; current++) {
        if(*current == '\r') {
            char buff[128];
            if(help_db.k60s_cmds_base + help_db.n_k60s_cmds >= MAX_CMDS) {
                klog(KLOG_SUBJECT_KMGR, LL_ERROR, "populate_k60s_help: command table exhausted; maximum=%u", (unsigned int)MAX_CMDS);
                return -2;
            }
            cmd_help_t *cmd = &help_db.cmds[help_db.k60s_cmds_base + help_db.n_k60s_cmds];
            *current = 0;
            if(add_to_help_db(start, &cmd->name) < 0) return -2; // add command

            snprintf(buff, sizeof(buff), "info %s 1\r\n", start);
            issue_k60s_text_command(buff, &response2);
            if(strcmp(response2.status, "OK") != 0) { klog(KLOG_SUBJECT_KMGR, LL_ERROR, "populate_k60s_help: parameter query failed; command=%s status=%s", start, response2.status); return -2; }
            if(add_to_help_db(response2.body, &cmd->param_list) < 0) return -2;

            snprintf(buff, sizeof(buff), "info %s 2\r\n", start);
            issue_k60s_text_command(buff, &response2);
            if(strcmp(response2.status, "OK") != 0) { klog(KLOG_SUBJECT_KMGR, LL_ERROR, "populate_k60s_help: description query failed; command=%s status=%s", start, response2.status); return -2; }
            if(add_to_help_db(response2.body, &cmd->description) < 0) return -2;

            snprintf(buff, sizeof(buff), "info %s 3\r\n", start);
            issue_k60s_text_command(buff, &response2);
            if(strcmp(response2.status, "OK") != 0) { klog(KLOG_SUBJECT_KMGR, LL_ERROR, "populate_k60s_help: help query failed; command=%s status=%s", start, response2.status); return -2; }
            if(add_to_help_db(response2.body, &cmd->parameters) < 0) return -2;

            ++help_db.n_k60s_cmds;
            start = current + 2; // get ready for next command
        }
    }
    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "learned %d k60s commands", help_db.n_k60s_cmds);
    return 0;
}

// local commands




typedef int(*lcmd_f_cb_t)(int argc, char**argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]);
typedef struct {
    char *name;
    lcmd_f_cb_t lcmd_f_cb;
} lcmd_t;

static const lcmd_t lcmds[]; // forward declaration
static const UINT32 n_lcmds; // forward declaration

char env_baud[64]              = "115200";            // tty speed used for communication
char env_brcm[2]               = "1";                 // boot k60b
char env_brcm_l_bin[128]       = "";                  // last broadcom bin file that was loaded
char env_brcm_l_status[16]     = "";                  // last broadcom bin file loading status
char env_brcmbin[128]          = "init_500.bin";      // broadcom's bin file in kfs, used in bootstrap handler
char env_brcmfw[128]           = "bcm85100mc.fw";     // firmware file in kfs, used in bootstrap handler
char env_forceburn[2]          = "0";                 // force re-burning of k60s image (even if versions are the same)
char env_forcesock[2]          = "0";                 // force listening to socket even if sock file is missing in env_kfs
char env_healthchk[2]          = "0";                 // force health check "0" or "1"
char env_healthchk_reinit[2]   = "1";                 // force health check "0", "1", or "-"
char env_help[2]               = "1";                 // populate help-db with items learned from k60s
char env_kfs[64]               = "/etc/kfs";          // kfs directory, usefull in multi-kfs env
char env_port[16]              = "1970";              // port number we listen to....
char env_role[2]               = "0";                 // role passed to k60b
char env_seeprom[2]            = "1";                 // convert serial eeprom files to ipf and and pass to k60
char env_sromfile0[128]        = "/tmp/eeprom70.lua"; // location of serial eeprom file 0
char env_sromfile1[128]        = "/tmp/eeprom80.lua"; // location of serial eeprom file 1
char env_timeout[16]           = "1";                 // location of serial eeprom file 1
char env_tty[64]               = "/dev/ttyS0";        // tty used for communication

static const struct {
    char *name;
    char *value;
    UINT32 size;
    UINT8  rw;
} env_items[] = {
        { "baud",            &env_baud[0],             sizeof(env_baud)            ,0 },
        { "brcm",            &env_brcm[0],             sizeof(env_brcm)            ,1 },
        { "brcm_l_bin",      &env_brcm_l_bin[0],       sizeof(env_brcm_l_bin)      ,0 },
        { "brcm_l_status",   &env_brcm_l_status[0],    sizeof(env_brcm_l_status)   ,0 },
        { "brcmbin",         &env_brcmbin[0],          sizeof(env_brcmbin)         ,1 },
        { "brcmfw",          &env_brcmfw[0],           sizeof(env_brcmfw)          ,1 },
        { "forceburn",       &env_forceburn[0],        sizeof(env_forceburn)       ,1 },
        { "forcesock",       &env_forcesock[0],        sizeof(env_forcesock)       ,1 },
        { "healthchk",       &env_healthchk[0],        sizeof(env_healthchk)       ,1 },
        { "healthchk_reinit",&env_healthchk_reinit[0], sizeof(env_healthchk_reinit),1 },
        { "help",            &env_help[0],             sizeof(env_help)            ,1 },
        { "kfs",             &env_kfs[0],              sizeof(env_kfs)             ,1 },
        { "port",            &env_port[0],             sizeof(env_port)            ,0 },
        { "role",            &env_role[0],             sizeof(env_role)            ,1 },
        { "seeprom",         &env_seeprom[0],          sizeof(env_seeprom)         ,1 },
        { "sromfile0",       &env_sromfile0[0],        sizeof(env_sromfile0)       ,1 },
        { "sromfile1",       &env_sromfile1[0],        sizeof(env_sromfile1)       ,1 },
        { "timeout",         &env_timeout[0],          sizeof(env_timeout)         ,1 },
        { "tty",             &env_tty[0],              sizeof(env_tty)             ,0 },
};

int lcmd_f_brcmcfg(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
//	we must switch to relay mode before using this function!
//	using switch_k60s_to_relay_mode().
    UINT8 mcBinBuf[BCM85100_MAX_CONFIG_FILE_LEN];
    char fname[256];
    int binLen, status;
    FILE *fd;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "<file>");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "reconfigures brcm. this command requires to switch to relay mode first (k60b r)");
        snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "<file> bin file (in kfs) that we want to use\n");
        return 0;
    }
    if(argc == 2) {

        snprintf(fname, sizeof(fname), "%s/%s", env_kfs, argv[1]);
        if((fd = fopen(fname, "rb")) == NULL) {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "lcmd_f_brcmcfg: cannot open configuration '%s': %s", fname, strerror(errno));
            response->status_length = sprintf(response->status, "BADARGS");
        } else {
        	strcpy(env_healthchk, "0");
            binLen = fread(mcBinBuf, 1, sizeof(mcBinBuf), fd);
            fclose(fd);
            klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "reconfiguring k60b with file %s (%d bytes)", argv[1], binLen);
            status = wdr85100_config(mcBinBuf, binLen);
            if(status) {
                klog(KLOG_SUBJECT_KMGR, LL_ERROR, "wdr85100_config failed with %d", status);
                klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "failed to reload brcm bin file - exit modemd process and restart via watchdog");

#ifdef _OCTEON
                system("echo \"failed to reload brcm bin file - exit modemd process and restart via watchdog\"   > /dev/ttyS1");
                LOG(LOG_ERR,  "failed to reload brcm bin file - exit modemd process and restart via watchdog");
                LOG(LOG_CRIT, "failed to reload brcm bin file - exit modemd process and restart via watchdog");
#endif //  _OCTEON

                sync();

                exit(1);

            } else {
                klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "wdr85100_config succeeded");
            }
            snprintf(env_brcm_l_bin, sizeof(env_brcm_l_bin), "%s", argv[1]);
            snprintf(env_brcm_l_status, sizeof(env_brcm_l_status), "%d", status);
        }

        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "will issue k60b d");
        issue_k60s_text_command("k60b d\r\n", response); // switch to direct mode
        //dump_response("got: ", DUMP_RESPONSE_BODY_STYLE_BODY, response);

        if(strcmp(env_healthchk_reinit, "-") != 0) {
            strncpy(env_healthchk, env_healthchk_reinit, sizeof(env_healthchk));
        }
    }
    else {
        response->status_length = sprintf(response->status, "BADARGS");
    }
    return 0;
}


static int lcmd_f_brcmfull(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    UINT8 c;
    (void)argc;
    (void)argv;
    (void)body;
    (void)body_length;
    if(usage) {
        usage[0][0] = 0; //snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "inject event to start k60b loading sequence");
        usage[2][0] = 0; //snprintf(usage[2], USAGE_PARAMETERS_SIZE, "");
        return 0;
    }
    c = EVENT_NUM_BRCM_LOAD;
    rx_event(NULL, &c, 1);
    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "initiating k60b load");

    if (response)
    {
        response->status = &(response->buff[0]);
        response->status_length = sprintf(response->status, "OK");
        response->body_length = 0;
    }
    return 0;
}


static int lcmd_f_clear(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    UINT8 c;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "<item>");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "clear an item");
        snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "<item> we want to clear, for now only ssrfcp is defined\n");
        return 0;
    }
    if((argc == 2) && (strcmp(argv[1], "ssrfcp") == 0)) {
        while(kio_read(K2H_SUBC_SSRFCP, &c, 1) > 0);
        response->status_length = sprintf(response->status, "OK");
    } else {
        response->status_length = sprintf(response->status, "BADARGS");
    }
    return 0;
}


static int lcmd_f_env(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    INT32 i, idx = -1;
    UINT16 ii, pos;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "[name] [value]");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "sets a kmgr environment variable");
        pos = snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "[name] name of variable: ");
        for(ii=0; ii < sizeof(env_items)/sizeof(env_items[0]); ii++) {
            pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, " %s", env_items[ii].name);
        }
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, "\n");
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, "[value] value of variable\n");
        return 0;
    }
    char *w_start = &response->buff[RSP_STATUS_MAX_SIZE], *w_pos = w_start, *w_last = &response->buff[sizeof(response->buff)-1];
    if(argc <= 1) {
        for(i=0; i < (INT32)(sizeof(env_items)/sizeof(env_items[0])); i++) {
            char *w_cur = w_pos;
            w_pos += snprintf(w_pos, w_last - w_pos, "%s: %s\n", env_items[i].name, env_items[i].value);
            if(w_pos > w_last) {
                w_pos = w_cur;
                break;
            }
        }
        response->status_length = sprintf(response->status, "OK");
        response->body = w_start;
        response->body_length = w_pos - w_start;
    } else if ((argc == 2) || (argc == 3)) {
        for(i=0; i < (INT32)(sizeof(env_items)/sizeof(env_items[0])); i++) {
            if(strcmp(env_items[i].name, argv[1]) == 0) {
                idx = i;
                break;
            }
        }
        if(idx >= 0) {
            if(argc == 2) {
                response->status_length = sprintf(response->status, "OK");
                w_pos += snprintf(w_pos, w_last - w_pos, "%s: %s\n", env_items[idx].name, env_items[idx].value);
                response->body = w_start;
                response->body_length = w_pos - w_start;
            } else {
                if(env_items[idx].rw) {
                    response->status_length = sprintf(response->status, "OK");
                    snprintf(env_items[idx].value, env_items[idx].size, "%s", argv[2]);
                } else {
                    response->status_length = sprintf(response->status, "READONLY");
                }
            }
        } else {
            response->status_length = sprintf(response->status, "BADARGS");
        }
    } else {
        response->status_length = sprintf(response->status, "BADARGS");
    }
    return 0;
}


static int lcmd_f_help(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    UINT32 j;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    char *help_start = &response->buff[RSP_STATUS_MAX_SIZE], *help_pos = help_start, *help_last = &response->buff[sizeof(response->buff)-1];
    (void)body;
    (void)body_length;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "[command]");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "gives help for commands");
        snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "[command] commands for which we want help\n");
        return 0;
    }
    help_pos[0] = 0;
    if(argc <= 1) {
        for(j = help_db.local_cmds_base; j < help_db.k60s_cmds_base + help_db.n_k60s_cmds; j++) {
            if(j == help_db.local_cmds_base) {
                help_pos += snprintf(help_pos, help_last - help_pos, "kmgr commands:\n");
            }
            if(j == help_db.k60s_cmds_base) {
                help_pos += snprintf(help_pos, help_last - help_pos, "k60s commands:\n");
            }
            help_pos += snprintf(help_pos, help_last - help_pos, "%s %s - %s\n", help_db.cmds[j].name, help_db.cmds[j].param_list, help_db.cmds[j].description);
        }
        response->status_length = sprintf(response->status, "OK");
        response->body = help_start;
        response->body_length = help_pos - help_start;
    } else if (argc == 2) {
        cmd_help_t *cmd = NULL;
        for(j = help_db.local_cmds_base; j < help_db.k60s_cmds_base + help_db.n_k60s_cmds; j++) {
            if(strcmp(argv[1], help_db.cmds[j].name) == 0) {
                cmd = &help_db.cmds[j];
                break;
            }
        }
        if(cmd) {
            help_pos += snprintf(help_pos, help_last - help_pos, "%s %s - %s\n", help_db.cmds[j].name, help_db.cmds[j].param_list, help_db.cmds[j].description);
            help_pos += snprintf(help_pos, help_last - help_pos, "%s", help_db.cmds[j].parameters);
            response->status_length = sprintf(response->status, "OK");
            response->body = help_start;
            response->body_length = help_pos - help_start;
        } else {
            response->status_length = sprintf(response->status, "BADARGS");
        }
    } else {
        response->status_length = sprintf(response->status, "BADARGS");
    }
    return 0;
}


static int lcmd_f_hlearn(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    (void)argc;
    (void)argv;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        usage[0][0] = 0; //snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "learn or re-learn k60s help items");
        usage[2][0] = 0; //snprintf(usage[2], USAGE_PARAMETERS_SIZE, "");
        return 0;
    }
    populate_k60s_help();
    response->status_length = sprintf(response->status, "OK");
    return 0;
}


static int lcmd_f_log_filter(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    UINT32 ii, pos;
    char *name;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "<type> <subject> [level]");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "reads or writes filter level");
        pos = snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "<type> store or display\n");
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE,
                "<subject> subject for which we want to control level: ");
        for(ii=0; (name = klog_subj_to_name(ii)) != NULL; ii++) {
            pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, " %s", name);
        }
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, "\n");
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE,
                "[level] level to set: ");
        for(ii=0; (name = klog_level_to_name(ii)) != NULL; ii++) {
            pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, " %s", name);
        }
        pos += snprintf(&usage[2][pos], USAGE_PARAMETERS_SIZE, "\n");
        return 0;
    }
    if((argc != 3) && (argc != 4)) return -1;
    INT32 type, subj, level;
    if(strcmp(argv[1], "store") == 0) {
        type = KLOG_TYPE_STORE;
    } else if(strcmp(argv[1], "display") == 0) {
        type = KLOG_TYPE_DISPLAY;
    } else {
        response->status_length = sprintf(response->status, "BADARGS");
        return 0;
    }
    if((subj = klog_name_to_subj(argv[2])) < 0) {
        response->status_length = sprintf(response->status, "BADARGS");
        return 0;
    }
    if(argc == 3) {
        level = klog_get_filter_level(subj, type);
        name =  klog_level_to_name(level);
        response->status_length = sprintf(response->status, "OK");
        response->body = &response->buff[RSP_STATUS_MAX_SIZE];
        response->body_length = sprintf(response->body, "%s", name);
    } else {
        if((level = klog_name_to_level(argv[3])) < 0) {
            response->status_length = sprintf(response->status, "BADARGS");
        } else {
            klog_set_filter_level(subj, type, level);
            response->status_length = sprintf(response->status, "OK");
        }
    }
    return 0;
}


static int lcmd_f_lua2ipf(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    response->status = &(response->buff[0]);
    response->body_length = 0;
    char *ipf_start = &response->buff[RSP_STATUS_MAX_SIZE], *ipf_last = &response->buff[sizeof(response->buff)-1];
    (void)argc;
    (void)argv;
    UINT32 bytes;
    if(usage) {
        usage[0][0] = 0; //snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "return conversion of lua to ipf");
        usage[2][0] = 0; //snprintf(usage[2], USAGE_PARAMETERS_SIZE, "");
        return 0;
    }
    bytes = make_ipf_by_buff(body, body_length, ipf_start, ipf_last-ipf_start);
    response->status_length = sprintf(response->status, "OK");
    response->body = ipf_start;
    response->body_length = bytes;
    return 0;
}


static int lcmd_f_reset_k60s(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    (void)argc;
    (void)argv;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        usage[0][0] = 0; //snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "reset k60s");
        usage[2][0] = 0; //snprintf(usage[2], USAGE_PARAMETERS_SIZE, "");
        return 0;
    }
    if(host_reset_k60s_cb == NULL) {
        response->status_length = sprintf(response->status, "UNHOOKED");
    } else {
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "reset k60s");
        strcpy(env_healthchk, "0");
        host_reset_k60s_cb();
        response->status_length = sprintf(response->status, "OK");
    }
    return 0;
}


static int lcmd_f_seeprom(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]) {
    char fname0[128], fname1[128];
    char ipf[8192];
    char cmd[sizeof(ipf) + 128];
    UINT32 length, l, i, pos, size, csum;
    (void)body;
    (void)body_length;
    response->status = &(response->buff[0]);
    response->body_length = 0;
    if(usage) {
        snprintf(usage[0], USAGE_PARAM_LIST_SIZE, "<file1> <file2>");
        snprintf(usage[1], USAGE_DESCRIPTION_SIZE, "send ipf based on serial eeprom files");
        snprintf(usage[2], USAGE_PARAMETERS_SIZE,
                "<file1> first serial eeprom file\n"
                "<file2> second serial eeprom file\n");
        return 0;
    }
    if(argc != 3) return -1;
    sprintf(fname0, "%s", argv[1]);
    sprintf(fname1, "%s", argv[2]);
    length = make_ipf_by_files(fname0, fname1, ipf, sizeof(ipf));

    for(pos = 0; pos < length; pos+= size) {
        size = length - pos <= 512 ? length - pos : 512; // we split to 512 byes to avoid out of buffer in k60
        l = sprintf(cmd, "ipf w%d %d\r\n", (int)pos, (int)size);
        memcpy(&cmd[l], ipf + pos, size);
        l += size;
        l += sprintf(&cmd[l], "\r\n");
        issue_command(cmd, l, response);
        //dump_response("got: ", DUMP_RESPONSE_BODY_STYLE_BODY, &lresponse);

    }
    csum = 0;
    for(i = 0; i < length; i++) {
        csum += (UINT8)ipf[i];
    }

    l = sprintf(cmd, "ipf c%d %d %d\r\n", (int)length, (int)csum, 0 /* no body*/);
    issue_command(cmd, l, response);
    //dump_response("got: ", DUMP_RESPONSE_BODY_STYLE_BODY, &lresponse);
    if(strcmp(response->status, "OK") != 0) {
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "ipf loading failed");
    } else {
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "ipf loading succeeded");
    }

    return 0;
}


static const lcmd_t lcmds[] = {
        {"brcmcfg",    lcmd_f_brcmcfg      },
        {"brcmfull",   lcmd_f_brcmfull     },
        {"clear",      lcmd_f_clear        },
        {"env",        lcmd_f_env          },
        {"help",       lcmd_f_help         },
        {"hlearn",     lcmd_f_hlearn       },
        {"log-filter", lcmd_f_log_filter   },
        {"lua2ipf",    lcmd_f_lua2ipf      },
        {"reset-k60s", lcmd_f_reset_k60s   },
        {"seeprom",    lcmd_f_seeprom      },
};
static const UINT32 n_lcmds = sizeof(lcmds) / sizeof(lcmds[0]);

static int issue_local_command(char *buff, int length, kmgr_response_t *response) {
    char req_line[REQ_LINE_MAX_SIZE + 2 + 1], *p, *body;
    UINT32 argc, body_length;
    UINT8 *argv[512];
    kmgr_response_t lresponse;
    UINT32 i, found = 0;
    if(response == NULL) response = &lresponse;
    memset(response, 0, sizeof(*response));
    response->status = &response->buff[0];

    p = strchr(buff, '\n');
    body_length = length - (p - buff);
    body = body_length ? p + 1: NULL;
    if((p == NULL) || (((UINT32)(p - buff)) > sizeof(req_line) - 1) || (((UINT32)(p - buff)) < 2)) {
        response->status_length = sprintf(response->status, "BADLINE");
    } else {
        memcpy(req_line, buff, p - buff);
        req_line[p - buff + 1] = 0;
        if(make_args(req_line, p - buff, sizeof(argv)/sizeof(argv[0]), &argc, (char **)argv) < 0) {
            response->status_length = sprintf(response->status, "BADLINE");
        } else {
            for(i=0; (i<sizeof(lcmds)/sizeof(lcmds[0])) && (found == 0); i++) {
                if(strcmp((char*)argv[0], lcmds[i].name) == 0) {
                    if(lcmds[i].lcmd_f_cb(argc, (char**)argv, body_length, body, response, NULL) < 0) {
                        klog(KLOG_SUBJECT_KMGR, LL_WARNG, "issue_local_command: invalid arguments; command=%s argc=%u", lcmds[i].name, argc);
                        response->status_length = sprintf(response->status, "BADARGS");
                        response->body_length = 0;
                    }
                    found++;
                }
            }
        }
    }
    return found;
}

// issue_command

static UINT32 issue_command(char *buff, int length, kmgr_response_t *response) {
    if(length > 2) { // we should have more than \r\n...
        if(issue_local_command(buff, length, response)) {
            //klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "kmgr executed command locally");
        } else {
            issue_k60s_command(buff, length, response);
        }
    } else {
        if(response) {
            memset(response, 0, sizeof(*response));
            response->status = &response->buff[0];
            response->status_length = sprintf(response->status, "BADLINE");
        }
    }
    return 0;
}


static void load_k60s(void) {
    int i, rv = 0;
    char fname[256];
    snprintf(fname, sizeof(fname), "%s/%s", env_kfs, "z_skm.hex");
    rv = loader_entry(env_tty, env_baud, fname, ENV_TRUE(env_forceburn));
    if (rv != 0) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "failed to load k60s rv=%d", rv);
        klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "failed to load k60s - exit modemd process and restart via watchdog");

#ifdef _OCTEON
        system("echo \"failed to load k60s - exit modemd process and restart via watchdog\"   > /dev/ttyS1");
        LOG(LOG_ERR,  "failed to load k60s - exit modemd process and restart via watchdog");
        LOG(LOG_CRIT, "failed to load k60s - exit modemd process and restart via watchdog");
#endif //  _OCTEON

        sync();

        exit(1);
    }
    for(i=0; i<1000000; i++);
}


static int load_k60b(void) {
    FILE *fd;
    UINT8 mcFwBuf[BCM85100_FW_MAX_SIZE_IN_BYTES];
    UINT8 mcBinBuf[BCM85100_MAX_CONFIG_FILE_LEN];
    char fname[256];
    int fwLen, binLen, rv = 0;
    snprintf(fname, sizeof(fname), "%s/%s", env_kfs, env_brcmfw);
    if((fd = fopen(fname, "rb")) == NULL) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "load_k60b: cannot open firmware '%s': %s", fname, strerror(errno));
        rv = -1;
    } else {
        fwLen = fread(mcFwBuf, 1, sizeof(mcFwBuf), fd);
        fclose(fd);
        snprintf(fname, sizeof(fname), "%s/%s", env_kfs, env_brcmbin);
        if((fd = fopen(fname, "rb")) == NULL) {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "load_k60b: cannot open configuration '%s': %s", fname, strerror(errno));
            rv = -2;
        } else {
            binLen = fread(mcBinBuf, 1, sizeof(mcBinBuf), fd);
            fclose(fd);
            klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "k60b: fw is %d bytes, bin is %d bytes", fwLen, binLen);
            rv = wdr85100_load_config(mcBinBuf, binLen, mcFwBuf, fwLen);
            if(rv)
            {
                klog(KLOG_SUBJECT_KMGR, LL_ERROR, "k60b initialization failed with rv %d", rv);
                klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "failed to load brcm bin/fw file - exit modemd process and restart via watchdog");

#ifdef _OCTEON
                system("echo \"failed to reload brcm bin file - exit modemd process and restart via watchdog\"   > /dev/ttyS1");
                LOG(LOG_ERR,  "failed to reload brcm bin file - exit modemd process and restart via watchdog");
                LOG(LOG_CRIT, "failed to reload brcm bin file - exit modemd process and restart via watchdog");
#endif //  _OCTEON

                sync();

                exit(1);

            }
            else
            {
                klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "k60b initialization completed with rv %d", rv);
            }
            snprintf(env_brcm_l_bin, sizeof(env_brcm_l_bin), "%s", env_brcmbin);
            snprintf(env_brcm_l_status, sizeof(env_brcm_l_status), "%d", rv);

        }
    }
    return rv;
}


static void event_handler(unsigned char event_num) {
    char buff[1024];
    UINT32 len, inv70_tx_freq;
    int rv;
    kmgr_response_t response;
	UINT32 argc;
	char *argv[512];
	argc = 2;
	argv[0] = "brcmcfg";
    switch(event_num) {
    case EVENT_NUM_BOOTSTRAP_V1:
        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "bootstrap...");
        kio_control(KIO_FLAG_RAW, 1);
        kio_control(KIO_FLAG_NO_READ, 1);
        kio_control(KIO_FLAG_FLUSH_RX, 1);
        load_k60s();
        kio_write(H2K_SUBC_RAW, "g", 1, 1 /* commit */, 0 /* PDU flags */);
        if(strcmp(env_healthchk_reinit, "-") != 0) {
            strncpy(env_healthchk, env_healthchk_reinit, sizeof(env_healthchk));
        }
        // fall thtough
    case EVENT_NUM_BRCM_LOAD: // dummy event for only loading broadcom, usefull when running k60s from debugger
        kio_control(KIO_FLAG_RAW, 0);
        kio_control(KIO_FLAG_NO_READ, 0);
        sleep(1);

        if (host_inv70_tx_freq_cb) {
            inv70_tx_freq = host_inv70_tx_freq_cb();
            if (inv70_tx_freq == 0) //tx == 80
                snprintf(env_role, sizeof("1"), "1");
            else if (inv70_tx_freq == 70) //tx == 70
                snprintf(env_role, sizeof("0"), "0");
        }

        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "setting role as %s", env_role);
        len = snprintf(buff, sizeof(buff), "ctl role %s\r\n", env_role);
        issue_command(buff, len, NULL);
        if(ENV_TRUE(env_help)) {
            populate_k60s_help();
        }
        if(ENV_TRUE(env_seeprom)) {
            len = snprintf(buff, sizeof(buff), "seeprom %s %s\r\n", env_sromfile0, env_sromfile1);
            issue_command(buff, len, NULL);
        }
        if(ENV_TRUE(env_brcm)) { // broadcom loading
            klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "will issue k60b r...");
            issue_k60s_text_command("k60b r\r\n", &response); // switch to relay mode
            //dump_response("got: ", DUMP_RESPONSE_BODY_STYLE_BODY, &response);
            if((rv = load_k60b()) == 0) {
                klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "k60b loading ok, will issue k60b d...");
                issue_k60s_text_command("k60b d\r\n", &response); // switch to done
                //dump_response("got: ", DUMP_RESPONSE_BODY_STYLE_BODY, &response);
            } else {
                klog(KLOG_SUBJECT_KMGR, LL_ERROR, "k60b loading failed with return code %d", rv);
            }
        }
        k60s_consecutive_failures = 0;
        if(host_modem_ready_cb) {
            host_modem_ready_cb();
        }
        break;
    case EVENT_NUM_BRCM_INIT_ALIGNMENT_LOAD:
		argv[1] = "init_alignment.bin";
		lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
    	break;
    case EVENT_NUM_BRCM_INIT_BW_250_LOAD:
		argv[1] = "init_250.bin";
		lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
		break;
    case EVENT_NUM_BRCM_INIT_BW_500_LOAD:
		argv[1] = "init_500.bin";
		lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
		break;
    case EVENT_NUM_BRCM_INIT_BW_750_LOAD:
        argv[1] = "init_750.bin";
        lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
        break;
    case EVENT_NUM_BRCM_INIT_BW_1250_LOAD:
        argv[1] = "init_1250.bin";
        lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
        break;
    case EVENT_NUM_BRCM_INIT_BW_250_BER_LOAD:
		argv[1] = "initber_250.bin";
		lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
		break;
    case EVENT_NUM_BRCM_INIT_BW_500_BER_LOAD:
		argv[1] = "initber_500.bin";
		lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
		break;
    case EVENT_NUM_BRCM_INIT_BW_750_BER_LOAD:
        argv[1] = "initber_750.bin";
        lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
        break;
    case EVENT_NUM_BRCM_INIT_BW_1250_BER_LOAD:
        argv[1] = "initber_1250.bin";
        lcmd_f_brcmcfg(argc, (char **)argv, 0, NULL, &response, NULL);
        break;
    default:
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "ignoring event %d/%d", event_num, EVENT_NUM - 1);
        break;
    }
}


void *kmgr_thread_poller(void* arg) {
    (void)arg;
    UINT8 rdbuff[1024 + 1];
    UINT32 len, i, ops;

    while (1) {
        ops = 0;
        for(i=0; i<sizeof(plr_rx_table)/sizeof(plr_rx_table[0]); i++) {
            if(plr_rx_table[i].rx_f) {
                if((len = kio_read(plr_rx_table[i].rx_subc, rdbuff, sizeof(rdbuff) - 1)) > 0) {
                    rdbuff[len] = 0;
                    plr_rx_table[i].rx_f(plr_rx_table[i].vctx, rdbuff, len);
                    ops++;
                }
            }
        }
        if(ops == 0) {
            usleep(1000);
        }
    }
    return NULL;
}

struct {
    char *req_buff;
    UINT32 req_length;
    sem_t wait_sem;
    kmgr_response_t response;
} kmgr_clients[KMGR_CLIENT_LAST];

extern kmgr_response_t *kmgr_issue_command(int client, char *buff, int length) {
    if(client < 0 || client >= KMGR_CLIENT_LAST || buff == NULL || length <= 0) {
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kmgr_issue_command: invalid client=%d request_length=%d", client, length);
        return NULL;
    }
    kmgr_clients[client].req_length = length;
    kmgr_clients[client].req_buff = buff; // must be set after length
    pthread_mutex_lock(&kmgr_issue_command_mutex);
    sem_post(&kmgr_wait_sem);
    sem_wait(&kmgr_clients[client].wait_sem);
    pthread_mutex_unlock(&kmgr_issue_command_mutex);
    return &kmgr_clients[client].response;
}

extern int switch_k60s_to_relay_mode(void) {
    kmgr_response_t response;
    response.status = &(response.buff[0]);
    response.body_length = 0;
    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "will issue k60b r");
    issue_k60s_text_command("k60b r\r\n", &response);
    if (strcmp(response.status, "OK") == 0)
        return 1;

    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "switch_k60s_to_relay_mode: command failed; status=%s", response.status);
    return 0;
}

extern void load_brcm_bin_file(int binFileType) {
	UINT8 c;
	switch (binFileType) {
	case BRCM_ALIGNMENT_BIN_FILE:
		c = EVENT_NUM_BRCM_INIT_ALIGNMENT_LOAD;
		break;
	case BRCM_BW_250_BIN_FILE:
		c = EVENT_NUM_BRCM_INIT_BW_250_LOAD;
		break;
	case BRCM_BW_500_BIN_FILE:
		c = EVENT_NUM_BRCM_INIT_BW_500_LOAD;
		break;
    case BRCM_BW_750_BIN_FILE:
        c = EVENT_NUM_BRCM_INIT_BW_750_LOAD;
        break;
    case BRCM_BW_1250_BIN_FILE:
        c = EVENT_NUM_BRCM_INIT_BW_1250_LOAD;
        break;
	case BRCM_BW_250_BER_BIN_FILE:
		c = EVENT_NUM_BRCM_INIT_BW_250_BER_LOAD;
		break;
	case BRCM_BW_500_BER_BIN_FILE:
		c = EVENT_NUM_BRCM_INIT_BW_500_BER_LOAD;
		break;
    case BRCM_BW_750_BER_BIN_FILE:
        c = EVENT_NUM_BRCM_INIT_BW_750_BER_LOAD;
        break;
    case BRCM_BW_1250_BER_BIN_FILE:
        c = EVENT_NUM_BRCM_INIT_BW_1250_BER_LOAD;
        break;
	default:
		c = EVENT_NUM_BRCM_INIT_BW_500_LOAD;
		break;
	}
	rx_event(NULL, &c, 1);
}


void kmgr_init(char *env_fname) {
    FILE *fd;
    char line[512];
    char usage_param_list[USAGE_PARAM_LIST_SIZE], usage_description[USAGE_DESCRIPTION_SIZE], usage_parameters[USAGE_PARAMETERS_SIZE];
    char *lusage[3] = {usage_param_list, usage_description, usage_parameters};
    UINT32 i;
    kmgr_response_t response;

    memset(&events_q, 0, sizeof(events_q));

    // init env based on env file
    if(env_fname) {
        if((fd = fopen(env_fname, "rb")) != NULL) {
            while(fgets(line, sizeof(line), fd) != NULL) {
                char *cmnt = strstr(line, "//");
                if(cmnt) {
                    *cmnt = 0;
                }
                INT32 len = strlen(line);
                while(len) {
                    if(strchr(" \t\r\n", line[len])) {
                        line[len--] = 0;
                    } else {
                        break;
                    }
                }
                char *value = strchr(line, ':');
                if(!value) {
                    continue;
                }
                *value = 0;
                value++;
                while(*value && strchr(" \t", *value)) {
                    value++;
                }
                INT32 i, idx = -1;
                for(i=0; i < (INT32)(sizeof(env_items)/sizeof(env_items[0])); i++) {
                    if(strcmp(env_items[i].name, line) == 0) {
                        idx = i;
                        break;
                    }
                }
                if(idx >= 0) {
                    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "env_item %s <- %s", line, value);
                    if(snprintf(env_items[idx].value, env_items[idx].size, "%s", value) >= (int)env_items[idx].size)
                        klog(KLOG_SUBJECT_KMGR, LL_WARNG, "kmgr_init: configuration value truncated; key=%s capacity=%u", line, env_items[idx].size);
                } else {
                    klog(KLOG_SUBJECT_KMGR, LL_WARNG, "kmgr_init: unknown configuration key=%s", line);
                }

            }
            if(ferror(fd)) klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kmgr_init: failed to read configuration '%s': %s", env_fname, strerror(errno));
            if(fclose(fd) != 0) klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kmgr_init: failed to close configuration '%s': %s", env_fname, strerror(errno));
        } else {
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "kmgr_init: failed to open configuration '%s': %s; using defaults", env_fname, strerror(errno));
        }
    }

    // init semaphores
    if(sem_init(&kmgr_wait_sem, 0, 0) != 0) {
        klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kmgr_init: cannot initialize manager semaphore: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(i=0; i<sizeof(kmgr_clients)/sizeof(kmgr_clients[0]); i++) {
        if(sem_init(&kmgr_clients[i].wait_sem, 0, 0) != 0) {
            klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kmgr_init: cannot initialize client=%u semaphore: %s", i, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // create poller thread for polling logs (and modchange)
    pthread_attr_t plr_thread_attributes;
    pthread_attr_init(&plr_thread_attributes);
    pthread_attr_setdetachstate(&plr_thread_attributes, PTHREAD_CREATE_DETACHED);
    int thread_result = pthread_create(&kplr_thread, &plr_thread_attributes, &kmgr_thread_poller, NULL);
    pthread_attr_destroy(&plr_thread_attributes);
    if(thread_result != 0) {
        klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "kmgr_init: failed to start poller thread: %s", strerror(thread_result));
        exit(EXIT_FAILURE);
    }

    // populate help db with local commands
    memset(&help_db, 0, sizeof(help_db));
    for(i=0; i<n_lcmds; i++) {
        cmd_help_t *cmd = &help_db.cmds[help_db.local_cmds_base + help_db.n_local_cmds];
        char cmd_name[128];
        snprintf(cmd_name, sizeof(cmd_name), "%s", lcmds[i].name);
        if(add_to_help_db(cmd_name, &cmd->name) < 0) continue; // add command
        lcmds[i].lcmd_f_cb(0, NULL, 0, NULL, &response, lusage);
        if(add_to_help_db(usage_param_list, &cmd->param_list) < 0) continue;
        if(add_to_help_db(usage_description, &cmd->description) < 0) continue;
        if(add_to_help_db(usage_parameters, &cmd->parameters) < 0) continue;
        help_db.n_local_cmds++;
    }
    help_db.k60s_cmds_base = help_db.local_cmds_base + help_db.n_local_cmds;
    help_db.k60s_pos_base = help_db.pos;


    wdr85100_com_init(); // initialize broadcoms driver communication layer
}

void *kmgr_thread_main(void* arg) {
    UINT32 i, length;
    UINT8 cmd[256];
    kmgr_response_t lresponse;
    char rndm[32];
    char last_expected_rndm[32];
    T_BOOL first_hc = E_FALSE;
    (void)arg;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rx_event_mutex, &attr);

    pthread_mutexattr_t issue_command_attr;
    pthread_mutexattr_init(&issue_command_attr);
    pthread_mutexattr_settype(&issue_command_attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&kmgr_issue_command_mutex, &issue_command_attr);

    static time_t curtime;
    static time_t last_hc_time;
    curtime = time(NULL);

#if 0
    pthread_t me = pthread_self();
    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "kmgr: me is 0x%llX", (unsigned long long)me);
    pthread_detach(me);
#endif

    // 2do - wait for serial eeprom files (rfd)
    // 2do - reset k60s and cause bootstrap sequence

    // at this point kmgr is responsible for 3 things:
    // accepting events from k60s and handling them
    // accepting commands from users and execute them
    // health-check and restarting of k60s
    while (1) {

        switch(wait_sem_seconds(&kmgr_wait_sem, 3)) {
        case 0:
            //klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "process work");
            for(i=0; i<sizeof(kmgr_clients)/sizeof(kmgr_clients[0]); i++) {
                if(kmgr_clients[i].req_buff) {
                    issue_command(kmgr_clients[i].req_buff, kmgr_clients[i].req_length, &kmgr_clients[i].response);
                    kmgr_clients[i].req_length = 0;
                    kmgr_clients[i].req_buff = 0;
                    sem_post(&kmgr_clients[i].wait_sem);
                }
            }
            for( ;events_q.rd != events_q.wr; events_q.rd = events_q.rd + 1 >= (UINT16)sizeof(events_q.q) ? 0 : events_q.rd + 1) {
                event_handler(events_q.q[events_q.rd]);
            }

            curtime = time(NULL);
            if (curtime - last_hc_time  < 3 && first_hc)
            {
                break;
            }
        case 1:
            if(ENV_TRUE(env_healthchk)) {
                first_hc = E_TRUE;
                snprintf(rndm, sizeof(rndm), "%.8X", rand());
                length = sprintf((char*)cmd, "echo %s\r\n", rndm);
                klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "issue health check with random %s", rndm);
                issue_command((char*)cmd, length, &lresponse);
                if((strcmp(lresponse.status, "OK") == 0) && lresponse.body && (strcmp(lresponse.body, rndm) == 0)) {
                    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "health-check succeeded");
                    k60s_consecutive_failures = 0;
                } else {
                    klog(KLOG_SUBJECT_KMGR, LL_ERROR, "health-check failed. status %s, expected: %s, actual: %s",
                            lresponse.status, rndm, lresponse.body);
                    if(k60s_consecutive_failures < 100) {
                        ++k60s_consecutive_failures;
                    }
                    if( ( lresponse.body == NULL || strcmp(lresponse.body, "") == 0) ) {
                        strcpy(last_expected_rndm, rndm);
                    }
                    else if ( strcmp(lresponse.body, last_expected_rndm) == 0 ){
                        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "health-check response matches previous value. expected: %s. previous: %s. actual: %s", rndm, last_expected_rndm, lresponse.body);
                        k60s_consecutive_failures = 0;
                    }
                }

                last_hc_time = time(NULL);
            }
            break;
        default:
            klog(KLOG_SUBJECT_KMGR, LL_ERROR, "semaphore problem");
            usleep(1000);
            break;
        }
        if(k60s_consecutive_failures >= 5) {
            //in case of consecutive failures
            klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "k60s is non-responsive -  - exit modemd process and restart via watchdog");


#ifdef _OCTEON
            system("echo \"k60s is non-responsive - exit modemd process and restart via watchdog\"   > /dev/ttyS1");
            LOG(LOG_ERR,   "k60s is non-responsive - exit modemd process and restart via watchdog");
            LOG(LOG_CRIT, "k60s is non-responsive - exit modemd process and restart via watchdog");
#endif //  _OCTEON

            sync();

            exit(1);
        }
    }
    return NULL;
}

