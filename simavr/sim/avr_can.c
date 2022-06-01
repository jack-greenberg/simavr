/*
    avr_can.c
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
#include "avr_can.h"
#include "sim_avr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DECLARE_MOB(_mobnb) { \
    .enabled = AVR_IO_REGBIT(CANEN2, ENMOB ## _mobnb), \
},

void avr_can_reset(struct avr_io_t *io) {
    avr_can_t * p = (avr_can_t *)io;
    avr_regbit_set(p->io.avr, p->ainc);
}

/*
 * When AVR device writes to the data register
 */
static void avr_can_write_data(struct avr_t * avr, avr_io_addr_t addr, uint8_t v, void * param)
{
    avr_can_t * p = (avr_can_t *)param;

    uint8_t curr_mob = avr_regbit_get(avr, p->mob);
    assert(curr_mob < 6);

    uint8_t indx = avr_regbit_get(avr, p->indx);

    if (avr_regbit_get(avr, p->ainc)) {
        indx = (indx + 1) % 8;
        avr_regbit_setto(avr, p->indx, indx);
    }
}

/*
 * Sending or receiving CAN message
 *
 * If everything looks good, raise an IRQ
 */
static void avr_can_write_cfg(struct avr_t * avr, avr_io_addr_t addr, uint8_t v, void * param)
{
    // Check mob, mode, dlc, etc
    avr_can_t * p = (avr_can_t *)param;

    uint8_t curr_mob = avr_regbit_get(avr, p->mob);
    assert(curr_mob < 6);

    avr_core_watch_write(avr, addr, v);

    // Save data in registers to MOb
    uint8_t dlc = avr_regbit_get(avr, p->dlc);
    uint8_t mode = avr_regbit_get(avr, p->mode);
    uint8_t indx = avr_regbit_get(avr, p->indx);
    uint16_t id = (avr->data[p->canidt1] << 3) | 
                        (avr->data[p->canidt2] >> 5);
    uint16_t msk = (avr->data[p->canidm1] << 3) | 
                        (avr->data[p->canidm2] >> 5);

    p->mobs[curr_mob].dlc = dlc;
    p->mobs[curr_mob].mode = mode;
    p->mobs[curr_mob].data_indx = indx;
    p->mobs[curr_mob].id = id;
    p->mobs[curr_mob].mask = msk;

    uint8_t masked_v = (v >> 6) & 0x3;

    switch (masked_v) {
        case MOB_MODE_DISABLE: {

        } break;
        case MOB_MODE_ENABLE_TX: {
            // Check if MOb is enabled
            AVR_LOG(avr, LOG_TRACE, "CAN: transmit id=0x%X dlc=0x%X",
                    p->mobs[curr_mob].id,
                    p->mobs[curr_mob].dlc);

            // Set txbsy, set timer that will disable it
            avr_regbit_set(avr, p->txbsy);
            avr_raise_irq(p->io.irq + CAN_IRQ_TX, curr_mob);
        } break;
        case MOB_MODE_ENABLE_RX: {
            // Check if MOb is enabled
            // if (avr_regbit_get(avr, p->mobs[curr_mob].enabled)) {
            AVR_LOG(avr, LOG_TRACE, "CAN: receive id=0x%X msk=0x%X",
                    p->mobs[curr_mob].id,
                    p->mobs[curr_mob].mask);

                // can_receive(); // Wait for ioctl? idk yet
            // }
        } break;
        case MOB_MODE_ENABLE_FRAME_BUFFER_RX: {
            // unimplemented!
        } break;
        default: {
            AVR_LOG(avr, LOG_ERROR, "CAN: Invalid MOb configuration mode: %i\n", v);
            exit(1);
        } break;
    };
}

static void avr_can_write_mobnb(struct avr_t * avr, avr_io_addr_t addr, uint8_t v, void * param)
{
    // Save data and swap in with current MOb data
    avr_can_t * p = (avr_can_t *)param;

    uint8_t prev_mobnb = avr_regbit_get(avr, p->mob);
    uint8_t new_mobnb = (v >> 4) & 0x7;

    /*
     * Save prev values
     */
    uint8_t prev_dlc = avr->data[p->cancdmob] & 0xF;
    uint8_t prev_mode = (avr->data[p->cancdmob] >> 6) & 0x3;
    uint8_t prev_indx = avr_regbit_get(avr, p->indx);
    uint16_t prev_id = (avr->data[p->canidt1] << 3) | 
                        (avr->data[p->canidt2] >> 5);
    uint16_t prev_msk = (avr->data[p->canidm1] << 3) | 
                        (avr->data[p->canidm2] >> 5);

    p->mobs[prev_mobnb].dlc = prev_dlc;
    p->mobs[prev_mobnb].mode = prev_mode;
    p->mobs[prev_mobnb].data_indx = prev_indx;
    p->mobs[prev_mobnb].id = prev_id;
    p->mobs[prev_mobnb].mask = prev_msk;

    /*
     * Update new values in the background
     */
    avr_can_mob_t new_mob = p->mobs[new_mobnb];

    avr_regbit_setto(avr, p->dlc, new_mob.dlc);
    avr_regbit_setto(avr, p->mode, new_mob.mode);
    avr_regbit_setto(avr, p->indx, new_mob.data_indx);
    avr->data[p->canidt1] = new_mob.id >> 3;
    avr->data[p->canidt2] = new_mob.id << 5;
    avr->data[p->canidm1] = new_mob.mask >> 3;
    avr->data[p->canidm2] = new_mob.mask << 5;

    avr_core_watch_write(avr, addr, v);

    avr_raise_irq(p->io.irq + CAN_IRQ_MOBNB, new_mobnb);
}

// Need functions that write data specific to each MOb
// enable?
// maybe need a raise_irq for each of these? so like raise everytime data is
// written, raise when dlc is written, etc. those fill up a can_frame_t and can
// be sent with write()

// static void avr_can_read(struct avr_t * avr, avr_io_addr_t addr, void * param) 
// {
//     // avr_can_t * p = (avr_can_t *)param;
//     // uint8_t* data = p->mobs[__].data;
//     // uint8_t dlc = p->mobs[__].dlc;
// }

static const char * irq_names[CAN_IRQ_COUNT] = {
	[CAN_IRQ_TX] = "can<out",
	[CAN_IRQ_RX] = "can<in",
    [CAN_IRQ_MOBNB] = "can<mob",
};

static int avr_can_ioctl(struct avr_io_t * port, uint32_t ctl, void * io_param)
{
    int st = -1;

    avr_can_t* can = (avr_can_t *)port;
    avr_t* avr = can->io.avr;

    switch (ctl) {
        case AVR_IOCTL_CAN_RECEIVE: {
            for (uint8_t i = 0; i < 6; i++) {
                avr_can_mob_t mob = can->mobs[i];
                avr_can_frame_t* frame = (avr_can_frame_t*)io_param;

                if (mob.mode == MOB_MODE_ENABLE_RX) {

                    // Check for match with ID and mask
                    if ((frame->id & mob.mask) == (mob.id & mob.mask)) {
                        // Received!
                        AVR_LOG(avr, LOG_TRACE, "Received CAN message\n");

                        memcpy(mob.data, frame->data, frame->dlc);
                        avr_regbit_setto(avr, can->dlc, frame->dlc);
                        avr->data[can->canidt1] = frame->id >> 3;
                        avr->data[can->canidt2] = frame->id << 5;
                        st = 0;
                        break;
                    } else {
                        continue;
                    }
                }
            }
        } break;
        case AVR_IOCTL_CAN_SEND: {
            avr_can_frame_t* frame = (avr_can_frame_t*) io_param;
            avr_can_mob_t mob = can->mobs[frame->mobnb];

            memcpy(frame->data, mob.data, mob.dlc);
            frame->dlc = mob.dlc;
            frame->id = mob.id;
            st = 0;
        } break;
        default: {

        } break;
    };

    return st;
}

static avr_io_t _io = {
    .kind = "can",
    .ioctl = avr_can_ioctl,
    .reset = avr_can_reset,
    .irq_names = irq_names,
};

void avr_can_init(avr_t * avr, avr_can_t * p) {
    p->io = _io;

    avr_register_io(avr, &p->io);

    // avr_register_vector(avr, &p->can); // TODO: handle CAN_INT_vect

    avr_io_setirqs(&p->io, AVR_IOCTL_CAN_GETIRQ(), CAN_IRQ_COUNT, NULL);

	avr_register_io_write(avr, p->canmsg, avr_can_write_data, p);
	avr_register_io_write(avr, p->dlc.reg, avr_can_write_cfg, p);
	avr_register_io_write(avr, p->mob.reg, avr_can_write_mobnb, p);
	// avr_register_io_read(avr, p->r_spdr, avr_spi_read, p);
}
