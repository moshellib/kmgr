#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "loader/sysdep.h"
#include "loader/hc08sprg.h"
#include "loader/serial.h"
#include "gl_types.h"
#include "kdefs.h"

static char last_log[512];
static int last_level;
static int write_failure;
static int read_result;

void klog(int subject, int level, char *format, ...)
{
    va_list arguments;
    (void)subject;
    assert(format != NULL);
    last_level = level;
    va_start(arguments, format);
    vsnprintf(last_log, sizeof(last_log), format, arguments);
    va_end(arguments);
}

int wb(const void *data, unsigned length)
{
    (void)data;
    errno = EIO;
    return write_failure ? -1 : (int)length;
}

int rb(void *data, unsigned length)
{
    (void)data;
    (void)length;
    errno = EIO;
    return read_result;
}

static void fixture(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    assert(fputs(content, file) != EOF);
    assert(fclose(file) == 0);
    memset(&image, 0, sizeof(image));
}

int main(void)
{
    char path[] = "/tmp/kmgr-protocol-test-XXXXXX";
    int descriptor = mkstemp(path);
    assert(descriptor >= 0 && close(descriptor) == 0);
    fixture(path, "S306000040800138\nS70500000000FA\n");
    assert(read_s19(path, &image) == 0);
    assert(image.d[0x4080] == 1 && image.f[0x4080] == 1);
    fixture(path, "S306000040800139\n");
    assert(read_s19(path, &image) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "checksum"));
    fixture(path, "S30000004080\n");
    assert(read_s19(path, &image) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "length"));
    fixture(path, "not firmware\n");
    assert(read_s19(path, &image) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "no valid data"));
    assert(unlink(path) == 0);
    assert(read_s19(path, &image) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "open"));

    write_failure = 1;
    assert(unhook() == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "unhook"));
    write_failure = 0;
    read_result = 0;
    assert(read_mcu_info() == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "read_mcu_info"));
    assert(unhook() == 0);
    puts("Bootloader protocol return-path tests passed");
    return 0;
}