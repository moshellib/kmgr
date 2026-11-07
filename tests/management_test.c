#include <assert.h>
#include "../kmgr.c"
#include "cmnstruct_api.h"

static char last_log[512];
static int last_level;
static int log_count;
static kmgr_response_t mock_response;
static int missing_response;

static void capture_log(char *subject, int level, char *name, char *content)
{
    (void)subject;
    (void)name;
    last_level = level;
    ++log_count;
    snprintf(last_log, sizeof(last_log), "%s", content);
}

kmgr_response_t *__wrap_kmgr_issue_command(int client, char *request, int length)
{
    (void)client;
    (void)request;
    (void)length;
    return missing_response ? NULL : &mock_response;
}

int main(void)
{
    sem_t semaphore;
    int count;
    char *stored;
    char request[] = "echo\r\n";
    cmnstrct_log_level_in_t input = {0};
    cmnstrct_system_info_get_out_t output;
    cmnstrct_system_info_get_out_t mock_body = {0};
    klog_set_cb(capture_log);
    assert(sem_init(&semaphore, 0, 0) == 0);
    count = log_count;
    assert(wait_sem_seconds(&semaphore, 0) == 1 && log_count == count);
    assert(sem_post(&semaphore) == 0);
    assert(wait_sem_seconds(&semaphore, 0) == 0);
    assert(wait_sem_seconds(&semaphore, -1) == -2);
    assert(last_level == LL_ERROR && strstr(last_log, "invalid semaphore"));
    assert(wait_sem_seconds(NULL, 0) == -2);
    assert(sem_destroy(&semaphore) == 0);
    memset(&help_db, 0, sizeof(help_db));
    assert(add_to_help_db("command", &stored) == 0 && strcmp(stored, "command") == 0);
    help_db.pos = sizeof(help_db.buff) - 1;
    assert(add_to_help_db("command", &stored) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "exceeds storage"));
    help_db.pos = sizeof(help_db.buff);
    assert(add_to_help_db("command", &stored) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "exhausted"));
    assert(kmgr_issue_command(-1, request, strlen(request)) == NULL);
    assert(last_level == LL_ERROR && strstr(last_log, "invalid client"));
    assert(logLevel_set(0, NULL) == -1);
    assert(last_level == LL_ERROR && strstr(last_log, "invalid management buffers"));
    missing_response = 1;
    assert(logLevel_set(0, &input) == -2);
    assert(last_level == LL_ERROR && strstr(last_log, "no management response"));
    missing_response = 0;
    mock_response.status = "FAIL";
    assert(logLevel_set(0, &input) == -2);
    assert(last_level == LL_ERROR && strstr(last_log, "status=FAIL"));
    mock_response.status = "OK";
    assert(logLevel_set(0, &input) == 0);
    assert(rf_system_info_get(0, &output) == -2);
    assert(last_level == LL_ERROR && strstr(last_log, "invalid management response body"));
    mock_response.body = (char *)&mock_body;
    mock_response.body_length = sizeof(output);
    assert(rf_system_info_get(0, &output) == 0);
    ++mock_response.body_length;
    assert(rf_system_info_get(0, &output) == -2);
    count = log_count;
    klog(-1, LL_ERROR, "invalid subject");
    klog(KLOG_SUBJECT_KMGR, 99, "invalid level");
    klog(KLOG_SUBJECT_KMGR, LL_ERROR, NULL);
    assert(log_count == count);
    assert(klog_name_to_level("debug") == LL_DEBUG);
    assert(klog_name_to_subj("kmgr") == KLOG_SUBJECT_KMGR);
    puts("Management return-path tests passed");
    return 0;
}