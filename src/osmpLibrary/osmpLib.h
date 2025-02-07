/******************************************************************************
 * FILE: osmpLib.h
 * AUTHOR: Darius Malysiak
 * DESCRIPTION:
 * In dieser Datei sind zusätzliche (eigene) Hilfsfunktionen für die **interne**
 * Verwendung der OSMP-Bibliothek definiert.
 ******************************************************************************/

#ifndef OSMP_LIB_H
#define OSMP_LIB_H

#include "OSMP.h"

/**
 * @brief Dieses Makro wird verwendet, um den Compiler davon zu überzeugen, dass
 * eine Variable verwendet wird.
 *
 * @param x Die zu verwendende Variable
 */
#define UNUSED(x) \
  { (void)(x); }

#endif  // OSMP_LIB_H
