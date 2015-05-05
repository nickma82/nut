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

#define VOLTRONIC_INVERTER_VERSION "Voltronicinverter 0.02"

/* == qx2nut lookup table == */
static item_t	voltronic_inverter_qx2nut[] = {

	/* Query UPS for protocol
	 * > [QPI\r]
	 * < [(PI00\r]
	 *    012345
	 *    0
	 */
	{ "ups.firmware.aux",		0,	NULL,	"QPI\r",	"",	6,	'(',	"",	1,	4,	"%s",	QX_FLAG_STATIC,	NULL,	voltronic_inverter_protocol },

	/* Query UPS for firmware version
	 * > [QVFW\r]
	 * < [(VERFW:00322.02\r]
	 *    0123456789012345
	 *    0         1
	 */
	{ "ups.firmware",	0,	NULL,	"QVFW\r",	"",	16,	'(',	"",	7,	14,	"%s",	QX_FLAG_STATIC,	NULL,	NULL },

	/**
	 * > [QVFW2\r]
	 * < [(VERFW2:00000.31]
	 *    0123456789012345
	 *    0         1
	 */
	{ "ups.firmware.slave",	0,	NULL,	"QVFW2\r",	"",	13,	'(',	"",	8,	15,	"%s",	QX_FLAG_STATIC,	NULL,	NULL },


	/* status request
	 * applies for infini inverters
	 * > [QPIGS\r]
	 * < [(224.6 000275 49.9 0001.4 225.0 00275 49.9 001.2 011 438.1 438.1 052.9 ---.- 092 00014 00000 ----- 120.7 ---.- ---.- 042.0 F---101201k\x1A\r]
	 * < [(225.3 000249 50.0 0001.4 226.2 00249 50.0 001.1 010 404.7 404.7 049.7 ---.- 064 00000 00159 ----- 029.7 ---.- ---.- 040.0 D---110001\xae\xe6\r] //draining battery
	 * < [(226.1 000378 50.0 0001.7 226.8 00378 49.9 001.6 013 436.4 436.4 052.6 ---.- 077 00920 00292 ----- 196.1 ---.- ---.- 027.0 A---101001\x41\xda\r] //pass through, charging batt, pvInput active
	 * < [(225.9 000401 49.9 0001.8 226.4 00401 49.9 001.7 014 439.8 439.8 053.1 ---.- 079 01024 00300 ----- 203.1 ---.- ---.- 028.0 A---101001\x8b\x66\r]
	 *    01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123   4   5
	 *    0         1         2         3         4         5         6         7         8         9        10        11        12        13
	 *     0     1      2    3      4     5     6    7     8   9     10    11    12    13  14    15    16    17    18    19    20    21
	 */
	{ "input.voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	1,	5,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "input.power",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	7,	12,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	voltronic_inverter_sign },
	{ "input.frequency",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	14,	17,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "input.current",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	19,	24,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	voltronic_inverter_sign },
	{ "output.voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	26,	30,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "output.power", 		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	32,	36,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //trim 0
	{ "output.frequency",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	38,	41,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "output.current",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	43,	47,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "output.power_percent",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	49,	51,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //trim '0'
	{ "pbus_voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	53,	57,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "sbus_voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	59,	63,	"%.1f",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT,	NULL,	NULL },
	{ "battery.voltage",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	65,	69,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "battery.voltage_n",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	71,	75,	"%.1f",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT | QX_FLAG_SKIP,	NULL,	NULL },
	{ "battery.charge",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	77,	79,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "generator_L1_power",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	81,	85,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "generator_L2_power",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	87,	91,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "generator_L3_power",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	93,	97,	"%.1f",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT |QX_FLAG_SKIP,	NULL,	NULL },
	{ "generator_L1_voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	99,	103,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "generator_L2_voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	105,	109,	"%.1f",	QX_FLAG_QUICK_POLL |QX_FLAG_SKIP,	NULL,	NULL },
	{ "generator_L3_voltage",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	111,	115,	"%.1f",	QX_FLAG_QUICK_POLL | QX_FLAG_NONUT |QX_FLAG_SKIP,	NULL,	NULL },
	{ "ups.temperature",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	117,	121,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },

	{ "ups_status_unknown",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	123,	123,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //D..Drain Bat, F..Feeding grid,
	{ "ups_hasload",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	127,	127,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "battery_status",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	128,	129,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //0b00.. , 0b01..full, 0b10..discharge
	{ "ups_inverter_direction",	0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	130,	130,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //0..feed in, 1..
	{ "ups_line_direction",		0,	NULL,	"QPIGS\r",	"",	136,	'(',	"",	131,	132,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL }, //10..feed in, 01..take from grid

	/* Ask for battery status information
	 * applies for infini inverters
	 * > [QCHGS\r]
	 * < [(00.3 54.0 25.0 55.4\x04\xb6\r]
	 *    012345678901234567890   1   2
	 *    0         1         2
	 */
	{ "battery.current",			0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	1,	4,	"%.1f",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "battery.charging.floating_voltage",	0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	6,	9,	"%.1f",	QX_FLAG_STATIC,	NULL,	NULL },
	{ "battery.charging.max_charge_current",0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	11,	14,	"%.1f",	QX_FLAG_STATIC,	NULL,	NULL },
	{ "battery.charging.max_charge_voltage",0,	NULL,	"QCHGS\r",	"",	22,	'(',	"",	16,	19,	"%.1f",	QX_FLAG_STATIC,	NULL,	NULL },

	/* query grid output frequency
	 * [0]gridOutputHighF; [1]gridOutputLowF
	 * >[QGOF\r]
	 * <[(51.5 47.5,\r]
	 */
	{ "qgof_dummy", 			0,	NULL,	"QGOF\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/* query grid output power max power
	 * [1]maxOutputPower
	 * > [QOPMP\r]
	 * < [(03000u\r]
	 */
	{ "ups_output_power_max", 			0,	NULL,	"QOPMP\r",	"",	5,	'(',	"",	1,	5,	"%s",	QX_FLAG_STATIC,	NULL,	NULL },

	/* pull machine rating infos
	 * applies for infini inverters (P16)
	 * > [QPIRI\r]
	 * < [(230.0 50.0 013.0 230.0 013.0 18.0 048.0 1 10 0\x86\x42\r]
	 *    012345678901234567890123456789012345678901234567   8   9
	 *    0         1         2         3         4
	 */
	{ "generator_mppt_tracknumber",	0,	NULL,	"QPIRI\r",	"",	49,	'(',	"",	41,	41,	"%s",	QX_FLAG_STATIC,	NULL,	NULL },
	{ "ups_machine_type",		0,	NULL,	"QPIRI\r",	"",	49,	'(',	"",	43,	44,	"%s",	QX_FLAG_STATIC,	NULL,	NULL }, //00..grid-tie, 01..off-grid, 10..hybrid
	{ "ups_topology",		0,	NULL,	"QPIRI\r",	"",	49,	'(',	"",	46,	46,	"%s",	QX_FLAG_STATIC,	NULL,	NULL }, //0..transformerless, 1..transformer

	/* not working yet, device sends [NAKss\r]
	 * > [QPIBI\r]
	 * < []
	 *    01234567890123456789012345678901234567890123456   7
	 *    0         1         2         3         4
	 */
	{ "qpibi_dummy",		0,	NULL,	"QPIBI\r",	"",	5,	'(',	"",	1,	1,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/* unknown yet
	 * > [QPICF\r]
	 * < [(00 00\xadM\r]   //NOTE: stop @package 266
	 *    0123456   78
	 *    0
	 */
	{ "ups_fault_status",		0,	NULL,	"QPICF\r",	"",	8,	'(',	"",	1,	2,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },
	{ "ups_fault_id",		0,	NULL,	"QPICF\r",	"",	8,	'(',	"",	4,	5,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	NULL },

	/* Set battery cut off voltage
	 *  for both gridLoss and gridOn (%04.01f %04.01f)
	 * > [BSDV47,9 47,9\r]
	 * < [(ACK9\x20\r]
	 *    012345   6
	 */

	/** query max feedGrid voltage
	 * maxfeedgrid = qmpptvStr[0].substring(1)
	 * > [QGPMP\r]
	 * < [(03000u\r]
	 *    0123456
	 */
	{ "ups_maxgrid_feed_power",	0,	NULL,	"QGPMP\r",	"",	6,	'(',	"",	1,	5,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },

	/** query planID
	 * substring(1, 3))
	 * > [QPRIO\r]
	 * < [(02<\r]
	 *    01234
	 */
	{ "qprio_dummy",		0,	NULL,	"QPRIO\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query enable flags
	 * > [QENF\r]
	 * < [(A1B0C1D0E1F0G0H0I_J_�M\r]
	 *    012345678901234567890123
	 *    0         1         2
	 */
	{ "ups_en_charge_battery", 		0,	NULL,	"QENF\r",	"",	23,	'(',	"",	2,	2,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_charge_battery_fromac",	0,	NULL,	"QENF\r",	"",	23,	'(',	"",	4,	4,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_feed_into_grid",		0,	NULL,	"QENF\r",	"",	23,	'(',	"",	6,	6,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_battery_discharge_generator_on",0,	NULL,	"QENF\r",	"",	23,	'(',	"",	8,	8,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_battery_discharge_generator_off",0,	NULL,	"QENF\r",	"",	23,	'(',	"",	10,	10,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_battery_feed_generator_on",	0,	NULL,	"QENF\r",	"",	23,	'(',	"",	12,	12,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },
	{ "ups_en_battery_feed_generator_off",	0,	NULL,	"QENF\r",	"",	23,	'(',	"",	14,	14,	"%s",	QX_FLAG_SEMI_STATIC,	NULL,	NULL },


	/** query AcChargeStarttime of battery
	 * > [QPKT\r]
	 * < [(0304 0304��\r]
	 *    0123456789012
	 *    0         1
	 */
	{ "qpkt_dummy", 		0,	NULL,	"QPKT\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query floating/max charching currents
	 * > [QOFFC\r]
	 * < [(00.0 53.0 060�L\r]
	 *    01234567890123456
	 *    0         1
	 */
	{ "qoffc_dummy",		0,	NULL,	"QOFFC\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query AcChargeStarttime
	 * > [QLDT\r]
	 * < [(0000 0000e�\r]
	 *    0123456789012
	 *    0         1
	 */
	{ "qldt_dummy", 		0,	NULL,	"QLDT\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query liFeSign
	 * liFeSign = (splitArray[1] > 0) ? true : false
	 * > [QEBGP\r]
	 * < [(+000 00\r]| QX_FLAG_SKIP
	 *    012345678
	 */
	{ "qebgp_dummy", 		0,	NULL,	"QEBGP\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },


	/** query MaxAcChargingCurrent
	 * > [QACCHC\r]
	 * < [(NAKss\r]
	 */
	{ "qacchc_dummy", 		0,	NULL,	"QACCHC\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query high/low value
	 * > [QDI\r]
	 * < [(264.5 184.0 51.5 47.5 264.5 184.0 51.5 47.5 500 090 450 120 03000 253 02 04 --- --7�\r]
	 *    01234567890123456789012345678901234567890123456789012345678901234567890123456789012345
	 *    0         1         2         3         4         5         6         7         8
	 */
	{ "qdi_dummy", 			0,	NULL,	"QDI\r",	"",	5,	'(',	"",	0,	0,	"%s",	QX_FLAG_STATIC | QX_FLAG_SKIP,	NULL,	NULL },

	/** query device model type
	 * > [QDM\r]
	 * < [(058:\x??\r]
	 *    012345   6
	 */
	{ "device.model", 			0,	NULL,	"QDM\r",	"",	5,	'(',	"",	1,	2,	"%s",	QX_FLAG_STATIC,	NULL,	NULL },

	/* Query UPS for actual working mode
	 * @note inverter answers have a different answer length than other voltronic UPSs
	 * > [QMOD\r]
	 * < [(G\xb6l\r] //pass through, charging batt, pvInput active
	 * < [(G\xb7l\r] //pass through, charging batt, pvInput active
	 *    012   34
	 */
	{ "ups.alarm",		0,	NULL,	"QMOD\r",	"",	4,	'(',	"",	1,	1,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	voltronic_inverter_mode },
	{ "ups.status",		0,	NULL,	"QMOD\r",	"",	4,	'(',	"",	1,	1,	"%s",	QX_FLAG_QUICK_POLL,	NULL,	voltronic_inverter_mode },

	/* request device system time
	 * > [QT\r]
	 * < [(20150321210652\xe9\x1e\r
	 *    0123456789012345   6   7
	 *    0         1
	 */
	{ "ups.date",		0,	NULL,	"QT\r",	"",	17,	'(',	"",	1,	8,	"%s",	0,	NULL,	NULL },
	{ "ups.time",		0,	NULL,	"QT\r",	"",	17,	'(',	"",	9,	14,	"%s",	0,	NULL,	NULL },

	/* Query UPS for serial number
	 * > [QID\r]
	 * < [(12345679012345\r]	<- As far as I know it hasn't a fixed length -> min length = ( + \r = 2
	 *    0123456789012345
	 *    0         1
	 */
	{ "device.serial",	0,	NULL,	"QID\r",	"",	2,	'(',	"",	1,	0,	"%s",	QX_FLAG_STATIC,	NULL,	voltronic_serial_numb },

	/* Instant commands */
	/* Set AcoutputStarttime and AcoutputEndtime (formate is hhmm)
	 * > [LDT0102 0102\r]
	 * < []
	 */
	{ "ac_output_timerange", 	0,	NULL,	"LDT%04d %04d\r",	"",	5,	'(',	"",	1,	2,	"%s",	QX_FLAG_CMD | QX_FLAG_SKIP,	NULL,	NULL },
	/* Set AcChargingStarttime and AcChargingStoptime (formate is hhmm)
	 * > [PKT0102 0102\r]
	 * < []
	 */
	{ "ac_charge_timerange", 	0,	NULL,	"PKT%04d %04d\r",	"",	5,	'(',	"",	1,	2,	"%s",	QX_FLAG_CMD | QX_FLAG_SKIP,	NULL,	NULL },

	/* enable commands */
	/* Allow AC to charge battery - set enable feed battery from grid (ac charge)
	 * > [ENFBn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.feedfromac",	0,	NULL,	"ENFB%d\r",	"",	5,	'(',	"",	1,	2,	"%s",	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },
	/* Allow battery to discharge when PV is unavailable (set bat Dis Pv Loss)
	 * > [ENFEn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.enfe",	0,	NULL,	"ENFE%d\r",	"",	5,	'(',	"",	1,	2,	NULL,	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },
	/* Allow battery to discharge when PV is available (set bat Dis Pv On)
	 * > [ENFDn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.enfd",	0,	NULL,	"ENFD%d\r",	"",	5,	'(',	"",	1,	2,	NULL,	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },
	/* Allow (PV) to feed-in to Grid (Pv Feed Grid)
	 * > [ENFCn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.enfc",	0,	NULL,	"ENFC%d\r",	"",	5,	'(',	"",	1,	2,	NULL,	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },
	/* Allow battery to feed-in to the Grid when PV is available (bat Feed Pv On)
	 * > [ENFFn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.enff",	0,	NULL,	"ENFF%d\r",	"",	5,	'(',	"",	1,	2,	NULL,	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },
	/* Allow battery to feed-in to the Grid when PV is unavailable (set bat Feed pv loss)
	 * > [ENFGn\r]
	 * < [(ACK9\x20\r
	 */
	{ "battery.enfg",	0,	NULL,	"ENFG%d\r",	"",	5,	'(',	"",	1,	2,	NULL,	QX_FLAG_CMD,	NULL,	voltronic_inverter_cmd_boolinput },


	/* ask for generated power for a specific day
	 * > [QED20150322105\r]
	 * < [(005758\x43\x9b\r]
	 * > [QED20150323106\r]
	 * < [(020922\x98\x81\r]
	 *    01234567   8   9
	 */
	//{ "ups.generated.daily", 0,	NULL,	"QED%s\r",	"",	5,	'(',	"",	1,	6,	"%s",	QX_FLAG_CMD,	NULL,	voltronic_inverter_qe },

	{ "load.off",		0,	NULL,	"SOFF\r",	"",	5,	'(',	"",	1,	3,	NULL,	QX_FLAG_CMD,	NULL,	NULL },
	{ "load.on",		0,	NULL,	"SON\r",	"",	5,	'(',	"",	1,	3,	NULL,	QX_FLAG_CMD,	NULL,	NULL },
	/* End of structure. */
	{ NULL,			0,	NULL,	NULL,	"",	0,	0,	"",	0,	0,	NULL,	0,	NULL,	NULL }
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

	/* set to Solar Controller Devices */
	dstate_setinfo("device.type", "scd");

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
