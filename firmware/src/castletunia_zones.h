/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CASTLETUNIA_ZONES_H
#define CASTLETUNIA_ZONES_H

#include "castletunia_types.h"

extern const ZoneDef G_ZONES[NUM_ZONES];

const ZoneDef *zone_get(uint8_t zone_id);

#endif /* CASTLETUNIA_ZONES_H */
