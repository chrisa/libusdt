#include "usdt.h"

#ifdef __APPLE__

uint32_t usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc) {
#ifdef __x86_64__
  return (uint32_t) ((uint64_t) probe->probe_addr - (uint64_t) dof + 2);
#elif __i386__
  return (uint32_t) ((uint32_t) probe->probe_addr - (uint32_t) dof + 2);
#else
  #error "only x86_64 and i386 supported"
#endif
}

uint32_t usdt_is_enabled_offset(usdt_probe_t *probe, char *dof) {
#ifdef __x86_64__
  return (uint32_t) ((uint64_t) probe->isenabled_addr - (uint64_t) dof + 2);
#elif __i386__
  return (uint32_t) ((uint32_t) probe->isenabled_addr - (uint32_t) dof + 6);
#else
  #error "only x86_64 and i386 supported"
#endif
}

#else /* solaris */

uint32_t usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc) {
  return 2;
}

uint32_t usdt_is_enabled_offset(usdt_probe_t *probe, char *dof) {
  return 6;
}

#endif

void usdt_create_tracepoints(usdt_probe_t *probe) {
  probe->isenabled_addr = valloc(FUNC_SIZE);
  (void)mprotect((void *)probe->isenabled_addr, FUNC_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
  memcpy(probe->isenabled_addr, usdt_tracepoint_isenabled, FUNC_SIZE);

  probe->probe_addr = (void *) valloc(FUNC_SIZE);
  (void)mprotect((void *)probe->probe_addr, FUNC_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
  memcpy(probe->probe_addr, (void *)probe_tracepoint, FUNC_SIZE);
}

int usdt_is_enabled(usdt_probe_t *probe) {
  return (*probe->isenabled_addr)();
}

void usdt_fire_probe(usdt_probe_t *probe, int argc, void **nargv) {
  usdt_tracepoint_probe(probe->probe_addr, argc, nargv);
}
