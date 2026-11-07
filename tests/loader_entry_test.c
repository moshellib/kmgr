#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include "loader/sysdep.h"
#include "loader/hc08sprg.h"
#include "loader/serial.h"
#include "loader/sikluK60Image.h"
#include "kdefs.h"

int loader_entry(char *device, char *speed, char *file, int force_burn);
int SikluConvert(char *input, char *output, UINT32 run_address, T_SIKLU_K60_HEADER *header);
int load_siklu_image(char *file);
int SikluGetRemoteVersion(char *version, int capacity);
int orig_main(int argc, char **argv);

BOARD_MEM image;
IDENT_DATA ident;
int bl_version;
t_sdid sdidprops[] = {{.sdid = SDID_UNDEF}};

static char last_log[512];
static char converted_path[256];
static const unsigned int image_limit = 512 * 1024;
static int last_level;
static int parser_result;
static unsigned int firmware_start = 0x4080;
static unsigned int firmware_length = 8;
static int setup_calls;
static int setup_failure_call;
static int uart_result = 1;
static int uart_closes;
static int mcu_result;
static int program_result;
static int handoff_result;
static int write_result = 1;
static int read_failure;
static const char *response = "old-version\r\n";
static size_t response_position;

void klog(int subject, int level, char *format, ...)
{
	va_list arguments;
	va_list path_arguments;
	(void)subject;
	assert(format != NULL);
	last_level = level;
	va_start(arguments, format);
	va_copy(path_arguments, arguments);
	vsnprintf(last_log, sizeof(last_log), format, arguments);
	if(strcmp(format, "Firmware converted: input=%s output=%s address=0x%X bytes=%u") == 0)
	{
		(void)va_arg(path_arguments, char *);
		snprintf(converted_path, sizeof(converted_path), "%s", va_arg(path_arguments, char *));
	}
	va_end(path_arguments);
	va_end(arguments);
}

int read_s19(char *file, BOARD_MEM *memory)
{
	(void)file;
	memset(memory, 0, sizeof(*memory));
	memset(&memory->d[firmware_start], 0x5a, firmware_length);
	memset(&memory->f[firmware_start], 1, firmware_length);
	return parser_result;
}

int setup_vect_tbl(unsigned source, unsigned destination)
{
	(void)source;
	(void)destination;
	return ++setup_calls == setup_failure_call ? -1 : 0;
}

void CRC_AddByteArray(unsigned short *crc, unsigned char *data, int size)
{
	(void)crc;
	(void)data;
	(void)size;
}

void CRC_ResetCRC(unsigned short *crc, unsigned short seed)
{
	*crc = seed;
}

int init_uart(char *device, char *speed)
{
	(void)device;
	(void)speed;
	return uart_result;
}

int close_uart(void)
{
	++uart_closes;
	return 0;
}
int read_mcu_info(void) { return mcu_result; }
int check_image(void) { return 0; }
int prg_mem(void) { return program_result; }
int unhook(void) { return handoff_result; }

int wb(const void *data, unsigned length)
{
	(void)data;
	(void)length;
	errno = EIO;
	return write_result;
}

int rb(void *data, unsigned length)
{
	(void)length;
	if(read_failure)
	{
		errno = EIO;
		return -1;
	}
	if(response[response_position] == 0)
		return 0;
	*(char *)data = response[response_position++];
	return 1;
}

static void reset_mocks(void)
{
	last_log[0] = 0;
	converted_path[0] = 0;
	parser_result = 0;
	firmware_start = 0x4080;
	firmware_length = 8;
	setup_calls = 0;
	setup_failure_call = 0;
	uart_result = 1;
	uart_closes = 0;
	mcu_result = 0;
	program_result = 0;
	handoff_result = 0;
	write_result = 1;
	read_failure = 0;
	response = "old-version\r\n";
	response_position = 0;
}

static void expect_log(int level, const char *message)
{
	assert(last_level == level);
	assert(strstr(last_log, message) != NULL);
}

static void write_fixture(const char *path, UINT32 address, UINT32 data_length, size_t file_length)
{
	unsigned char data[sizeof(T_SIKLU_K60_HEADER) + 12] = {0};
	T_SIKLU_K60_HEADER header = {0};
	FILE *file = fopen(path, "wb");
	assert(file != NULL);
	header.header_version = FE32(1);
	header.burn_address = FE32(address);
	header.data_length = FE32(data_length);
	memcpy(data, &header, sizeof(header));
	assert(file_length <= sizeof(data));
	assert(fwrite(data, 1, file_length, file) == file_length);
	assert(fclose(file) == 0);
}

static void test_files(void)
{
	char path[] = "/tmp/kmgr-loader-test-XXXXXX";
	T_SIKLU_K60_HEADER header;
	int descriptor = mkstemp(path);
	assert(descriptor >= 0);
	assert(close(descriptor) == 0);

	reset_mocks();
	assert(SikluConvert("fixture", path, 0x4080, &header) == 0);
	expect_log(LL_DEBUG, "Firmware converted");
	assert(load_siklu_image(path) == 0);
	expect_log(LL_DEBUG, "Converted firmware loaded");

	assert(SikluConvert("fixture", "/dev/full", 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Failed to finalize converted firmware");
	firmware_length = 8192;
	assert(SikluConvert("fixture", "/dev/full", 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Failed to write converted firmware");
	firmware_length = 8;
	parser_result = -1;
	assert(SikluConvert("fixture", path, 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Failed to read firmware");
	reset_mocks();
	setup_failure_call = 1;
	assert(SikluConvert("fixture", path, 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Failed to prepare vector table");
	reset_mocks();
	firmware_length = 0;
	assert(SikluConvert("fixture", path, 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Invalid firmware range");
	firmware_length = 8;
	firmware_start = 0;
	assert(SikluConvert("fixture", path, 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Invalid firmware range");
	firmware_start = 0x4080;
	firmware_length = image_limit;
	assert(SikluConvert("fixture", path, 0x4080, &header) == -1);
	expect_log(LL_ERROR, "Invalid firmware range");

	write_fixture(path, 0, 8, 0);
	assert(load_siklu_image(path) == -1);
	expect_log(LL_ERROR, "Invalid converted firmware size");
	write_fixture(path, 0, 9, sizeof(header) + 12);
	assert(load_siklu_image(path) == -1);
	expect_log(LL_ERROR, "Invalid or truncated converted firmware");
	write_fixture(path, MAX_ADDRESS - 1, 8, sizeof(header) + 12);
	assert(load_siklu_image(path) == -1);
	expect_log(LL_ERROR, "exceeds image memory");
	descriptor = open(path, O_WRONLY);
	assert(descriptor >= 0);
	assert(ftruncate(descriptor, image_limit + 1) == 0);
	assert(close(descriptor) == 0);
	assert(load_siklu_image(path) == -1);
	expect_log(LL_ERROR, "Invalid converted firmware size");
	assert(load_siklu_image("/tmp") == -1);
	expect_log(LL_ERROR, "Failed to read converted firmware");
	assert(unlink(path) == 0);
	assert(load_siklu_image(path) == -1);
	expect_log(LL_ERROR, "Failed to open converted firmware");
}

static void test_versions(void)
{
	char version[8];
	reset_mocks();
	response = "12345\r\n";
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 0);
	assert(strcmp(version, "12345") == 0);
	response = "1234567\n";
	response_position = 0;
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 0);
	assert(strcmp(version, "1234567") == 0);
	response = "12345678\n";
	response_position = 0;
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 1);
	assert(version[sizeof(version) - 1] == 0);
	expect_log(LL_WARNG, "exceeded buffer capacity");
	reset_mocks();
	response = "abc";
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 1);
	assert(strcmp(version, "abc") == 0);
	expect_log(LL_WARNG, "Timed out");
	read_failure = 1;
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 1);
	assert(version[0] == 0);
	expect_log(LL_ERROR, "Failed to read MCU version");
	write_result = -1;
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 1);
	expect_log(LL_ERROR, "Failed to write MCU version");
	write_result = 0;
	assert(SikluGetRemoteVersion(version, sizeof(version)) == 1);
	expect_log(LL_ERROR, "Incomplete MCU version request");
	assert(SikluGetRemoteVersion(NULL, 0) == 1);
	expect_log(LL_ERROR, "Invalid MCU version buffer");
}

static void test_loader_returns(void)
{
	T_SIKLU_K60_HEADER header;
	char matching_response[sizeof(header.sw_version) + 2];
	reset_mocks();
	uart_result = -1;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 1);
	expect_log(LL_ERROR, "Serial initialization failed");
	assert(uart_closes == 0);
	reset_mocks();
	parser_result = -1;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == -1);
	assert(uart_closes == 1);
	reset_mocks();
	mcu_result = 7;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 5);
	expect_log(LL_ERROR, "rc=7");
	assert(uart_closes == 1);
	assert(converted_path[0] != 0 && access(converted_path, F_OK) == -1);
	reset_mocks();
	setup_failure_call = 2;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 6);
	expect_log(LL_ERROR, "Vector-table setup failed");
	assert(uart_closes == 1);
	reset_mocks();
	program_result = -1;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 8);
	expect_log(LL_ERROR, "Firmware programming failed");
	assert(uart_closes == 1);
	reset_mocks();
	handoff_result = -1;
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 8);
	expect_log(LL_ERROR, "Failed to exit programming mode");
	assert(uart_closes == 1);
	reset_mocks();
	assert(loader_entry("mock-tty", "115200", "fixture", 1) == 0);
	expect_log(LL_INFOR, "action=programmed");
	assert(uart_closes == 1);
	assert(converted_path[0] != 0 && access(converted_path, F_OK) == -1);
	reset_mocks();
	assert(SikluConvert("fixture", "/dev/null", 0x4080, &header) == 0);
	snprintf(matching_response, sizeof(matching_response), "%s\n", header.sw_version);
	response = matching_response;
	program_result = -1;
	handoff_result = -1;
	assert(loader_entry("mock-tty", "115200", "fixture", 0) == 0);
	expect_log(LL_INFOR, "action=reused");
	assert(uart_closes == 1);
}

static void test_legacy_returns(void)
{
	char executable[] = "loader";
	char device[] = "mock-tty";
	char firmware[] = "fixture";
	char *arguments[] = {executable, device, firmware, NULL};
	reset_mocks();
	assert(orig_main(1, arguments) == 1);
	expect_log(LL_ERROR, "Invalid loader arguments");
	uart_result = -1;
	assert(orig_main(3, arguments) == 1);
	expect_log(LL_ERROR, "Serial initialization failed");
	reset_mocks();
	parser_result = -1;
	assert(orig_main(3, arguments) == -1);
	assert(uart_closes == 1);
	reset_mocks();
	mcu_result = 7;
	assert(orig_main(3, arguments) == 5);
	expect_log(LL_ERROR, "rc=7");
	assert(uart_closes == 1);
	reset_mocks();
	setup_failure_call = 2;
	assert(orig_main(3, arguments) == 6);
	expect_log(LL_ERROR, "Vector-table setup failed");
	assert(uart_closes == 1);
	reset_mocks();
	program_result = -1;
	assert(orig_main(3, arguments) == 8);
	expect_log(LL_ERROR, "Firmware programming failed");
	assert(uart_closes == 1);
	reset_mocks();
	handoff_result = -1;
	assert(orig_main(3, arguments) == 8);
	expect_log(LL_ERROR, "Failed to exit programming mode");
	assert(uart_closes == 1);
}

int main(void)
{
	test_files();
	test_versions();
	test_loader_returns();
	test_legacy_returns();
	puts("Loader return-path tests passed");
	return 0;
}