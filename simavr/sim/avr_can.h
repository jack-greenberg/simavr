/*
    avr_can.h
    Copyright 2022 Jack Greenberg <j@jackgreenberg.co>
    This file is part of simavr.
    simavr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    simavr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __AVR_CAN_H__
#define __AVR_CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr.h"

#include <stdint.h>
#include <stdbool.h>

#define AVR_IOCTL_CAN_GETIRQ() AVR_IOCTL_DEF('c', 'a', 'n', ' ')

enum {
	CAN_IRQ_TX,
	CAN_IRQ_RX,
    CAN_IRQ_MOBNB,
	CAN_IRQ_COUNT
};

#define CANMOB(_mobnb) { \
    .enabled = AVR_IO_REGBIT(CANEN2, ENMOB ## _mobnb), \
}

enum {
    MOB_MODE_DISABLE = 0x00u,
    MOB_MODE_ENABLE_TX,
    MOB_MODE_ENABLE_RX,
    MOB_MODE_ENABLE_FRAME_BUFFER_RX,
};

typedef struct {
    uint8_t mobnb;
    uint16_t id;
    uint16_t mask;
    uint8_t data[8];
    uint8_t dlc;
}  avr_can_frame_t;

#define AVR_IOCTL_CAN_SEND	AVR_IOCTL_DEF('c','a','n','s')
#define AVR_IOCTL_CAN_RECEIVE	AVR_IOCTL_DEF('c','a','n','r')

/*
 * Need to monitor
 *   DLC
 *   ID
 *   MSK
 *   Data
 *   cfg/mode
 *
 *   baud rade
 */
typedef struct avr_can_mob_t {
    avr_regbit_t enabled;

    uint16_t id;
    uint16_t mask;

    bool ainc;
    uint8_t data[8];
    uint8_t data_indx;
    uint8_t dlc;

    uint8_t mode; // enum-ify?
} avr_can_mob_t;

typedef struct avr_can_t {
    avr_io_t io;

    avr_regbit_t txbsy;
    avr_regbit_t rxbsy;

    avr_regbit_t mob;

    avr_io_addr_t canmsg; // Data
    avr_io_addr_t cancdmob;

    // MOb specific
    avr_regbit_t mode;
    avr_regbit_t dlc;
    avr_regbit_t ainc;
    avr_regbit_t indx;
    avr_io_addr_t canidt1, canidt2, canidt3, canidt4;
    avr_io_addr_t canidm1, canidm2, canidm3, canidm4;

    avr_can_mob_t mobs[6];
    avr_regbit_t baud_rate;
    uint8_t baud_rate_prescalar;
} avr_can_t;

void avr_can_init(avr_t * avr, avr_can_t * p);

#ifdef __cplusplus
};
#endif

#endif
