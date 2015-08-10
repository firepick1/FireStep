#ifndef PROGMEM_H
#define PROGMEM_H

#ifdef CMAKE
#include <cstring>
#include <cstdio>
#endif

#include "Status.h"
#include "JsonCommand.h"

namespace firestep {

const char *prog_src(const char *name);	
Status prog_dump(const char *name);
Status prog_load_cmd(const char *name, JsonCommand &jcmd);

} // namespace firestep


////////////////////// PROGMEM_H //////////////
#endif
