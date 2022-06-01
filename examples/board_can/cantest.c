/*
	cantest.c

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

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <pthread.h>

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"
#include "can.h"

avr_t * avr = NULL;
can_device_t can;

int main(void) {
    // Read in firmware
    elf_firmware_t f = {{0}};
    const char * firmware = "air_control.elf";

    elf_read_firmware(firmware, &f);

    // Set up simulator
	avr = avr_make_mcu_by_name("atmega16m1");
	if (!avr) {
		fprintf(stderr, "AVR '%s' not known\n", f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

    char* can_if_name;
    can_if_name = getenv("CAN_NETWORK");
    if (can_if_name) {
        printf("Initializing CAN device...\n");
        can.can_if_name = can_if_name;
        can_device_init(avr, &can);
    }

#ifdef DEBUG
	// avr->gdb_port = 1234;
	//     avr->state = cpu_Stopped;
	//     avr_gdb_init(avr);
#endif

	int state = cpu_Running;
	while ((state != cpu_Done) && (state != cpu_Crashed))
		state = avr_run(avr);
}
