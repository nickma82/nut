/* nutdrv_qx_voltronic_inverter.h - @TODO
 *
 * Copyright (C)
 *   2015 Nick Mayerhofer <nick.mayerhofer@enchant.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef DRIVERS_NUTDRV_QX_VOLTRONIC_INVERTER_H_
#define DRIVERS_NUTDRV_QX_VOLTRONIC_INVERTER_H_

#include "nutdrv_qx.h"

#ifndef RC_FAILED
	#define RC_FAILED 	0
#endif
#ifndef RC_SUCCESSFUL
	#define RC_SUCCESSFUL 	1
#endif

extern subdriver_t	voltronic_inverter_subdriver;

/* e.g.: instead of the following
 *	{ "input.voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	1,	5,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
 * one is able to use something like the following:
 *	ITEM_QUICKPOLL_BARE("input.voltage",	"QPIGS\r",	136,	'(',	1,	5,	"%.1f"),
 */
#define ITEM_QUICKPOLL_BARE(info_type, command, answer_len, leading, from, to, dfl) \
	{ info_type, 0, NULL, command, "", answer_len, leading,	"",	from, to,	dfl,	QX_FLAG_QUICK_POLL,	NULL,	NULL }

#define ITEM_QPIGS(info_type, from, to, dfl) ITEM_QUICKPOLL_BARE(info_type, "QPIGS\r", 136, '(', from, to, dfl)

#endif /* DRIVERS_NUTDRV_QX_VOLTRONIC_INVERTER_H_ */
