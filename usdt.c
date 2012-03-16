#include "usdt.h"

#include <stdlib.h>
#include <stdarg.h>

#include <stdio.h>

void *usdt_probe_dof(usdt_probe_t *probe) {
  void *dof;
  dof_probe_t p;
  memset(&p, 0, sizeof(p));

#ifdef __x86_64__
  p.dofpr_addr     = (uint64_t) probe->isenabled_addr;
#elif __i386__
  p.dofpr_addr     = (uint32_t) probe->isenabled_addr;
#else
  #error "only x86_64 and i386 supported"
#endif
  p.dofpr_func     = probe->func;
  p.dofpr_name     = probe->name;
  p.dofpr_nargv    = probe->nargv;
  p.dofpr_xargv    = probe->xargv;
  p.dofpr_argidx   = probe->argidx;
  p.dofpr_offidx   = probe->offidx;
  p.dofpr_nargc    = probe->nargc;
  p.dofpr_xargc    = probe->xargc;
  p.dofpr_noffs    = probe->noffs;
  p.dofpr_enoffidx = probe->enoffidx;
  p.dofpr_nenoffs  = probe->nenoffs;

  dof = malloc(sizeof(dof_probe_t));
  memcpy(dof, &p, sizeof(dof_probe_t));

  return dof;
}

dof_stridx_t usdt_strtab_add(usdt_strtab_t *strtab, char *string) {
  size_t length;
  int index;
  
  length = strlen(string);

  if (strtab->data == NULL) {
    strtab->strindex = 1;
    strtab->data = (char *) malloc(1);
    memcpy((void *) strtab->data, "\0", 1);
  }

  index = strtab->strindex;
  strtab->strindex += (length + 1);
  strtab->data = (char *) realloc(strtab->data, strtab->strindex);
  (void) memcpy((void *) (strtab->data + index), (void *)string, length + 1);
  strtab->size = index + length + 1;

  return index;
}

void usdt_dof_section_add_data(usdt_dof_section_t *section, void *data, size_t length) {
  if (section->data == NULL) {
    section->data = (char *) malloc(1);
  }
  section->data = (char *) realloc((void *)section->data, section->size + length);
  (void) memcpy(section->data + section->size, data, length);
  section->size += length;
}

void usdt_create_probe_varargs(usdt_probedef_t *p, char *func, char *name, ...) {
  va_list ap;
  int i;
  char *type;
  
  p->function = strdup(func);
  p->name = strdup(name);

  va_start(ap, name);
  
  for (i = 0; i < 6; i++) {
    if ((type = va_arg(ap, char *)) != NULL) {
      if (strncmp("char *", type, 6)) {
        p->types[i] = USDT_ARGTYPE_STRING;
      }
      if (strncmp("int", type, 3)) {
        p->types[i] = USDT_ARGTYPE_INTEGER;
      }
    }
    else {
      p->types[i] = USDT_ARGTYPE_NONE;
    }
  }
}

void usdt_create_probe(usdt_probedef_t *p, char *func, char *name, char **types) {
  int i;
  
  p->function = strdup(func);
  p->name = strdup(name);

  for (i = 0; i < 6; i++) {
    if (types[i] != NULL) {
      if (strncmp("char *", types[i], 6) == 0) {
        p->types[i] = USDT_ARGTYPE_STRING;
      }
      if (strncmp("int", types[i], 3) == 0) {
        p->types[i] = USDT_ARGTYPE_INTEGER;
      }
    }
    else {
      p->types[i] = USDT_ARGTYPE_NONE;
    }
  }
}

void usdt_provider_add_probe(usdt_provider_t *provider, usdt_probedef_t *probedef) {
  usdt_probedef_t *pd;
  
  probedef->next = NULL;
  if (provider->probedefs == NULL)
    provider->probedefs = probedef;
  else {
    for (pd = provider->probedefs; (pd->next != NULL); pd = pd->next) ;
    pd->next = probedef;
  }
}

size_t usdt_provider_dof_size(usdt_provider_t *provider, usdt_strtab_t *strtab) {
  uint8_t i, j;
  int args = 0;
  int probes = 0;
  size_t size = 0;
  usdt_probedef_t *pd;
  size_t sections[8];

  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    args += usdt_probedef_argc(pd);
    probes++;
  }
    
  sections[0] = sizeof(dof_hdr_t);
  sections[1] = sizeof(dof_sec_t) * 6;
  sections[2] = strtab->size;
  sections[3] = sizeof(dof_probe_t) * probes;
  sections[4] = sizeof(uint8_t) * args;
  sections[5] = sizeof(uint32_t) * probes;
  sections[6] = sizeof(uint32_t) * probes;
  sections[7] = sizeof(dof_provider_t);
  
  for (i = 0; i < 8; i++) {
    size += sections[i];
    j = size % 8;
    if (j > 0) {
      size += (8 - j);
    }
  }
  
  return size;
}

uint8_t usdt_probedef_argc(usdt_probedef_t *probedef) {
  uint8_t args = 0;
  uint8_t i;
  
  for (i = 0; probedef->types[i] != USDT_ARGTYPE_NONE && i < 6; i++)
    args++;
  
  return args;
}

void *usdt_dof_section_header(usdt_dof_section_t *section) {
  void *dof;
  dof_sec_t header;
  memset(&header, 0, sizeof(header));
  
  header.dofs_flags   = section->flags;
  header.dofs_type    = section->type;
  header.dofs_offset  = section->offset;
  header.dofs_size    = section->size;
  header.dofs_entsize = section->entsize;
  header.dofs_align   = section->align;
  
  dof = malloc(sizeof(dof_sec_t));
  memcpy(dof, &header, sizeof(dof_sec_t));
  
  return dof;
}

void *usdt_strtab_header(usdt_strtab_t *strtab) {
  void *dof;
  dof_sec_t header;
  memset(&header, 0, sizeof(header));
  
  header.dofs_flags   = strtab->flags;
  header.dofs_type    = strtab->type;
  header.dofs_offset  = strtab->offset;
  header.dofs_size    = strtab->size;
  header.dofs_entsize = strtab->entsize;
  header.dofs_align   = strtab->align;
  
  dof = malloc(sizeof(dof_sec_t));
  memcpy(dof, &header, sizeof(dof_sec_t));
  
  return dof;
}

void usdt_strtab_init(usdt_strtab_t *strtab, dof_secidx_t index) {
  strtab->type    = DOF_SECT_STRTAB;;
  strtab->index   = index;
  strtab->flags   = DOF_SECF_LOAD;
  strtab->offset  = 0;
  strtab->size    = 0;
  strtab->entsize = 0;
  strtab->pad	  = 0;
  strtab->data    = NULL;
  strtab->align   = 1;
}

void usdt_dof_section_init(usdt_dof_section_t *section, uint32_t type, dof_secidx_t index) {
  section->type    = type;
  section->index   = index;
  section->flags   = DOF_SECF_LOAD;
  section->offset  = 0;
  section->size    = 0;
  section->entsize = 0;
  section->pad	   = 0;
  section->next    = NULL;
  section->data    = NULL;
      
  switch(type) {
  case DOF_SECT_PROBES:   section->align = 8; break;
  case DOF_SECT_PRARGS:   section->align = 1; break;
  case DOF_SECT_PROFFS:   section->align = 4; break;
  case DOF_SECT_PRENOFFS: section->align = 4; break;
  case DOF_SECT_PROVIDER: section->align = 4; break;
  }
}

void usdt_provider_enable(usdt_provider_t *provider) {
  void *dof;
  uint32_t argidx = 0;
  uint32_t offidx = 0;
  uint8_t i;
  usdt_strtab_t *strtab;
  usdt_probedef_t *pd;
  usdt_dof_file_t *file;
  dof_provider_t p;
  dof_stridx_t pv_name;  

  usdt_dof_section_t *probes;
  usdt_dof_section_t *prargs;
  usdt_dof_section_t *proffs;
  usdt_dof_section_t *prenoffs;
  usdt_dof_section_t *provider_s;
  
  strtab = malloc(sizeof(*strtab));
  usdt_strtab_init(strtab, 0);
  pv_name = usdt_strtab_add(strtab, provider->name);
  
  if (provider->probedefs == NULL) {
    return;
  }

  /* PROBES SECTION */
  
  probes = malloc(sizeof(*probes));
  usdt_dof_section_init(probes, DOF_SECT_PROBES, 1);
  
  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint8_t argc = 0;
    dof_stridx_t argv = 0;
    usdt_probe_t *probe;

    probe = malloc(sizeof(*probe));
    
    for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++) {
      dof_stridx_t type;
      
      probe->types[i] = pd->types[i];
      
      switch(pd->types[i]) {
      case USDT_ARGTYPE_INTEGER:
        type = usdt_strtab_add(strtab, "int");
        break;
      case USDT_ARGTYPE_STRING:
        type = usdt_strtab_add(strtab, "char *");
        break;
      }
      
      argc++;
      if (argv == 0)
        argv = type;
    }

    probe->next     = NULL;
    probe->name     = usdt_strtab_add(strtab, pd->name);
    probe->func     = usdt_strtab_add(strtab, pd->function);
    probe->noffs    = 1;
    probe->enoffidx = offidx;
    probe->argidx   = argidx;
    probe->nenoffs  = 1;
    probe->offidx   = offidx;
    probe->nargc    = argc;
    probe->xargc    = argc;
    probe->nargv    = argv;
    probe->xargv    = argv;
    
    usdt_create_tracepoints(probe);
    
    argidx += argc;
    offidx++;
    
    pd->probe = probe;
    
    if (provider->probes == NULL)
      provider->probes = probe;
    else {
      usdt_probe_t *p;
      for (p = provider->probes; (p->next != NULL); p = p->next) ;
      p->next = probe;
    }

    dof = usdt_probe_dof(probe);
    usdt_dof_section_add_data(probes, dof, sizeof(dof_probe_t));
    free(dof);

    probes->entsize = sizeof(dof_probe_t);
  }
  
  /* PRARGS SECTION */

  prargs = malloc(sizeof(*prargs));
  usdt_dof_section_init(prargs, DOF_SECT_PRARGS, 2);
  prargs->entsize = 1;

  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++) {
      usdt_dof_section_add_data(prargs, &i, 1);
    }
  }
  if (prargs->size == 0) {
    i = 0;
    usdt_dof_section_add_data(prargs, &i, 1);
  }
  
  /* estimate DOF size here, allocate */
  file = malloc(sizeof(*file));
  file->sections = NULL;
  file->size = usdt_provider_dof_size(provider, strtab);
  file->dof = (char *) valloc(file->size);

  file->strtab = strtab;
  usdt_dof_file_append_section(file, probes);
  usdt_dof_file_append_section(file, prargs);
 
  /* PROFFS SECTION */

  proffs = malloc(sizeof(*proffs));
  usdt_dof_section_init(proffs, DOF_SECT_PROFFS, 3);
  proffs->entsize = 4;
  
  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint32_t off = usdt_probe_offset(pd->probe, file->dof, usdt_probedef_argc(pd));
    usdt_dof_section_add_data(proffs, &off, 4);
  }
  usdt_dof_file_append_section(file, proffs);

  /* PRENOFFS SECTION */

  prenoffs = malloc(sizeof(*prenoffs));
  usdt_dof_section_init(prenoffs, DOF_SECT_PRENOFFS, 4);
  prenoffs->entsize = 4;

  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint32_t off = usdt_is_enabled_offset(pd->probe, file->dof);
    usdt_dof_section_add_data(prenoffs, &off, 4);
  }
  usdt_dof_file_append_section(file, prenoffs);
    
  /* PROVIDER SECTION */

  provider_s = malloc(sizeof(*provider_s));
  usdt_dof_section_init(provider_s, DOF_SECT_PROVIDER, 5);

  memset(&p, 0, sizeof(p));
  
  p.dofpv_strtab   = 0;
  p.dofpv_probes   = 1;
  p.dofpv_prargs   = 2;
  p.dofpv_proffs   = 3;
  p.dofpv_prenoffs = 4;
  p.dofpv_name     = pv_name;
  p.dofpv_provattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING);
  p.dofpv_modattr  = DOF_ATTR(DTRACE_STABILITY_PRIVATE,
                              DTRACE_STABILITY_PRIVATE,
                              DTRACE_STABILITY_EVOLVING);
  p.dofpv_funcattr = DOF_ATTR(DTRACE_STABILITY_PRIVATE,
                              DTRACE_STABILITY_PRIVATE,
                              DTRACE_STABILITY_EVOLVING);
  p.dofpv_nameattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING);
  p.dofpv_argsattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING,
                              DTRACE_STABILITY_EVOLVING);
  
  usdt_dof_section_add_data(provider_s, &p, sizeof(p));
  usdt_dof_file_append_section(file, provider_s);

  usdt_dof_file_generate(file);
  usdt_dof_file_load(file);

  provider->file = file;
}

