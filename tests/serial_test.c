#include <assert.h>
#include <dirent.h>
#include <pty.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gl_types.h"
#include "kdefs.h"
#include "loader/serial.h"

static char last_log[512];
static int last_level;

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

static int descriptor_count(void)
{
    DIR *directory = opendir("/proc/self/fd");
    struct dirent *entry;
    int count = 0;
    assert(directory != NULL);
    while((entry = readdir(directory)) != NULL)
        if(entry->d_name[0] != '.') ++count;
    assert(closedir(directory) == 0);
    return count;
}

int main(void)
{
    int master;
    int slave;
    int before;
    char device[128];
    char argument[132];
    char received[3];
    char invalid[] = "/proc/self/kmgr-missing:d";
    char not_terminal[] = "/dev/null:d";
    assert(openpty(&master, &slave, device, NULL, NULL) == 0);
    before = descriptor_count();
    assert(init_uart(invalid, "115200") < 0);
    assert(last_level == LL_ERROR && strstr(last_log, "failed to open"));
    assert(descriptor_count() == before);
    assert(init_uart(not_terminal, "115200") < 0);
    assert(last_level == LL_ERROR && strstr(last_log, "get settings"));
    assert(descriptor_count() == before);
    snprintf(argument, sizeof(argument), "%s:d", device);
    assert(init_uart(argument, "invalid") < 0);
    assert(last_level == LL_ERROR && strstr(last_log, "unsupported baud"));
    assert(descriptor_count() == before);
    snprintf(argument, sizeof(argument), "%s:d", device);
    assert(init_uart(argument, "115200") == 0);
    assert(wb("abc", 3) == 3);
    assert(read(master, received, sizeof(received)) == sizeof(received));
    assert(memcmp(received, "abc", sizeof(received)) == 0);
    assert(write(master, "def", 3) == 3);
    assert(rb(received, sizeof(received)) == sizeof(received));
    assert(memcmp(received, "def", sizeof(received)) == 0);
    assert(close_uart() == 0);
    assert(descriptor_count() == before);
    assert(wb("abc", 3) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "serial write failed"));
    assert(rb(received, sizeof(received)) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "serial read failed"));
    assert(close_uart() == 0);
    assert(close(master) == 0 && close(slave) == 0);
    puts("Serial return-path tests passed");
    return 0;
}