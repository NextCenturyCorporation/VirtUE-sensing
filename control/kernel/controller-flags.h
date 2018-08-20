/*******************************************************************
 * in-Virtue Kernel Controller
 *
 * Copyright (C) 2017-2018  Michael D. Day II
 * Copyright (C) 2017-2018  Two Six Labs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *******************************************************************/

#ifndef _CONTROLLER_FLAGS_H
#define _CONTROLLER_FLAGS_H

/**
 * TODO:  https://lwn.net/Articles/588444/
 * checking for unused flags
 **/

#define __SET_FLAG(flag, bits) ((flag) |= (bits))
#define __CLEAR_FLAG(flag, bits) ((flag) &= ~(bits))
#define __FLAG_IS_SET(flag, bits) ((flag) & (bits) ? 1 : 0)
#define __FLAG_IS_IS_CLEAR(flag, bits) ((flag) & (bits) ? 0 : 1)

/**
 * sensor flags, sensor.flags
 **/
#define SENSOR_INITIALIZED       0x01
#define SENSOR_DESTROYED         0x02
#define SENSOR_UNUSED            0x08
#define SENSOR_HAS_NAME_FIELD    0x10
#define SENSOR_HAS_WORK          0x20
#define SENSOR_LISTEN            0x40
#define SENSOR_CONNECT           0x80

#define SENSOR_KPS               0x04
#define SENSOR_KLSOF             0x100
#define SENSOR_KSYSFS            0x200

#endif
