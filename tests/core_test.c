#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "gl_types.h"
#include "kdefs.h"
#include "cmnutil.h"
#include "framer.h"
#include "seeprom.h"

int open_listener(int port, int *descriptor);
void *servlet_entry(void *argument);

char env_tty[64] = "/proc/self/kmgr-missing";
char env_baud[64] = "115200";
static kmgr_response_t socket_response;
static int command_count;
static char last_failure[512];

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
    if(level <= LL_WARNG) snprintf(last_failure, sizeof(last_failure), "%s", last_log);
}

kmgr_response_t *kmgr_issue_command(int client, char *request, int length)
{
    (void)client;
    (void)request;
    (void)length;
    ++command_count;
    return &socket_response;
}

static int descriptor_count(void)
{
    DIR *directory = opendir("/proc/self/fd");
    struct dirent *entry;
    int count = 0;
    assert(directory != NULL);
    while((entry = readdir(directory)) != NULL)
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            ++count;
    assert(closedir(directory) == 0);
    return count;
}

static void test_listener(void)
{
    int descriptor;
    int rejected;
    int count;
    struct sockaddr_in address;
    socklen_t address_size = sizeof(address);

    assert(open_listener(-1, &descriptor) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "Invalid listener arguments"));
    assert(open_listener(0, NULL) == -1);
    assert(open_listener(65536, &descriptor) == -1);
    assert(open_listener(0, &descriptor) == 0);
    assert(getsockname(descriptor, (struct sockaddr *)&address, &address_size) == 0);
    count = descriptor_count();
    assert(open_listener(ntohs(address.sin_port), &rejected) == -1);
    assert(rejected == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "Failed to bind listener"));
    assert(descriptor_count() == count);
    assert(close(descriptor) == 0);
}

static void test_arguments(void)
{
    char command[] = "first second";
    char *arguments[2];
    UINT32 count = 99;
    assert(make_args(command, strlen(command), 1, &count, arguments) == -1);
    assert(count == 0 && last_level == LL_WARNG);
    assert(make_args(NULL, 0, 2, &count, arguments) == -1);
    assert(last_level == LL_ERROR);
    memcpy(command, "first second", sizeof(command));
    assert(make_args(command, strlen(command), 2, &count, arguments) == 0);
    assert(count == 2 && strcmp(arguments[1], "second") == 0);
}

static void test_framing(void)
{
    framer_ctx_t transmit;
    framer_ctx_t receive;
    csm_tx_t tx_checksum[1];
    csm_rx_t rx_checksum[1];
    UINT8 input[] = {'a', 27, 'b'};
    UINT8 encoded[32];
    UINT8 decoded[16];
    UINT8 flags;
    UINT8 channel;
    UINT32 encoded_size;
    UINT32 decoded_size;
    framer_ctx_init(&transmit, 1, 1, 32, 27, tx_checksum, NULL);
    framer_ctx_init(&receive, 1, 1, 32, 27, NULL, rx_checksum);
    assert(framer_tx_convert(&transmit, PDU_FLAG_START | PDU_FLAG_END | PDU_FLAG_CSM_PASS, input, sizeof(input), 0, &encoded_size, encoded, sizeof(encoded)) == sizeof(input));
    assert(framer_rx_convert(&receive, encoded, encoded_size, &flags, &channel, &decoded_size, decoded, sizeof(decoded)) == encoded_size);
    assert(decoded_size == sizeof(input) && memcmp(decoded, input, sizeof(input)) == 0);
    assert(flags & PDU_FLAG_CSM_PASS);
    encoded[encoded_size - 1] ^= 1;
    framer_ctx_reset(&receive);
    assert(framer_rx_convert(&receive, encoded, encoded_size, &flags, &channel, &decoded_size, decoded, sizeof(decoded)) == encoded_size);
    assert(flags & PDU_FLAG_CSM_FAIL);
    framer_ctx_init(&receive, 1, 1, 32, 27, NULL, NULL);
    assert(framer_rx_convert(&receive, encoded, encoded_size, &flags, &channel, &decoded_size, decoded, sizeof(decoded)) == encoded_size);
    assert(decoded_size == sizeof(input));
    assert(framer_tx_convert(&transmit, PDU_FLAG_START, input, sizeof(input), 1, &encoded_size, encoded, sizeof(encoded)) == 0);
    assert(encoded_size == 0 && last_level == LL_ERROR);
    assert(framer_tx_convert_single(&transmit, PDU_FLAG_END | PDU_FLAG_CSM_PASS, input, 0, 0, &encoded_size, encoded, 4) == 0);
    assert(encoded_size == 0 && last_level == LL_WARNG);
}

static void test_io_boundaries(void)
{
    UINT8 data[1] = {0};
    assert(kio_read((UINT32)-1, data, 1) == 0);
    assert(last_level == LL_ERROR);
    assert(kio_write((UINT32)-1, data, 1, 1, 0) == 0);
    assert(last_level == LL_ERROR);
    assert(kio_commit((UINT32)-1) == (UINT32)-1);
    assert(last_level == LL_ERROR);
    assert(kio_read(0, data, 1) == 0);
    assert(kio_write(0, data, 1, 1, 0) == 1);
}

static void test_calibration(void)
{
    char output[2048];
    char malformed[] = "local inventory = { name = \"unfinished";
    char long_token[1100];
    char empty[] = "";
    memset(output, 0x5a, sizeof(output));
    assert(make_ipf_by_buff(malformed, strlen(malformed), output, sizeof(output)) == 0);
    assert(last_level == LL_ERROR && strstr(last_log, "unterminated string"));
    memset(long_token, 'x', sizeof(long_token));
    assert(make_ipf_by_buff(long_token, sizeof(long_token), output, sizeof(output)) == 0);
    assert(last_level == LL_ERROR && strstr(last_log, "token exceeds"));
    assert(make_ipf_by_buff(empty, 0, output, 1) == 0);
    assert((unsigned char)output[0] == 0x5a);
    assert(last_level == LL_ERROR && strstr(last_log, "output buffer insufficient"));
    assert(make_ipf_by_buff(NULL, 1, output, sizeof(output)) == 0);
    assert(last_level == LL_ERROR);
    assert(make_ipf_by_files("/proc/self/kmgr-missing", NULL, output, sizeof(output)) == 0);
    assert(last_level == LL_ERROR && strstr(last_log, "failed to open"));
    assert(make_ipf_by_buff(empty, 0, output, sizeof(output)) == 8);
}

static void socket_request(const char *request, size_t length, int expected_commands)
{
    int sockets[2];
    int *argument = malloc(sizeof(*argument));
    pthread_t thread;
    assert(argument != NULL);
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    *argument = sockets[1];
    command_count = 0;
    last_failure[0] = 0;
    socket_response.status = "OK";
    assert(pthread_create(&thread, NULL, servlet_entry, argument) == 0);
    assert(write(sockets[0], request, length) == (ssize_t)length);
    assert(shutdown(sockets[0], SHUT_WR) == 0);
    assert(pthread_join(thread, NULL) == 0);
    assert(command_count == expected_commands);
    assert(close(sockets[0]) == 0);
}

static void test_socket_requests(void)
{
    char oversized[REQ_LINE_MAX_SIZE + 4];
    socket_request("echo\r\n", 6, 1);
    assert(last_failure[0] == 0);
    socket_request("echo 3+++\r\nabc\r\n", 16, 1);
    assert(last_failure[0] == 0);
    socket_request("echo -1+++\r\n", 12, 0);
    assert(strstr(last_failure, "invalid request body length"));
    socket_request("echo 3+++\r\nx", 12, 0);
    assert(strstr(last_failure, "incomplete request body"));
    memset(oversized, 'x', sizeof(oversized));
    oversized[sizeof(oversized) - 1] = '\n';
    socket_request(oversized, sizeof(oversized), 0);
    assert(strstr(last_failure, "request line exceeds"));
}

static void test_serial_thread_failures(void)
{
    assert(kio_thread_main(NULL) == NULL);
    assert(last_level == LL_CRTCL && strstr(last_log, "failed to open"));
    strcpy(env_tty, "/dev/null");
    assert(kio_thread_main(NULL) == NULL);
    assert(last_level == LL_CRTCL && strstr(last_log, "failed to configure"));
}

int main(void)
{
    test_listener();
    test_arguments();
    test_framing();
    test_io_boundaries();
    test_calibration();
    test_socket_requests();
    test_serial_thread_failures();
    puts("Core return-path tests passed");
    return 0;
}