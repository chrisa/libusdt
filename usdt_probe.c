/*
 * Copyright (c) 2012, Chris Andrews. All rights reserved.
 */

#include "usdt_internal.h"

#ifdef __APPLE__

uint32_t
usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc)
{
        uint32_t offset;
#ifdef __x86_64__
        offset = ((uint64_t) probe->probe_addr - (uint64_t) dof + 2);
#elif __i386__
        offset = ((uint32_t) probe->probe_addr - (uint32_t) dof + 2);
#else
#error "only x86_64 and i386 supported"
#endif
        return (offset);
}

uint32_t
usdt_is_enabled_offset(usdt_probe_t *probe, char *dof) 
{
        uint32_t offset;
#ifdef __x86_64__
        offset = ((uint64_t) probe->isenabled_addr - (uint64_t) dof + 2);
#elif __i386__
        offset = ((uint32_t) probe->isenabled_addr - (uint32_t) dof + 6);
#else
#error "only x86_64 and i386 supported"
#endif
        return (offset);
}

#else /* solaris and freebsd */

uint32_t
usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc)
{
        return (12);
}

uint32_t
usdt_is_enabled_offset(usdt_probe_t *probe, char *dof)
{
        return (6);
}

#endif

int
usdt_create_tracepoints(usdt_probe_t *probe)
{
        /* ensure that the tracepoints will fit the heap we're allocating */
        assert((usdt_tracepoint_end - usdt_tracepoint_isenabled) < FUNC_SIZE);

        if ((probe->isenabled_addr = valloc(FUNC_SIZE)) == NULL) {
                return (-1);
        }
        probe->probe_addr = probe->isenabled_addr +
                (usdt_tracepoint_probe - usdt_tracepoint_isenabled);

        memcpy(probe->isenabled_addr, usdt_tracepoint_isenabled, FUNC_SIZE);
        mprotect((void *)probe->isenabled_addr, FUNC_SIZE,
                 PROT_READ | PROT_WRITE | PROT_EXEC);

        return (0);
}
