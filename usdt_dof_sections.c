#include "usdt.h"

int
usdt_dof_probe_sect(usdt_probedef_t *pd, usdt_strtab_t *strtab,
                    uint32_t offidx, uint32_t argidx)
{
        dof_stridx_t argv = 0;
        dof_stridx_t type;
        uint8_t i, argc = 0;
        usdt_probe_t *probe = pd->probe;

        for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++) {
                probe->types[i] = pd->types[i];

                switch(pd->types[i]) {
                case USDT_ARGTYPE_INTEGER:
                        if ((type = usdt_strtab_add(strtab, "int")) < 0)
                                return (-1);
                        break;
                case USDT_ARGTYPE_STRING:
                        if ((type = usdt_strtab_add(strtab, "char *")) < 0)
                                return (-1);
                        break;
                default:
                        break;
                }

                argc++;
                if (argv == 0)
                        argv = type;
        }

        if ((probe->name = usdt_strtab_add(strtab, pd->name)) < 0)
                return (-1);
        if ((probe->func = usdt_strtab_add(strtab, pd->function)) < 0)
                return (-1);

        probe->next     = NULL;
        probe->noffs    = 1;
        probe->enoffidx = offidx;
        probe->argidx   = argidx;
        probe->nenoffs  = 1;
        probe->offidx   = offidx;
        probe->nargc    = argc;
        probe->xargc    = argc;
        probe->nargv    = argv;
        probe->xargv    = argv;

        return usdt_create_tracepoints(probe);
}

int
usdt_dof_probes_sect(usdt_dof_section_t *probes,
                     usdt_provider_t *provider, usdt_strtab_t *strtab)
{
        usdt_probedef_t *pd;
        usdt_probe_t *p;
        uint32_t argidx = 0;
        uint32_t offidx = 0;
        void *dof;

        usdt_dof_section_init(probes, DOF_SECT_PROBES, 1);

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                usdt_dof_probe_sect(pd, strtab, offidx, argidx);

                argidx += pd->probe->nargc;
                offidx++;

                if (provider->probes == NULL) {
                        provider->probes = pd->probe;
                }
                else {
                        for (p = provider->probes; p->next != NULL; p = p->next) ;
                        p->next = pd->probe;
                }

                dof = usdt_probe_dof(pd->probe);
                if (usdt_dof_section_add_data(probes, dof, sizeof(dof_probe_t)) < 0) {
                        usdt_error(provider, USDT_ERROR_MALLOC);
                        return (-1);
                }

                probes->entsize = sizeof(dof_probe_t);
        }

        return (0);
}

int
usdt_dof_prargs_sect(usdt_dof_section_t *prargs, usdt_provider_t *provider)
{
        usdt_probedef_t *pd;
        uint8_t i;

        usdt_dof_section_init(prargs, DOF_SECT_PRARGS, 2);
        prargs->entsize = 1;

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                for (i = 0; pd->types[i] != USDT_ARGTYPE_NONE && i < 6; i++)
                        usdt_dof_section_add_data(prargs, &i, 1);
        }
        if (prargs->size == 0) {
                i = 0;
                if (usdt_dof_section_add_data(prargs, &i, 1) < 0) {
                        usdt_error(provider, USDT_ERROR_MALLOC);
                        return (-1);
                }
        }

        return (0);
}

int
usdt_dof_proffs_sect(usdt_dof_section_t *proffs,
                     usdt_provider_t *provider, char *dof)
{
        usdt_probedef_t *pd;
        uint32_t off;

        usdt_dof_section_init(proffs, DOF_SECT_PROFFS, 3);
        proffs->entsize = 4;

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                off = usdt_probe_offset(pd->probe, dof, usdt_probedef_argc(pd));
                if (usdt_dof_section_add_data(proffs, &off, 4) < 0) {
                        usdt_error(provider, USDT_ERROR_MALLOC);
                        return (-1);
                }
        }

        return (0);
}

int
usdt_dof_prenoffs_sect(usdt_dof_section_t *prenoffs,
                       usdt_provider_t *provider, char *dof)
{
        usdt_probedef_t *pd;
        uint32_t off;

        usdt_dof_section_init(prenoffs, DOF_SECT_PRENOFFS, 4);
        prenoffs->entsize = 4;

        for (pd = provider->probedefs; pd != NULL; pd = pd->next) {
                off = usdt_is_enabled_offset(pd->probe, dof);
                if (usdt_dof_section_add_data(prenoffs, &off, 4) < 0) {
                        usdt_error(provider, USDT_ERROR_MALLOC);
                        return (-1);
                }
        }

        return (0);
}

int
usdt_dof_provider_sect(usdt_dof_section_t *provider_s, usdt_provider_t *provider)
{
        dof_provider_t p;

        usdt_dof_section_init(provider_s, DOF_SECT_PROVIDER, 5);

        p.dofpv_strtab   = 0;
        p.dofpv_probes   = 1;
        p.dofpv_prargs   = 2;
        p.dofpv_proffs   = 3;
        p.dofpv_prenoffs = 4;
        p.dofpv_name     = 1; // provider name always first strtab entry.
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

        if ((usdt_dof_section_add_data(provider_s, &p, sizeof(p))) < 0) {
                usdt_error(provider, USDT_ERROR_MALLOC);
                return (-1);
        }

        return (0);
}
