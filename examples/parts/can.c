/*
	can.c

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

#include "can.h"

#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include "sim_irq.h"
#include "avr_can.h"

static void can_device_handle_tx(
    struct avr_irq_t * irq,
    uint32_t value,
    void * param
) {
    can_device_t* can = (can_device_t*)param;
    avr_can_frame_t frame = {0};
    frame.mobnb = value;
    avr_ioctl(can->avr, AVR_IOCTL_CAN_SEND, &frame);

    struct can_frame cf = {
        .can_id = frame.id,
        .can_dlc = frame.dlc
    };

    memcpy(cf.data, frame.data, frame.dlc);

    // Write to the socket to send the message
    int nbytes = write(can->s, &cf, sizeof(struct can_frame));

    if (nbytes < 0) {
        perror("write() failed");
        exit(errno);
    }

    if (nbytes < cf.can_dlc) {
        fprintf(stderr,
                "Incomplete frame send: sent %i bytes, wanted %i bytes\n",
                nbytes, cf.can_dlc);
    }

    printf("Board tx'ed frame: id=0x%X, dlc=0x%X\n", frame.id, frame.dlc);
}

int can_device_init(struct avr_t* avr, can_device_t* p) {
    p->avr = avr;

    avr_irq_t* tx_irq = avr_io_getirq(p->avr, AVR_IOCTL_CAN_GETIRQ(), CAN_IRQ_TX);
    avr_irq_register_notify(tx_irq, can_device_handle_tx, p);

    if ((p->s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket() failed");
        return 1;
    }

    // log_info("Socket opened successfully");
    p->addr.can_family = AF_CAN;

    // Set up the Linux device name (i.e. vcan0, can0) and connect the socket
    // to it
    strncpy(p->ifr.ifr_name, p->can_if_name, IFNAMSIZ - 1);
    p->ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(p->s, SIOCGIFINDEX, &p->ifr) < 0) {
        perror("ioctl() failed");
        fprintf(stderr, "Did you forget to setup the CAN device?\n");
        return 1;
    }

    p->addr.can_ifindex = p->ifr.ifr_ifindex;

    // Set up max transmission unit (mtu)
    // "Maximum transmission unit (MTU) determines the maximum payload size of a
    // packet that is sent."
    if (ioctl(p->s, SIOCGIFMTU, &p->ifr) < 0) {
        perror("ioctl() failed");
        return 1;
    }

    // After the socket is initialized and the device is found, we bind to the
    // socket to enable us to read and write to it. `bind` could fail if the
    // port is already in use.
    if (bind(p->s, (struct sockaddr*)&p->addr, sizeof(p->addr)) < 0) {
        perror("bind() failed");
        return 1;
    }

    return 0;
}
