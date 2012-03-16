#include "usdt.h"

#include <stdlib.h>

void *
usdt_probe_dof(usdt_probe_t *probe)
{
        void *dof;
        dof_probe_t p;

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

void
usdt_dof_section_add_data(usdt_dof_section_t *section, void *data, size_t length)
{
        section->data = (char *) realloc((void *)section->data, section->size + length);
        memcpy(section->data + section->size, data, length);
        section->size += length;
}

size_t
usdt_provider_dof_size(usdt_provider_t *provider, usdt_strtab_t *strtab)
{
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
                if (j > 0)
                        size += (8 - j);
        }
  
        return size;
}

void
usdt_dof_section_init(usdt_dof_section_t *section, uint32_t type, dof_secidx_t index)
{
        section->type    = type;
        section->index   = index;
        section->flags   = DOF_SECF_LOAD;
        section->offset  = 0;
        section->size    = 0;
        section->entsize = 0;
        section->pad	 = 0;
        section->next    = NULL;
        section->data    = (char *) malloc(1);
      
        switch(type) {
        case DOF_SECT_PROBES:   section->align = 8; break;
        case DOF_SECT_PRARGS:   section->align = 1; break;
        case DOF_SECT_PROFFS:   section->align = 4; break;
        case DOF_SECT_PRENOFFS: section->align = 4; break;
        case DOF_SECT_PROVIDER: section->align = 4; break;
        }
}

void *
usdt_dof_section_header(usdt_dof_section_t *section)
{
        void *dof;
        dof_sec_t header;
  
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

void *
usdt_strtab_header(usdt_strtab_t *strtab)
{
        void *dof;
        dof_sec_t header;
  
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

void
usdt_strtab_init(usdt_strtab_t *strtab, dof_secidx_t index)
{
        strtab->type    = DOF_SECT_STRTAB;;
        strtab->index   = index;
        strtab->flags   = DOF_SECF_LOAD;
        strtab->offset  = 0;
        strtab->size    = 0;
        strtab->entsize = 0;
        strtab->pad	  = 0;
        strtab->data    = NULL;
        strtab->align   = 1;
        strtab->strindex = 1;
        strtab->data = (char *) malloc(1);
        memcpy((void *) strtab->data, "\0", 1);
}

dof_stridx_t
usdt_strtab_add(usdt_strtab_t *strtab, char *string)
{
        size_t length;
        int index;
  
        length = strlen(string);
        index = strtab->strindex;
        strtab->strindex += (length + 1);
        strtab->data = (char *) realloc(strtab->data, strtab->strindex);
        memcpy((void *) (strtab->data + index), (void *)string, length + 1);
        strtab->size = index + length + 1;

        return index;
}

