#include "usdt.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv) {
        usdt_provider_t *provider;
        usdt_probedef_t *probedef;
        char *char_argv[6] = { "a", "b", "c", "d", "e", "f" };
        int int_argv[6] = { 1, 2, 3, 4, 5, 6 };
        void **args;
        int i;

        if (argc < 3) {
                fprintf(stderr, "usage: %s func name [types ...]\n", argv[0]);
                return(1);
        }

        if (argc > 3) {
                args = malloc((argc-3) * sizeof(void *));
        }

        for (i = 0; i < 6; i++) {
                if (argv[i+3] != NULL && i+3 < argc) {
                        if (strncmp("c", argv[i+3], 1) == 0) {
                                args[i] = (void *)char_argv[i];
                                argv[i+3] = strdup("char *");
                        }
                        if (strncmp("i", argv[i+3], 1) == 0) {
                                args[i] = (void *)(long)int_argv[i];
                                argv[i+3] = strdup("int");
                        }
                }
        }

        if ((provider = usdt_create_provider("testlibusdt")) == NULL) {
                fprintf(stderr, "unable to create provider\n");
                exit (1);
        }
        if ((probedef = usdt_create_probe(argv[1], argv[2], (argc-3), &argv[3])) == NULL) {
                fprintf(stderr, "unable to create probe\n");
                exit (1);
        }
        usdt_provider_add_probe(provider, probedef);

        if ((usdt_provider_enable(provider)) < 0) {
                fprintf(stderr, "unable to enable provider: %s\n", usdt_errstr(provider));
                exit (1);
        }

        if (usdt_is_enabled(probedef->probe)) {
                usdt_fire_probe(probedef->probe, (argc-3), (void **)args);
        }

        return 0;
}
