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

#endif /* DRIVERS_NUTDRV_QX_VOLTRONIC_INVERTER_H_ */
