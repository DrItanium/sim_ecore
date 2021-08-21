//
// Created by jwscoggins on 8/21/21.
//

#ifndef SIM3_SBCORE_H
#define SIM3_SBCORE_H
#ifdef DESKTOP_BUILD
#include "DesktopSBCore.h"
#elif defined(ARDUINO)
#include "SBCoreArduino.h"
#else
#error "NO VALID SBCORE IMPLEMENTATION FOR GIVEN TARGET"
#endif
#endif //SIM3_SBCORE_H
