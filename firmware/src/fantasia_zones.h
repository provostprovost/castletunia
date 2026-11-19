/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FANTASIA_ZONES_H
#define FANTASIA_ZONES_H

#include "fantasia_types.h"

extern const ZoneDef G_ZONES[NUM_ZONES];

const ZoneDef *zone_get(uint8_t zone_id);

#endif /* FANTASIA_ZONES_H */
