/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2015 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Mellanox Technologies, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <pmix.h>

int main(int argc, char **argv)
{
    pmix_proc_t myproc;
    int rc;
    pmix_value_t value;
    pmix_value_t *val = &value;
    char *tmp;
    pmix_proc_t proc;
    uint32_t nprocs, n;
    pmix_info_t *info;
    bool flag;

    /* init us */
    if (PMIX_SUCCESS != (rc = PMIx_Init(&myproc))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Init failed: %d\n", myproc.nspace, myproc.rank, rc);
        exit(0);
    }
    fprintf(stderr, "Client ns %s rank %d: Running\n", myproc.nspace, myproc.rank);

    /* get our universe size */
    if (PMIX_SUCCESS != (rc = PMIx_Get(&myproc, PMIX_UNIV_SIZE, NULL, 0, &val))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Get universe size failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }
    nprocs = val->data.uint32;
    PMIX_VALUE_RELEASE(val);
    fprintf(stderr, "Client %s:%d universe size %d\n", myproc.nspace, myproc.rank, nprocs);

    /* put a few values */
    if (0 > asprintf(&tmp, "%s-%d-internal", myproc.nspace, myproc.rank)) {
        exit(1);
    }
    value.type = PMIX_UINT32;
    value.data.uint32 = 1234;
    if (PMIX_SUCCESS != (rc = PMIx_Store_internal(&myproc, tmp, &value))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Store_internal failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }
    free(tmp);

    if (0 > asprintf(&tmp, "%s-%d-local", myproc.nspace, myproc.rank)) {
        exit(1);
    }
    value.type = PMIX_UINT64;
    value.data.uint64 = 1234;
    if (PMIX_SUCCESS != (rc = PMIx_Put(PMIX_LOCAL, tmp, &value))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Put internal failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }
    free(tmp);

    if (0 > asprintf(&tmp, "%s-%d-remote", myproc.nspace, myproc.rank)) {
        exit(1);
    }
    value.type = PMIX_STRING;
    value.data.string = "1234";
    if (PMIX_SUCCESS != (rc = PMIx_Put(PMIX_REMOTE, tmp, &value))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Put internal failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }
    free(tmp);

    if (PMIX_SUCCESS != (rc = PMIx_Commit())) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Commit failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }

    /* call fence to ensure the data is received */
    PMIX_PROC_CONSTRUCT(&proc);
    (void)strncpy(proc.nspace, myproc.nspace, PMIX_MAX_NSLEN);
    proc.rank = PMIX_RANK_WILDCARD;
    PMIX_INFO_CREATE(info, 1);
    flag = true;
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    if (PMIX_SUCCESS != (rc = PMIx_Fence(&proc, 1, info, 1))) {
        fprintf(stderr, "Client ns %s rank %d: PMIx_Fence failed: %d\n", myproc.nspace, myproc.rank, rc);
        goto done;
    }
    PMIX_INFO_FREE(info, 1);

    /* check the returned data */
    for (n=0; n < nprocs; n++) {
        if (0 > asprintf(&tmp, "%s-%d-local", myproc.nspace, myproc.rank)) {
            exit(1);
        }
        if (PMIX_SUCCESS != (rc = PMIx_Get(&myproc, tmp, NULL, 0, &val))) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s failed: %d\n", myproc.nspace, myproc.rank, tmp, rc);
            goto done;
        }
        if (PMIX_UINT64 != val->type) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong type: %d\n", myproc.nspace, myproc.rank, tmp, val->type);
            PMIX_VALUE_RELEASE(val);
            free(tmp);
            goto done;
        }
        if (1234 != val->data.uint64) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong value: %d\n", myproc.nspace, myproc.rank, tmp, (int)val->data.uint64);
            PMIX_VALUE_RELEASE(val);
            free(tmp);
            goto done;
        }
        fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned correct\n", myproc.nspace, myproc.rank, tmp);
        PMIX_VALUE_RELEASE(val);
        free(tmp);
        if (0 > asprintf(&tmp, "%s-%d-remote", myproc.nspace, myproc.rank)) {
            exit(1);
        }
        if (PMIX_SUCCESS != (rc = PMIx_Get(&myproc, tmp, NULL, 0, &val))) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s failed: %d\n", myproc.nspace, myproc.rank, tmp, rc);
            goto done;
        }
        if (PMIX_STRING != val->type) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong type: %d\n", myproc.nspace, myproc.rank, tmp, val->type);
            PMIX_VALUE_RELEASE(val);
            free(tmp);
            goto done;
        }
        if (0 != strcmp(val->data.string, "1234")) {
            fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned wrong value: %s\n", myproc.nspace, myproc.rank, tmp, val->data.string);
            PMIX_VALUE_RELEASE(val);
            free(tmp);
            goto done;
        }
        fprintf(stderr, "Client ns %s rank %d: PMIx_Get %s returned correct\n", myproc.nspace, myproc.rank, tmp);
        PMIX_VALUE_RELEASE(val);
        free(tmp);
    }

 done:
    /* finalize us */
    fprintf(stderr, "Client ns %s rank %d: Finalizing\n", myproc.nspace, myproc.rank);
    if (PMIX_SUCCESS != (rc = PMIx_Finalize())) {
        fprintf(stderr, "Client ns %s rank %d:PMIx_Finalize failed: %d\n", myproc.nspace, myproc.rank, rc);
    } else {
        fprintf(stderr, "Client ns %s rank %d:PMIx_Finalize successfully completed\n", myproc.nspace, myproc.rank);
    }
    fflush(stderr);
    return(0);
}
