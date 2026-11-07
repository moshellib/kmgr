#ifndef BCM85100_DIAGNOSTICS_H
#define BCM85100_DIAGNOSTICS_H

#include "bcm85100_api.h"
#include "../kdefs.h"

static inline BCM85100_ERROR_MSG_ENUM bcm85100LogStatus(BCM85100_ERROR_MSG_ENUM status, const char *function, int line)
{
    if(status != BCM85100_SUCCESS_MSG_E)
        klog(KLOG_SUBJECT_KMGR, LL_ERROR, "%s: Broadcom operation failed; rc=%d source_line=%d", function, status, line);
    return status;
}

#define BCM85100_LOG_STATUS(status) bcm85100LogStatus((status), __func__, __LINE__)

#endif