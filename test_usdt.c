#include "usdt.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(void) {
  usdt_provider_t provider;
  usdt_probedef_t probedef[2];
  char *char_argv[2] = { "foo", "bar" };

  provider.name = "testlibusdt";
  provider.probedefs = NULL;
  provider.probes = NULL;
  provider.file = NULL;
  
  probedef[0].function = "func";
  probedef[0].name = "probe1";
  probedef[0].types[0] = USDT_ARGTYPE_STRING;
  probedef[0].types[1] = USDT_ARGTYPE_STRING;
  probedef[0].types[2] = USDT_ARGTYPE_NONE;

  probedef[1].function = "func";
  probedef[1].name = "probe2";
  probedef[1].types[0] = USDT_ARGTYPE_STRING;
  probedef[1].types[1] = USDT_ARGTYPE_STRING;
  probedef[1].types[2] = USDT_ARGTYPE_NONE;
  
  usdt_provider_add_probe(&provider, &probedef[0]);
  usdt_provider_add_probe(&provider, &probedef[1]);
  usdt_provider_enable(&provider);

  if (usdt_is_enabled(probedef[0].probe))
    usdt_fire_probe(probedef[0].probe, 2, (void **) &char_argv);

  if (usdt_is_enabled(probedef[1].probe))
    usdt_fire_probe(probedef[1].probe, 2, (void **) &char_argv);

  return 0;
}
