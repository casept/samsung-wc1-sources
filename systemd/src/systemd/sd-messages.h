/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#ifndef foosdmessageshfoo
#define foosdmessageshfoo

/***
  This file is part of systemd.

  Copyright 2012 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include "sd-id128.h"
#include "_sd-common.h"

_SD_BEGIN_DECLARATIONS;

/* Hey! If you add a new message here, you *must* also update the
 * message catalog with an appropriate explanation */

/* And if you add a new ID here, make sure to generate a random one
 * with journalctl --new-id128. Do not use any other IDs, and do not
 * count them up manually. */

#define SD_MESSAGE_JOURNAL_START    SD_ID128_MAKE(f7,73,79,a8,49,0b,40,8b,be,5f,69,40,50,5a,77,7b)
#define SD_MESSAGE_JOURNAL_STOP     SD_ID128_MAKE(d9,3f,b3,c9,c2,4d,45,1a,97,ce,a6,15,ce,59,c0,0b)
#define SD_MESSAGE_JOURNAL_DROPPED  SD_ID128_MAKE(a5,96,d6,fe,7b,fa,49,94,82,8e,72,30,9e,95,d6,1e)
#define SD_MESSAGE_JOURNAL_MISSED   SD_ID128_MAKE(e9,bf,28,e6,e8,34,48,1b,b6,f4,8f,54,8a,d1,36,06)
#define SD_MESSAGE_JOURNAL_USAGE    SD_ID128_MAKE(ec,38,7f,57,7b,84,4b,8f,a9,48,f3,3c,ad,9a,75,e6)

#define SD_MESSAGE_COREDUMP         SD_ID128_MAKE(fc,2e,22,bc,6e,e6,47,b6,b9,07,29,ab,34,a2,50,b1)

#define SD_MESSAGE_SESSION_START    SD_ID128_MAKE(8d,45,62,0c,1a,43,48,db,b1,74,10,da,57,c6,0c,66)
#define SD_MESSAGE_SESSION_STOP     SD_ID128_MAKE(33,54,93,94,24,b4,45,6d,98,02,ca,83,33,ed,42,4a)
#define SD_MESSAGE_SEAT_START       SD_ID128_MAKE(fc,be,fc,5d,a2,3d,42,80,93,f9,7c,82,a9,29,0f,7b)
#define SD_MESSAGE_SEAT_STOP        SD_ID128_MAKE(e7,85,2b,fe,46,78,4e,d0,ac,cd,e0,4b,c8,64,c2,d5)
#define SD_MESSAGE_MACHINE_START    SD_ID128_MAKE(24,d8,d4,45,25,73,40,24,96,06,83,81,a6,31,2d,f2)
#define SD_MESSAGE_MACHINE_STOP     SD_ID128_MAKE(58,43,2b,d3,ba,ce,47,7c,b5,14,b5,63,81,b8,a7,58)

#define SD_MESSAGE_TIME_CHANGE      SD_ID128_MAKE(c7,a7,87,07,9b,35,4e,aa,a9,e7,7b,37,18,93,cd,27)
#define SD_MESSAGE_TIMEZONE_CHANGE  SD_ID128_MAKE(45,f8,2f,4a,ef,7a,4b,bf,94,2c,e8,61,d1,f2,09,90)

#define SD_MESSAGE_STARTUP_FINISHED SD_ID128_MAKE(b0,7a,24,9c,d0,24,41,4a,82,dd,00,cd,18,13,78,ff)

#define SD_MESSAGE_SLEEP_START      SD_ID128_MAKE(6b,bd,95,ee,97,79,41,e4,97,c4,8b,e2,7c,25,41,28)
#define SD_MESSAGE_SLEEP_STOP       SD_ID128_MAKE(88,11,e6,df,2a,8e,40,f5,8a,94,ce,a2,6f,8e,bf,14)

#define SD_MESSAGE_SHUTDOWN         SD_ID128_MAKE(98,26,88,66,d1,d5,4a,49,9c,4e,98,92,1d,93,bc,40)

#define SD_MESSAGE_UNIT_STARTING    SD_ID128_MAKE(7d,49,58,e8,42,da,4a,75,8f,6c,1c,dc,7b,36,dc,c5)
#define SD_MESSAGE_UNIT_STARTED     SD_ID128_MAKE(39,f5,34,79,d3,a0,45,ac,8e,11,78,62,48,23,1f,bf)
#define SD_MESSAGE_UNIT_STOPPING    SD_ID128_MAKE(de,5b,42,6a,63,be,47,a7,b6,ac,3e,aa,c8,2e,2f,6f)
#define SD_MESSAGE_UNIT_STOPPED     SD_ID128_MAKE(9d,1a,aa,27,d6,01,40,bd,96,36,54,38,aa,d2,02,86)
#define SD_MESSAGE_UNIT_FAILED      SD_ID128_MAKE(be,02,cf,68,55,d2,42,8b,a4,0d,f7,e9,d0,22,f0,3d)
#define SD_MESSAGE_UNIT_RELOADING   SD_ID128_MAKE(d3,4d,03,7f,ff,18,47,e6,ae,66,9a,37,0e,69,47,25)
#define SD_MESSAGE_UNIT_RELOADED    SD_ID128_MAKE(7b,05,eb,c6,68,38,42,22,ba,a8,88,11,79,cf,da,54)

#define SD_MESSAGE_SPAWN_FAILED     SD_ID128_MAKE(64,12,57,65,1c,1b,4e,c9,a8,62,4d,7a,40,a9,e1,e7)

#define SD_MESSAGE_FORWARD_SYSLOG_MISSED SD_ID128_MAKE(00,27,22,9c,a0,64,41,81,a7,6c,4e,92,45,8a,fa,2e)

#define SD_MESSAGE_OVERMOUNTING     SD_ID128_MAKE(1d,ee,03,69,c7,fc,47,36,b7,09,9b,38,ec,b4,6e,e7)

#define SD_MESSAGE_LID_OPENED       SD_ID128_MAKE(b7,2e,a4,a2,88,15,45,a0,b5,0e,20,0e,55,b9,b0,6f)
#define SD_MESSAGE_LID_CLOSED       SD_ID128_MAKE(b7,2e,a4,a2,88,15,45,a0,b5,0e,20,0e,55,b9,b0,70)
#define SD_MESSAGE_SYSTEM_DOCKED    SD_ID128_MAKE(f5,f4,16,b8,62,07,4b,28,92,7a,48,c3,ba,7d,51,ff)
#define SD_MESSAGE_SYSTEM_UNDOCKED  SD_ID128_MAKE(51,e1,71,bd,58,52,48,56,81,10,14,4c,51,7c,ca,53)
#define SD_MESSAGE_POWER_KEY        SD_ID128_MAKE(b7,2e,a4,a2,88,15,45,a0,b5,0e,20,0e,55,b9,b0,71)
#define SD_MESSAGE_SUSPEND_KEY      SD_ID128_MAKE(b7,2e,a4,a2,88,15,45,a0,b5,0e,20,0e,55,b9,b0,72)
#define SD_MESSAGE_HIBERNATE_KEY    SD_ID128_MAKE(b7,2e,a4,a2,88,15,45,a0,b5,0e,20,0e,55,b9,b0,73)

#define SD_MESSAGE_CONFIG_ERROR     SD_ID128_MAKE(c7,72,d2,4e,9a,88,4c,be,b9,ea,12,62,5c,30,6c,01)

#define SD_MESSAGE_BOOTCHART        SD_ID128_MAKE(9f,26,aa,56,2c,f4,40,c2,b1,6c,77,3d,04,79,b5,18)

_SD_END_DECLARATIONS;

#endif
