/*
	can.h

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

#ifndef __CAN_DEVICE_H__
#define __CAN_DEVICE_H__

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdint.h>
#include <sys/socket.h>

#include "sim_irq.h"

typedef struct {
    avr_irq_t *	irq;		// irq list
	struct avr_t *avr;

    char* can_if_name;

    // pthread thread;

    int s; // socket
    struct ifreq ifr;
    struct sockaddr_can addr;
} can_device_t;

enum {
    CAN_DEVICE_IRQ_TX,
    CAN_DEVICE_IRQ_RX,
    CAN_DEVICE_IRQ_COUNT,
};

int can_device_init(struct avr_t* avr, can_device_t* can);

// int can_device_send(void);

#endif
