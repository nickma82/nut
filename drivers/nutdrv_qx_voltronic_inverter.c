/* nutdrv_qx_voltronic.c - Subdriver for Voltronic Power UPSes
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

/**
 * @note Voltronic inverters are "Solar Controller Devices".
 *
 * for further explanations about the implementation see:
 * http://www.networkupstools.org/docs/developer-guide.chunked/ar01s04.html#nutdrv_qx-subdrivers
 *
 * Variable name sheme:
 * http://www.networkupstools.org/docs/developer-guide.chunked/apa.html
 */

#include "main.h"
#include "nutdrv_qx.h"
#include "nutdrv_qx_voltronic_inverter.h"

#include "nutdrv_qx_voltronic_preprocessing.h"

#define VOLTRONIC_INVERTER_VERSION "Voltronicinverter 0.01"

/* == qx2nut lookup table == */
static item_t	voltronic_inverter_qx2nut[] = {

	/* Query UPS for protocol
	 * > [QPI\r]
	 * < [(PI00\r]
	 *    012345
	 *    0
	 */
	{ "ups.firmware.aux",		0,	NULL,	"QPI\r",	"",	6,	'(',	"",	1,	4,	"%s",	QX_FLAG_STATIC,	NULL,	voltronic_inverter_protocol },

	/* status request
	 * applies for infini inverters
	 * > [QPIGS\r]
	 * < [(224.6 000275 49.9 0001.4 225.0 00275 49.9 001.2 011 438.1 438.1 052.9 ---.- 092 00014 00000 ----- 120.7 ---.- ---.- 042.0 F---101201k\x1A\r]
	 * < [(225.3 000249 50.0 0001.4 226.2 00249 50.0 001.1 010 404.7 404.7 049.7 ---.- 064 00000 00159 ----- 029.7 ---.- ---.- 040.0 D---110001\xae\xe6\r] //draining battery
	 * < [(226.1 000378 50.0 0001.7 226.8 00378 49.9 001.6 013 436.4 436.4 052.6 ---.- 077 00920 00292 ----- 196.1 ---.- ---.- 027.0 A---101001\x41\xda\r] //pass through, charging batt, pvInput active
	 * < [(225.9 000401 49.9 0001.8 226.4 00401 49.9 001.7 014 439.8 439.8 053.1 ---.- 079 01024 00300 ----- 203.1 ---.- ---.- 028.0 A---101001\x8b\x66\r]
	 *    01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123   4   5
	 *    0         1         2         3         4         5         6         7         8         9        10        11        12        13
	 */
	{ "input.voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	1,	5,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	0,	0,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	0,	0,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	0,	0,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	0,	0,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	{ "outlet.realpower",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	32,	36,	"%d",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, // 7-12 vs 32-36
	{ "outlet.frequency",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	38,	41,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, // 14-17 vs 38-41
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	43,	47,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },*/
	{ "outlet.power.percent",0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	49,	51,	"%d",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	53,	57,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	59,	63,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	{ "battery.voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	65,	69,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "battery.capacity.percent",0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	77,	79,	"%d",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT,	NULL,	NULL },
	{ "generator_power",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	81,	85,	"%d",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT,	NULL,	NULL },
	/*{ "unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	87,	91,	"%d",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, */
	{ "generator.voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	99,	103,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "ups.temperature",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	117,	121,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },

	/* Ask for battery status information
	 * applies for infini inverters
	 * > [QCHGS\r]
	 * < [(00.3 54.0 25.0 55.4\x04\xb6\r]
	 *    012345678901234567890   1   2
	 *    0         1         2
	 */
	{ "battery.charging.current",		0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	1,	4,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //CHECK AGAIN
	{ "battery.charging.floating_voltage",	0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	6,	9,	"%.1f",	QX_FLAG_STATIC |QX_FLAG_NONUT,	NULL,	NULL }, //CHECK AGAIN
	{ "battery.charging.max_charge_current",0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	11,	14,	"%.1f",	QX_FLAG_STATIC |QX_FLAG_NONUT,	NULL,	NULL }, //CHECK AGAIN
	{ "battery.charging.bulk_voltage",	0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	16,	19,	"%.1f",	QX_FLAG_STATIC |QX_FLAG_NONUT,	NULL,	NULL },

	/* MAYBE allow battery to discharge when generator is unavailable
	 * applies for infini inverters
	 * > [LDT01020102\r]
	 * < [(ACK9\x20\r]
	*/

	/* ask for generated power for a specific day
	 * > [QED20150322105\r]
	 * < [(005758\x43\x9b\r]
	 * > [QED20150323106\r]
	 * < [(020922\x98\x81\r]
	 *    01234567   8   9
	 */

	/* unknown yet
	 * applies for infini inverters (P16)
	 * > [QPIRI\r]
	 * < [(230.0 50.0 013.0 230.0 013.0 18.0 048.0 1 10 0\x86\x42\r]
	 *    012345678901234567890123456789012345678901234567   8   9
	 *    0         1         2         3         4
	 */


	/* unknown yet
	 * applies for infini inverters (P16)
	 * > [QPICF\r]
	 * < [(00 00\xadM\r]   //NOTE: stop @package 266
	 *    0123456   78
	 *    0
	 */

	/* Set battery cut off voltage when grid is available
	 * > [BSDV47,9 47,9\r]
	 * < [(ACK9\x20\r]
	 *    012345   6
	 */

	/* unknown yet, maybe mode?
	 * > [QMOD\r]
	 * < [(G\xb6l\r] //pass through, charging batt, pvInput active
	 * < [(G\xb7l\r] //pass through, charging batt, pvInput active
	 *    0123  45
	 */

	/* request device system time
	 * applies for infini inverters (P16)
	 * > [QT\r]
	 * < [(20150321210652\xe9\x1e\r
	 *    0123456789012345   6   7
	 *    0         1
	 */
	{ "ups.date",	0,	NULL,	"QT\r",	"",	17,	'(',	"",	1,	8,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "ups.time",	0,	NULL,	"QT\r",	"",	17,	'(',	"",	9,	14,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },

	/* End of structure. */
	{ NULL,		0,	NULL,	NULL,	"",	0,	0,	"",	0,	0,	NULL,	0,	NULL,	NULL }
};

/* == Testing table == */
#ifdef TESTING
static testing_t	voltronic_inverter_testing[] = {
	{ "QPI\r",	"(PI01\r",	-1 },
	{ "QPIGS\r",	"(224.6 000275 49.9 0001.4 225.0 00275 49.9 001.2 011 438.1 438.1 052.9 ---.- 092 00014 00000 ----- 120.7 ---.- ---.- 042.0 F---101201k\x1A\r", -1 },
	{ NULL }
};
#endif	/* TESTING */

int voltronic_inverter_claim(void)
{
	upslogx(LOG_INFO, "trying to claim an voltronic_inverter device");
	/* UPS Protocol */
	item_t *item = find_nut_info("ups.firmware.aux", 0, 0);
	if (!item) {
		upslogx(LOG_ERR, "Unable to find ups.firmware.aux in nut_info");
		return RC_FAILED;
	}

	/* No reply/Unable to get value */
	if (qx_process(item, NULL) != 0) {
		upslogx(LOG_ERR, "Unable to initially communicate with the target (Target asnwered: %s)", item->answer);
		return RC_FAILED;
	}

	if (ups_infoval_set(item) != 1) {
		/* Unable to process value/Protocol out of range */
		return RC_FAILED;
	}

	upslogx(LOG_INFO, "successfuly claimed voltronic inverter");
	return RC_SUCCESSFUL;
}

/* Subdriver-specific flags/vars */
static void	voltronic_makevartable(void)
{
	/* For testing purposes */
	addvar(VAR_FLAG, "testing", "If invoked the driver will exec also commands that still need testing...");
}

/* == Subdriver interface == */
subdriver_t	voltronic_inverter_subdriver = {
	VOLTRONIC_INVERTER_VERSION,
	voltronic_inverter_claim,
	voltronic_inverter_qx2nut,
	NULL,
	NULL,
	voltronic_makevartable,
	"ACK",
	"(NAK\r",
#ifdef TESTING
	voltronic_inverter_testing,
#endif	/* TESTING */
};
