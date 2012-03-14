#include "usdt.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  usdt_provider_t provider;
  usdt_probedef_t probedef;
  char *char_argv[6] = { "a", "b", "c", "d", "e", "f" };
  int int_argv[6] = { 1, 2, 3, 4, 5, 6 };
  void *args[6];
  int i;

  if (argc < 3) {
          fprintf(stderr, "usage: %s func name [types ...]\n", argv[0]);
          return(1);
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

  provider.name = "testlibusdt";
  provider.probedefs = NULL;
  provider.probes = NULL;
  provider.file = NULL;

  usdt_create_probe(&probedef, argv[1], argv[2], &argv[3]);
  usdt_provider_add_probe(&provider, &probedef);
  usdt_provider_enable(&provider);

  if (usdt_is_enabled(probedef.probe)) {
    fprintf(stderr, "firing\n");
    usdt_fire_probe(probedef.probe, (argc-3), (void **)args);
  }

  return 0;
}
