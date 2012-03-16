#include "usdt.h"

#include <stdlib.h>
#include <stdarg.h>

void
usdt_create_probe_varargs(usdt_probedef_t *p, char *func, char *name, ...)
{
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

void
usdt_create_probe(usdt_probedef_t *p, char *func, char *name, char **types)
{
        int i;
  
        p->function = strdup(func);
        p->name = strdup(name);

        for (i = 0; i < 6; i++) {
                if (types[i] != NULL) {
                        if (strncmp("char *", types[i], 6) == 0)
                                p->types[i] = USDT_ARGTYPE_STRING;

                        if (strncmp("int", types[i], 3) == 0)
                                p->types[i] = USDT_ARGTYPE_INTEGER;
                }
                else {
                        p->types[i] = USDT_ARGTYPE_NONE;
                }
        }
}

void
usdt_provider_add_probe(usdt_provider_t *provider, usdt_probedef_t *probedef)
{
        usdt_probedef_t *pd;
  
        probedef->next = NULL;
        if (provider->probedefs == NULL)
                provider->probedefs = probedef;
        else {
                for (pd = provider->probedefs; (pd->next != NULL); pd = pd->next) ;
                pd->next = probedef;
        }
}

uint8_t
usdt_probedef_argc(usdt_probedef_t *probedef)
{
        uint8_t args = 0;
        uint8_t i;
  
        for (i = 0; probedef->types[i] != USDT_ARGTYPE_NONE && i < 6; i++)
                args++;
  
        return args;
}

void
usdt_provider_enable(usdt_provider_t *provider)
{
        usdt_strtab_t *strtab;
        usdt_dof_file_t *file;

        usdt_dof_section_t *probes;
        usdt_dof_section_t *prargs;
        usdt_dof_section_t *proffs;
        usdt_dof_section_t *prenoffs;
        usdt_dof_section_t *provider_s;
  
        strtab = malloc(sizeof(*strtab));
        usdt_strtab_init(strtab, 0);
        usdt_strtab_add(strtab, provider->name);
  
        if (provider->probedefs == NULL)
                return;

        probes = usdt_dof_probes_sect(provider, strtab);
        prargs = usdt_dof_prargs_sect(provider);

        file = malloc(sizeof(*file));
        file->sections = NULL;
        file->size = usdt_provider_dof_size(provider, strtab);
        file->dof = (char *) valloc(file->size);
        file->strtab = strtab;

        proffs = usdt_dof_proffs_sect(provider, file->dof);
        prenoffs = usdt_dof_prenoffs_sect(provider, file->dof);
        provider_s = usdt_dof_provider_sect(provider);

        usdt_dof_file_append_section(file, probes);
        usdt_dof_file_append_section(file, prargs);
        usdt_dof_file_append_section(file, proffs);
        usdt_dof_file_append_section(file, prenoffs);
        usdt_dof_file_append_section(file, provider_s);

        usdt_dof_file_generate(file);
        usdt_dof_file_load(file);

        provider->file = file;
}

int
usdt_is_enabled(usdt_probe_t *probe)
{
        return (*probe->isenabled_addr)();
}

void
usdt_fire_probe(usdt_probe_t *probe, int argc, void **nargv)
{
        usdt_probe_args(probe->probe_addr, argc, nargv);
}
