#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "brcm/diagnostics.h"
#include "brcm/hostDriver.h"

static int log_count;
static char last_log[512];
static int relay_result;
static int partial_write;

int relay_read(void *data, int length)
{
    (void)data;
    (void)length;
    return relay_result;
}

int relay_write(void *data, int length)
{
    (void)data;
    return relay_result < 0 ? relay_result : (partial_write ? length - 1 : length);
}

void klog(int subject, int level, char *format, ...)
{
    va_list arguments;
    (void)subject;
    assert((level == LL_ERROR || level == LL_WARNG) && format != NULL);
    ++log_count;
    va_start(arguments, format);
    vsnprintf(last_log, sizeof(last_log), format, arguments);
    va_end(arguments);
}

int main(void)
{
    BYTE buffer[4] = {0};
    UINT32 received = 99;
    assert(BCM85100_LOG_STATUS(BCM85100_SUCCESS_MSG_E) == BCM85100_SUCCESS_MSG_E);
    assert(log_count == 0);
    assert(BCM85100_LOG_STATUS(BCM85100_INVALID_DEVICE_INDEX_E) == BCM85100_INVALID_DEVICE_INDEX_E);
    assert(log_count == 1 && strstr(last_log, "main:") && strstr(last_log, "rc=19"));
    assert(BCM85100_API_ComTimeoutSet(BCM85100_DEVICES_NUM, 1) == BCM85100_INVALID_DEVICE_INDEX_E);
    assert(log_count == 2 && strstr(last_log, "ComTimeoutSet"));
    bcm85100HostUartDataSend(0, NULL, 1);
    assert(strstr(last_log, "missing buffer"));
    relay_result = -1;
    bcm85100HostUartDataSend(0, buffer, sizeof(buffer));
    assert(strstr(last_log, "relay send failed"));
    bcm85100HostUartDataReceive(0, buffer, sizeof(buffer), 1, &received);
    assert(received == 0 && strstr(last_log, "relay read failed"));
    relay_result = 0;
    partial_write = 1;
    bcm85100HostUartDataSend(0, buffer, sizeof(buffer));
    assert(strstr(last_log, "relay send failed"));
    puts("Broadcom return-path tests passed");
    return 0;
}