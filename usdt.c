#include "usdt.h"

#include <stdio.h>
#include <stdlib.h>

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

void usdt_create_tracepoints(usdt_probe_t *probe) {
  uint8_t tracepoints[FUNC_SIZE] = {
    0x55, 0x48, 0x89, 0xe5, 
    0x48, 0x33, 0xc0, 0x90, 
    0x90, 0xc9, 0xc3, 0x00,
    0x55, 0x48, 0x89, 0xe5, 
    0x90, 0x0f, 0x1f, 0x40, 
    0x00, 0xc9, 0xc3, 0x0f, 
    0x1f, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  };
  
  probe->addr = (void *) valloc(FUNC_SIZE);
  (void)mprotect((void *)probe->addr, FUNC_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
  memcpy(probe->addr, tracepoints, FUNC_SIZE);
}

void *usdt_probe_dof(usdt_probe_t *probe) {
  void *dof;
  dof_probe_t p;
  memset(&p, 0, sizeof(p));

  p.dofpr_addr     = (uint64_t) probe->addr;
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

void usdt_dof_section_add_data(usdt_dof_section_t *section, void *data, size_t length) {
  if (section->data == NULL) {
    section->data = (char *) malloc(1);
  }
  section->data = (char *) realloc((void *)section->data, section->size + length);
  (void) memcpy(section->data + section->size, data, length);
  section->size += length;
}

void usdt_provider_add_probe(usdt_provider_t *provider, usdt_probedef_t *probedef) {
  usdt_probedef_t *pd;
  
  if (provider->probedefs == NULL)
    provider->probedefs = probedef;
  else {
    for (pd = provider->probedefs; (pd->next != NULL); pd = pd->next) ;
    pd->next = probedef;
  }
}

void usdt_dof_file_append_section(usdt_dof_file_t *file, usdt_dof_section_t *section) {
  usdt_dof_section_t *s;
  
  if (file->sections == NULL)
    file->sections = section;
  else {
    for (s = file->sections; (s->next != NULL); s = s->next) ;
    s->next = section;
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

uint32_t usdt_probe_offset(usdt_probe_t *probe, char *dof, uint8_t argc) {
  return (uint32_t) ((uint64_t) probe->addr - (uint64_t) dof + 18);
}

uint32_t usdt_is_enabled_offset(usdt_probe_t *probe, char *dof) {
  return (uint32_t) ((uint64_t) probe->addr - (uint64_t) dof + 6);
}

static uint8_t dof_version(uint8_t header_version) {
  uint8_t dof_version;
  /* DOF versioning: Apple always needs version 3, but Solaris can use
     1 or 2 depending on whether is-enabled probes are needed. */
#ifdef __APPLE__
  dof_version = DOF_VERSION_3;
#else
  switch(header_version) {
  case 1:
    dof_version = DOF_VERSION_1;
    break;
  case 2:
    dof_version = DOF_VERSION_2;
    break;
  default:
    dof_version = DOF_VERSION;
  }
#endif
  return dof_version;
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

void usdt_dof_file_generate(usdt_dof_file_t *file) {
  dof_hdr_t header;
  uint64_t filesz;
  uint64_t loadsz;
  usdt_dof_section_t *section;
  size_t pad = 0;
  size_t i = 0;
  size_t offset;
  void *h;

  memset(&header, 0, sizeof(header));

  header.dofh_ident[DOF_ID_MAG0] = DOF_MAG_MAG0;
  header.dofh_ident[DOF_ID_MAG1] = DOF_MAG_MAG1;
  header.dofh_ident[DOF_ID_MAG2] = DOF_MAG_MAG2;
  header.dofh_ident[DOF_ID_MAG3] = DOF_MAG_MAG3;
	  
  header.dofh_ident[DOF_ID_MODEL]    = DOF_MODEL_NATIVE;
  header.dofh_ident[DOF_ID_ENCODING] = DOF_ENCODE_NATIVE;
  header.dofh_ident[DOF_ID_DIFVERS]  = DIF_VERSION;
  header.dofh_ident[DOF_ID_DIFIREG]  = DIF_DIR_NREGS;
  header.dofh_ident[DOF_ID_DIFTREG]  = DIF_DTR_NREGS;

  header.dofh_ident[DOF_ID_VERSION] = dof_version(2); /* default 2, will be 3 on OSX */
  header.dofh_hdrsize = sizeof(dof_hdr_t);
  header.dofh_secsize = sizeof(dof_sec_t);
  header.dofh_secoff = sizeof(dof_hdr_t);

  header.dofh_secnum = 6;

  filesz = sizeof(dof_hdr_t) + (sizeof(dof_sec_t) * header.dofh_secnum);
  loadsz = filesz;

  file->strtab->offset = filesz;
  
  if (file->strtab->align > 1) {
    i = file->strtab->offset % file->strtab->align;
    if (i > 0) {
      pad = file->strtab->align - i;
      file->strtab->offset = (pad + file->strtab->offset);
      file->strtab->pad = pad;
    }
  }
  
  filesz += file->strtab->size + pad;
  if (file->strtab->flags & 1)
    loadsz += file->strtab->size + pad;
  
  for (section = file->sections; section != NULL; section = section->next) {
    pad = 0;
    i = 0;

    section->offset = filesz;

    if (section->align > 1) {
      i = section->offset % section->align;
      if (i > 0) {
        pad = section->align - i;
        section->offset = (pad + section->offset);
        section->pad = pad;
      }
    }
    
    filesz += section->size + pad;
    if (section->flags & 1)
      loadsz += section->size + pad;
  }
  
  header.dofh_loadsz = loadsz;
  header.dofh_filesz = filesz;
  memcpy(file->dof, &header, sizeof(dof_hdr_t));

  offset = sizeof(dof_hdr_t);

  h = usdt_strtab_header(file->strtab);
  (void) memcpy((file->dof + offset), h, sizeof(dof_sec_t));
  free(h);
  offset += sizeof(dof_sec_t);

  for (section = file->sections; section != NULL; section = section->next) {
    h = usdt_dof_section_header(section);
    (void) memcpy((file->dof + offset), h, sizeof(dof_sec_t));
    free(h);
    offset += sizeof(dof_sec_t);
  }

  if (file->strtab->pad > 0) {
    (void) memcpy((file->dof + offset), "\0", file->strtab->pad);
    offset += file->strtab->pad;
  }
  (void) memcpy((file->dof + offset), file->strtab->data, file->strtab->size);
  offset += file->strtab->size;

  for (section = file->sections; section != NULL; section = section->next) {
    if (section->pad > 0) {
      (void) memcpy((file->dof + offset), "\0", section->pad);
      offset += section->pad;
    }
    (void) memcpy((file->dof + offset), section->data, section->size);
    offset += section->size;
  }
}

#ifdef __APPLE__
static const char *helper = "/dev/dtracehelper";

static int _loaddof(int fd, dof_helper_t *dh) {
  int ret;
  uint8_t buffer[sizeof(dof_ioctl_data_t) + sizeof(dof_helper_t)];
  dof_ioctl_data_t* ioctlData = (dof_ioctl_data_t*)buffer;
  user_addr_t val;

  ioctlData->dofiod_count = 1;
  memcpy(&ioctlData->dofiod_helpers[0], dh, sizeof(dof_helper_t));
    
  val = (user_addr_t)(unsigned long)ioctlData;
  ret = ioctl(fd, DTRACEHIOC_ADDDOF, &val);

  return ret;
}

#else /* Solaris */

/* ignore Sol10 GA ... */
static const char *helper = "/dev/dtrace/helper";

static int _loaddof(int fd, dof_helper_t *dh) {
  return ioctl(fd, DTRACEHIOC_ADDDOF, dh);
}

#endif

void usdt_dof_file_load(usdt_dof_file_t *file) {
  dof_helper_t dh;
  int fd;
  dof_hdr_t *dof;

  dof = (dof_hdr_t *) file->dof;

  dh.dofhp_dof  = (uintptr_t)dof;
  dh.dofhp_addr = (uintptr_t)dof;
  (void) snprintf(dh.dofhp_mod, sizeof (dh.dofhp_mod), "module");
  
  fd = open(helper, O_RDWR);
  file->gen = _loaddof(fd, &dh);

  (void) close(fd);
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
  
  strtab = malloc(sizeof(struct usdt_strtab));
  usdt_strtab_init(strtab, 0);
  pv_name = usdt_strtab_add(strtab, provider->name);
  
  if (provider->probedefs == NULL) {
    return;
  }

  /* PROBES SECTION */
  
  probes = malloc(sizeof(struct usdt_dof_section));
  usdt_dof_section_init(probes, DOF_SECT_PROBES, 1);
  
  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint8_t argc = 0;
    dof_stridx_t argv = 0;
    usdt_probe_t *probe;

    probe = malloc(sizeof(struct usdt_probe));
    
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

  prargs = malloc(sizeof(struct usdt_dof_section));
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
  file = malloc(sizeof(struct usdt_dof_file));
  file->sections = NULL;
  file->size = usdt_provider_dof_size(provider, strtab);
  file->dof = (char *) valloc(file->size);

  file->strtab = strtab;
  usdt_dof_file_append_section(file, probes);
  usdt_dof_file_append_section(file, prargs);
 
  /* PROFFS SECTION */

  proffs = malloc(sizeof(struct usdt_dof_section));
  usdt_dof_section_init(proffs, DOF_SECT_PROFFS, 3);
  proffs->entsize = 4;
  
  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint32_t off = usdt_probe_offset(pd->probe, file->dof, usdt_probedef_argc(pd));
    usdt_dof_section_add_data(proffs, &off, 4);
  }
  usdt_dof_file_append_section(file, proffs);

  /* PRENOFFS SECTION */

  prenoffs = malloc(sizeof(struct usdt_dof_section));
  usdt_dof_section_init(prenoffs, DOF_SECT_PRENOFFS, 4);
  prenoffs->entsize = 4;

  for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
    uint32_t off = usdt_is_enabled_offset(pd->probe, file->dof);
    usdt_dof_section_add_data(prenoffs, &off, 4);
  }
  usdt_dof_file_append_section(file, prenoffs);
    
  /* PROVIDER SECTION */

  provider_s = malloc(sizeof(struct usdt_dof_section));
  usdt_dof_section_init(provider_s, DOF_SECT_PROVIDER, 5);

  memset(&p, 0, sizeof(p));
  
  p.dofpv_strtab   = 0;
  p.dofpv_probes   = 1;
  p.dofpv_prargs   = 2;
  p.dofpv_proffs   = 3;
  p.dofpv_prenoffs = 4;
  p.dofpv_name     = pv_name;
  p.dofpv_provattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
  p.dofpv_modattr  = DOF_ATTR(DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_EVOLVING);
  p.dofpv_funcattr = DOF_ATTR(DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_EVOLVING);
  p.dofpv_nameattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
  p.dofpv_argsattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
  
  usdt_dof_section_add_data(provider_s, &p, sizeof(p));
  usdt_dof_file_append_section(file, provider_s);

  usdt_dof_file_generate(file);
  usdt_dof_file_load(file);

  provider->file = file;
}

void usdt_fire_probe(usdt_probe_t *probe, int argc, void **nargv) {
  void *argv[6];
  int i;

  void (*func0)();
  void (*func1)(void *);
  void (*func2)(void *, void *);
  void (*func3)(void *, void *, void *);
  void (*func4)(void *, void *, void *, void *);
  void (*func5)(void *, void *, void *, void *, void *);
  void (*func6)(void *, void *, void *, void *, void *, void *);
  
  for (i = 0; i < probe->nargc; i++) {
    if (probe->types[i] == USDT_ARGTYPE_STRING) {
      /* char * */
      argv[i] = (void *)(char *) nargv[i];
    }
    else {
      /* int */
      argv[i] = (void *)(long) nargv[i];
    }
  }

  switch (probe->nargc) {
  case 0:
    func0 = (void (*)())((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func0)();
    break;
  case 1:
    func1 = (void (*)(void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func1)(argv[0]);
    break;
  case 2:
    func2 = (void (*)(void *, void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func2)(argv[0], argv[1]);
    break;
  case 3:
    func3 = (void (*)(void *, void *, void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func3)(argv[0], argv[1], argv[2]);
    break;
  case 4:
    func4 = (void (*)(void *, void *, void *, void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func4)(argv[0], argv[1], argv[2], argv[3]);
    break;
  case 5:
    func5 = (void (*)(void *, void *, void *, void *, void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func5)(argv[0], argv[1], argv[2], argv[3], argv[4]);
    break;
  case 6:
    func6 = (void (*)(void *, void *, void *, void *, void *, void *))((uint64_t)probe->addr + IS_ENABLED_FUNC_LEN); 
    (void)(*func6)(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
    break;
  }
}
