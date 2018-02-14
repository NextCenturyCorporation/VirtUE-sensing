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

#define __SET_FLAG(flag, bits) ((flag) |= (bits))
#define __CLEAR_FLAG(flag, bits) ((flag) &= ~(bits))
#define __FLAG_IS_SET(flag, bits) ((flag) & (bits) ? 1 : 0)
#define __FLAG_IS_IS_CLEAR(flag, bits) ((flag) & (bits) ? 0 : 1)

#define PROBE_INITIALIZED       0x01
#define PROBE_DESTROYED         0x02
#define PROBE_KPS               0x04
#define PROBE_HAS_DATA_FIELD    0x08
#define PROBE_HAS_ID_FIELD      0x10
#define PROBE_HAS_WORK          0x20
#define PROBE_LISTENER          0x40
#define PROBE_CONNECTED         0x80

#endif
