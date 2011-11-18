#include "usdt.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(void) {
  usdt_provider_t provider;
  usdt_probedef_t probedef;
  char *char_argv[2] = { "foo", "bar" };

  provider.name = "testlibusdt";
  provider.probedefs = NULL;
  provider.probes = NULL;
  provider.file = NULL;
  
  probedef.function = "func";
  probedef.name = "myprobe";

  probedef.types[0] = USDT_ARGTYPE_STRING;
  probedef.types[1] = USDT_ARGTYPE_STRING;
  probedef.types[2] = USDT_ARGTYPE_NONE;
  
  usdt_provider_add_probe(&provider, &probedef);
  usdt_provider_enable(&provider);

  if (usdt_is_enabled(probedef.probe))
    usdt_fire_probe(probedef.probe, 2, (void **) &char_argv);

  return 0;
}
