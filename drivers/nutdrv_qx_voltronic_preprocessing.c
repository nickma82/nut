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
static int _validate_protocol_version(uint8_t protocol)
{
	upslogx(LOG_DEBUG, "validating protocol v%d", protocol);

	if (protocol == 16)
		return RC_PREPROC_SUCCESSFUL;
	return RC_PREPROC_FAILED;
}

/**
 * calculates checksums atm just seen at {QED, QEY, QEM} commands
 * getCheckum("QED20150322", 11, chk, chkMaxSize); //expected: 105
 * getCheckum("QED20150323", 11, chk, chkMaxSize); //expected: 106
 */
static void _getCheckum(const char* value, const size_t valueLen, char* chksum, size_t chksumMaxSize)
{
	uint16_t temp = 0;
	size_t i = 0;

	for (i = 0; i < valueLen; ++i) {
		temp += value[i];
	}
	temp &= 255;

	snprintf(chksum, chksumMaxSize, "%d", temp);
}

int voltronic_inverter_qe(item_t *item, char *value, size_t valuelen)
{
	const size_t expected_dateString_length = 8; //yyyyMMdd
	char buf[SMALLBUF] = "\0";

	upsdebugx(LOG_DEBUG, "%s: processing item->info_type:%s, value:%s, valuelen:%zu",
			__func__, item->info_type, value, valuelen);

	if (strlen(value) != strspn(value, FILTER_UINT)) {
		upslogx(LOG_ERR, "%s: non numerical value [%s]",
				item->info_type, value);
		return RC_PREPROC_FAILED;
	}

	if (strlen(value) != expected_dateString_length) {
		upslogx(LOG_ERR, "%s: expected a string formated like yyyyMMdd, actually got: %s",
				__func__, value);
		return RC_PREPROC_FAILED;
	}

	upsdebugx(LOG_DEBUG, "valuelen before: %zu", strlen(value));
	snprintf(value, valuelen, item->command, value);
	upsdebugx(LOG_DEBUG, "valuelen after: %zu", strlen(value));

	_getCheckum(value, strlen(value), buf, sizeof(buf)/sizeof(buf[0]) );
	snprintf(value, valuelen, "%s%s", value, buf);
	upsdebugx(LOG_DEBUG, "valuelen after2: %zu, value: %s", strlen(value), value);

	return RC_PREPROC_SUCCESSFUL;
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
		upsdebugx(LOG_DEBUG, "%s: invalid start characters [%.2s]", __func__, item->value);
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

int	voltronic_serial_numb(item_t *item, char *value, size_t valuelen)
{
	/* If the UPS report a 00..0 serial we'll log it but we won't store it in device.serial */
	if (strspn(item->value, "0") == strlen(item->value)) {
		upslogx(LOG_INFO, "%s: UPS reported a non-significant serial [%s]", item->info_type, item->value);
		return RC_PREPROC_FAILED;
	}

	snprintf(value, valuelen, item->dfl, item->value);
	return RC_PREPROC_SUCCESSFUL;
}

/* Working mode reported by the UPS */
int	voltronic_inverter_mode(item_t *item, char *value, size_t valuelen)
{
	char	*status = NULL, *alarm = NULL;

	switch (item->value[0])
	{
	case 'P':

		alarm = "UPS is going ON";
		break;

	case 'S':

		status = "OFF";
		break;

	case 'Y':

		status = "BYPASS";
		break;

	case 'L':

		status = "OL";
		break;

	case 'B':

		status = "!OL";
		break;

	case 'T':

		status = "CAL";
		break;

	case 'F':

		alarm = "Fault reported by UPS.";
		break;

	case 'E':

		alarm = "UPS is in ECO Mode.";
		break;

	case 'C':

		alarm = "UPS is in Converter Mode.";
		break;

	case 'D':

		alarm = "UPS is shutting down!";
		status = "FSD";
		break;

	case 'G':
		/* maybe state generating */
		status = "OL";
		break;

	default:

		upsdebugx(2, "%s: invalid reply from the UPS [%s]", __func__, item->value);
		return RC_PREPROC_FAILED;

	}

	/* decision where to save the obtained value, because this method supports different
	 * item->info_typs's */
	if (alarm && !strcasecmp(item->info_type, "ups.alarm")) {

		snprintf(value, valuelen, item->dfl, alarm);

	} else if (status && !strcasecmp(item->info_type, "ups.status")) {

		snprintf(value, valuelen, item->dfl, status);

	}

	return RC_PREPROC_SUCCESSFUL;
}

int	voltronic_inverter_sign(item_t *item, char *value, size_t valuelen) {
	const size_t minimum_supported_length = 2;
	const char staypositive_char = '0';

	char* payload = item->value;
	size_t leadin_zero_length = 0;
	size_t payload_length = (strlen(payload) < valuelen) ? strlen(payload) : valuelen;

	if (payload_length < minimum_supported_length) {
		upsdebugx(LOG_ERR, "%s: length [%zu] insufficient (strlen(item->value):%zu, valuelen:%zu): %s",
				__func__, payload_length, strlen(item->value), valuelen, item->value);
		return RC_PREPROC_FAILED;
	}

	/* check item->value validity*/
	if (strspn(payload, FILTER_FLOAT) != payload_length) {
		upsdebugx(LOG_ERR, "%s: non numerical item->value [%s: %s]", __func__, item->info_type, payload);
		return RC_PREPROC_FAILED;
	}

	/* process negative indicator */
	if (*payload != staypositive_char) {
		*(value++) = '-';
		payload++;
		payload_length--;
	}

	/* move to MSB position 0's, fill everything else with \0 */
	leadin_zero_length = strspn(payload, "0");
	if (leadin_zero_length) {
		payload_length -= leadin_zero_length;

		upsdebugx(LOG_DEBUG, "%s: item->value:%s", __func__, item->value);
		snprintf(value, payload_length+1, "%s", payload+leadin_zero_length);
	}

	return RC_PREPROC_SUCCESSFUL;
}
