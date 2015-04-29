/* nutdrv_qx_voltronic_preprocessing.h - @TODO
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

#ifndef DRIVERS_NUTDRV_QX_VOLTRONIC_PREPROCESSING_H_
#define DRIVERS_NUTDRV_QX_VOLTRONIC_PREPROCESSING_H_

#define RC_PREPROC_FAILED 	-1
#define RC_PREPROC_SUCCESSFUL 	0

#define FILTER_UINT	"0123456789"
#define FILTER_FLOAT	"0123456789."

#include <stdlib.h>
#include "common.h"
#include "nutdrv_qx.h"

/**
 * Is used to dynamically process QE{D,M,Y} commands
 *
 * generates the string requested in item->command using
 * the date (held by *value), a checksum and copies the result back into *value
 *
 * @var[in]	*item especially item->command is used as format option
 * @var[inout]	value input date(format: yyyyMMdd); output <command><date><checksum><\r>
 */
int voltronic_inverter_qe(const item_t *item, char *value, size_t valuelen);

/* Preprocessing methods prototypes */
int	voltronic_inverter_protocol(item_t *item, char *value, size_t valuelen);

/** Preprocesses the UPS serial number
 *
 * @var[in] 		item item to be processed
 * @var[in] value	pointer to the value stream
 * @var[in] valuelen	value stream length
 */
int	voltronic_serial_numb(item_t *item, char *value, size_t valuelen);


int	voltronic_inverter_mode(item_t *item, char *value, size_t valuelen);

/**
 * Processes item->value and saves the processed value in *value.
 *
 * - Processing it in a way that leading zeros are trimmed and if
 *     item->value[0] not equal to '0', values is handeled as negative.
 * - strlen(item->value) must be at lest of size two {<pos/neg>, <value>}
 *
 * @var[in]	item  		carrying item->answer from device to be processed
 * @var[out]	value		output processed value
 * @var[in]	valuelen	maximum value buffer length
 */
int	voltronic_inverter_sign(item_t *item, char *value, const size_t valuelen);

#endif /* DRIVERS_NUTDRV_QX_VOLTRONIC_PREPROCESSING_H_ */
