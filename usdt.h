#include <sys/dtrace.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef __APPLE__
#include <stdlib.h>
#include <malloc.h>
#endif
    
#define FUNC_SIZE 32

extern void usdt_tracepoint_isenabled(void);
extern void usdt_tracepoint_probe(void);

typedef uint8_t usdt_argtype_t;
#define USDT_ARGTYPE_NONE    0
#define USDT_ARGTYPE_STRING  1
#define USDT_ARGTYPE_INTEGER 2

typedef struct usdt_probe {
  dof_stridx_t name;
  dof_stridx_t func;
  dof_stridx_t nargv;
  dof_stridx_t xargv;
  uint16_t noffs;
  uint32_t enoffidx;
  uint32_t argidx;
  uint16_t nenoffs;
  uint32_t offidx;
  uint8_t nargc;
  uint8_t xargc;
  int (*isenabled_addr)(void);
  void *probe_addr;
  struct usdt_probe *next;
  usdt_argtype_t types[6];
} usdt_probe_t;

void *usdt_probe_dof(usdt_probe_t *probe);
uint32_t usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc);
uint32_t usdt_is_enabled_offset(usdt_probe_t *probe, char *dof);
void usdt_create_tracepoints(usdt_probe_t *probe);
int usdt_is_enabled(usdt_probe_t *probe);
void usdt_fire_probe(usdt_probe_t *probe, int argc, void **argv);

typedef struct usdt_probedef {
  char *name;
  char *function;
  usdt_argtype_t types[7];
  struct usdt_probe *probe;
  struct usdt_probedef *next;
} usdt_probedef_t;

uint8_t usdt_probedef_argc(usdt_probedef_t *probedef);

typedef struct usdt_dof_section {
  dof_secidx_t index;
  uint32_t type;
  uint32_t flags;
  uint32_t align;
  uint64_t offset;
  uint64_t size;
  uint32_t entsize;    
  size_t pad;
  struct usdt_dof_section *next;
  char *data;
} usdt_dof_section_t;

void usdt_dof_section_init(usdt_dof_section_t *section, uint32_t type, dof_secidx_t index);
void usdt_dof_section_add_data(usdt_dof_section_t *section, void *data, size_t length);
void *usdt_dof_section_header(usdt_dof_section_t *section);

typedef struct usdt_strtab {
  dof_secidx_t index;
  uint32_t type;
  uint32_t flags;
  uint32_t align;
  uint64_t offset;
  uint64_t size;
  uint32_t entsize;    
  size_t pad;
  int strindex;
  char *data;
} usdt_strtab_t;

void usdt_strtab_init(usdt_strtab_t *strtab, dof_secidx_t index);
dof_stridx_t usdt_strtab_add(usdt_strtab_t *strtab, char *string);
void *usdt_strtab_header(usdt_strtab_t *strtab);
size_t usdt_strtab_size(usdt_strtab_t *strtab);

typedef struct usdt_dof_file {
  char *dof;
  int gen;
  size_t size;
  usdt_strtab_t *strtab;
  usdt_dof_section_t *sections;
} usdt_dof_file_t;

void usdt_dof_file_init(usdt_dof_file_t *file, size_t size);
void usdt_dof_file_append_section(usdt_dof_file_t *file, usdt_dof_section_t *section);
void usdt_dof_file_generate(usdt_dof_file_t *file);
void usdt_dof_file_load(usdt_dof_file_t *file);

typedef struct usdt_provider {
  char *name;
  usdt_probedef_t *probedefs;
  usdt_probe_t *probes;
  usdt_dof_file_t *file;
} usdt_provider_t;

void usdt_provider_add_probe(usdt_provider_t *provider, usdt_probedef_t *probedef);
void usdt_provider_enable(usdt_provider_t *provider);
size_t usdt_provider_dof_size(usdt_provider_t *provider, usdt_strtab_t *strtab);
