/* nutdrv_qx_voltronic_preprocessing.c - @TODO
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

#include "nutdrv_qx_voltronic_preprocessing.h"
#include <stdint.h>

/* validates voltronic_inverter protocol version
 * @return RC_PREPROC_SUCCESSFUL if the protocol is supported,RC_PREPROC_FAILED otherwise
 */
static int _validate_protocol_version(uint8_t protocol) {

	upslogx(LOG_DEBUG, "validating protocol v%d", protocol);

	if (protocol == 16)
		return RC_PREPROC_SUCCESSFUL;
	return RC_PREPROC_FAILED;
}

/* Protocol used by the UPS */
int	voltronic_inverter_protocol(item_t *item, char *value, size_t valuelen)
{
	const size_t identifierLen = 2;
	uint8_t protocol
	/* runtime helpers */;
	uint8_t allValuesAreNumerical = 0;
	int rc;

	if (strncasecmp(item->value, "PI", identifierLen)) {
		upsdebugx(2, "%s: invalid start characters [%.2s]", __func__, item->value);
		return RC_PREPROC_FAILED;
	}
	upslogx(LOG_DEBUG, "PI matched");

	/* Exclude non numerical value and other non accepted protocols
	 * (hence the restricted comparison target) */
	allValuesAreNumerical = strspn(item->value+identifierLen, FILTER_UINT) == strlen(item->value+identifierLen);
	if (!allValuesAreNumerical) {
		upslogx(LOG_ERR, "Protocol [%s] is not supported by this driver", item->value);
		return RC_PREPROC_FAILED;
	}

	/* convert the protocol string to int */
	protocol = strtol(item->value+identifierLen, NULL, 10);

	rc = _validate_protocol_version(protocol);
	if (rc != RC_PREPROC_SUCCESSFUL) {
		upslogx(LOG_DEBUG, "Failed to validate protocol version: %d", protocol);
		return RC_PREPROC_FAILED;
	}

	snprintf(value, valuelen, "P%02d", protocol);

	/* @note Unskipping of various items could be done at this point
	 * according to the protocol */

	return RC_PREPROC_SUCCESSFUL;
}
