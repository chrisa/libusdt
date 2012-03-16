#include "usdt.h"

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

#else /* solaris */

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

void
usdt_create_tracepoints(usdt_probe_t *probe)
{
        probe->isenabled_addr = valloc(FUNC_SIZE);
        probe->probe_addr = probe->isenabled_addr + 
                (usdt_tracepoint_probe - usdt_tracepoint_isenabled);

        mprotect((void *)probe->isenabled_addr, FUNC_SIZE,
                 PROT_READ | PROT_WRITE | PROT_EXEC);

        memcpy(probe->isenabled_addr, usdt_tracepoint_isenabled, FUNC_SIZE);
}
