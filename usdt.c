#include "usdt_internal.h"

#include <stdlib.h>
#include <stdarg.h>

char *usdt_errors[] = {
  "failed to allocate memory",
  "failed to allocate page-aligned memory",
  "no probes defined",
  "failed to load DOF"
};

usdt_provider_t *
usdt_create_provider(const char *name, const char *module)
{
        usdt_provider_t *provider;

        if ((provider = malloc(sizeof *provider)) == NULL) 
                return NULL;

        provider->name = strdup(name);
        provider->module = strdup(module);
        provider->probedefs = NULL;

        return provider;
}

usdt_probedef_t *
usdt_create_probe_varargs(const char *func, const char *name, ...)
{
        va_list ap;
        int i;
        const char *type;
        usdt_probedef_t *p;

        if ((p = malloc(sizeof *p)) == NULL)
                return (NULL);

        p->function = strdup(func);
        p->name = strdup(name);

        va_start(ap, name);

        for (i = 0; i < USDT_ARG_MAX; i++) {
                if ((type = va_arg(ap, const char *)) != NULL) {
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

        return (p);
}

usdt_probedef_t *
usdt_create_probe(const char *func, const char *name, size_t argc, const char **types)
{
        int i;
        usdt_probedef_t *p;

        if ((p = malloc(sizeof *p)) == NULL)
                return (NULL);

        p->function = strdup(func);
        p->name = strdup(name);
        p->argc = argc;

        for (i = 0; i < USDT_ARG_MAX; i++) {
                if (i < argc && types[i] != NULL) {
                        if (strncmp("char *", types[i], 6) == 0)
                                p->types[i] = USDT_ARGTYPE_STRING;
                        if (strncmp("int", types[i], 3) == 0)
                                p->types[i] = USDT_ARGTYPE_INTEGER;
                }
                else {
                        p->types[i] = USDT_ARGTYPE_NONE;
                }
        }

        return (p);
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

        for (i = 0; probedef->types[i] != USDT_ARGTYPE_NONE && i < USDT_ARG_MAX; i++)
                args++;

        return args;
}

int
usdt_provider_enable(usdt_provider_t *provider)
{
        usdt_strtab_t strtab;
        usdt_dof_file_t *file;
        usdt_probedef_t *pd;
        int i;
        size_t size;
        usdt_dof_section_t sects[5];

        if (provider->probedefs == NULL) {
                usdt_error(provider, USDT_ERROR_NOPROBES);
                return (-1);
        }

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                if ((pd->probe = malloc(sizeof(*pd->probe))) == NULL) {
                        usdt_error(provider, USDT_ERROR_MALLOC);
                        return (-1);
                }
        }

        if ((usdt_strtab_init(&strtab, 0)) < 0) {
                usdt_error(provider, USDT_ERROR_MALLOC);
                return (-1);
        }

        if ((usdt_strtab_add(&strtab, provider->name)) < 0) {
                usdt_error(provider, USDT_ERROR_MALLOC);
                return (-1);
        }

        if ((usdt_dof_probes_sect(&sects[0], provider, &strtab)) < 0)
                return (-1);
        if ((usdt_dof_prargs_sect(&sects[1], provider)) < 0)
                return (-1);

        size = usdt_provider_dof_size(provider, &strtab);
        if ((file = usdt_dof_file_init(provider, size)) == NULL)
                return (-1);

        if ((usdt_dof_proffs_sect(&sects[2], provider, file->dof)) < 0)
                return (-1);
        if ((usdt_dof_prenoffs_sect(&sects[3], provider, file->dof)) < 0)
                return (-1);
        if ((usdt_dof_provider_sect(&sects[4], provider)) < 0)
                return (-1);

        for (i = 0; i < 5; i++)
                usdt_dof_file_append_section(file, &sects[i]);

        usdt_dof_file_generate(file, &strtab);

        if ((usdt_dof_file_load(file, provider->module)) < 0) {
                usdt_error(provider, USDT_ERROR_LOADDOF);
                return (-1);
        }

        return (0);
}

int
usdt_is_enabled(usdt_probe_t *probe)
{
        return (*probe->isenabled_addr)();
}

void
usdt_fire_probe(usdt_probe_t *probe, size_t argc, void **nargv)
{
        usdt_probe_args(probe->probe_addr, argc, nargv);
}

void
usdt_error(usdt_provider_t *provider, usdt_error_t error)
{
        provider->error = usdt_errors[error];
}

char *
usdt_errstr(usdt_provider_t *provider)
{
        return (provider->error);
}
