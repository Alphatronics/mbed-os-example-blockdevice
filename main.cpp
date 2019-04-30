/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include <stdio.h>
#include <algorithm>

// Block devices
#include "SDBlockDevice.h"



////////////// binbeat
//#define BINBEAT_V10
#define BINBEAT_V12

DigitalOut enableDebugPortRX(PB_15, 0); //enable RX on debug port
DigitalOut debugPortForceOff(PD_10, 1); // uses inverse logic!
DigitalOut debugPortForceOn(PD_11, 1);
DigitalOut disableVc(PC_6, 0);  //enable vcontrolled
DigitalOut enable3v3(PG_1, 1);
PwmOut buzzer(PF_7); //we're using the buzzer as audio feedback whenever a BLE connection is ready or lost
DigitalOut resetBLE(RST_BLE, 1);

#ifdef BINBEAT_V10
DigitalOut enable5v(PB_1, 1);
#endif // BINBEAT_V10
#ifdef BINBEAT_V12
DigitalOut enable5v(PA_1, 1);
#endif // BINBEAT_V12

void powerup(void) {
    wait_ms(10);
    printf("Initializing board\r\n");
    wait_ms(1000);
    resetBLE = 0;
    printf("Board ready\r\n");
}

void buzzzz()
{
    buzzer.period(1/(float)2050); //xx seconds period
	buzzer = 0.1; //duty cycle, relative to period
}
void mute()
{
    buzzer = 0;
}
void beep()
{
    buzzzz();
    wait_ms(100);
    mute();
}
///


// Physical block device, can be any device that supports the BlockDevice API
SDBlockDevice bd(
        MBED_CONF_SD_SPI_MOSI,
        MBED_CONF_SD_SPI_MISO,
        MBED_CONF_SD_SPI_CLK,
        MBED_CONF_SD_SPI_CS);


// Entry point for the example
int main() {
    powerup();
    printf("--- Mbed OS block device example ---\n");

    // Initialize the block device
    printf("bd.init()\n");
    int err = bd.init();
    printf("bd.init -> %d\n", err);

    // Get device geometry
    bd_size_t read_size    = bd.get_read_size();
    bd_size_t program_size = bd.get_program_size();
    bd_size_t erase_size   = bd.get_erase_size();
    bd_size_t size         = bd.size();

    printf("--- Block device geometry ---\n");
    printf("read_size:    %lld B\n", read_size);
    printf("program_size: %lld B\n", program_size);
    printf("erase_size:   %lld B\n", erase_size);
    printf("size:         %lld B\n", size);
    printf("---\n");

    // Allocate a block with enough space for our data, aligned to the
    // nearest program_size. This is the minimum size necessary to write
    // data to a block.
    size_t buffer_size = sizeof("Hello Storage!") + program_size-1;
    buffer_size = buffer_size - (buffer_size % program_size);
    char *buffer = new char[buffer_size];

    // Read what is currently stored on the block device. We haven't written
    // yet so this may be garbage
    printf("bd.read(%p, %d, %d)\n", buffer, 0, buffer_size);
    err = bd.read(buffer, 0, buffer_size);
    printf("bd.read -> %d\n", err);

    printf("--- Stored data ---\n");
    for (size_t i = 0; i < buffer_size; i += 16) {
        for (size_t j = 0; j < 16; j++) {
            if (i+j < buffer_size) {
                printf("%02x ", buffer[i+j]);
            } else {
                printf("   ");
            }
        }

        printf(" %.*s\n", buffer_size - i, &buffer[i]);
    }
    printf("---\n");

    // Update buffer with our string we want to store
    strncpy(buffer, "Hello Storage!", buffer_size);

    // Write data to first block, write occurs in two parts,
    // an erase followed by a program
    printf("bd.erase(%d, %lld)\n", 0, erase_size);
    err = bd.erase(0, erase_size);
    printf("bd.erase -> %d\n", err);

    printf("bd.program(%p, %d, %d)\n", buffer, 0, buffer_size);
    err = bd.program(buffer, 0, buffer_size);
    printf("bd.program -> %d\n", err);

    // Clobber the buffer so we don't get old data
    memset(buffer, 0xcc, buffer_size);

    // Read the data from the first block, note that the program_size must be
    // a multiple of the read_size, so we don't have to check for alignment
    printf("bd.read(%p, %d, %d)\n", buffer, 0, buffer_size);
    err = bd.read(buffer, 0, buffer_size);
    printf("bd.read -> %d\n", err);

    printf("--- Stored data ---\n");
    for (size_t i = 0; i < buffer_size; i += 16) {
        for (size_t j = 0; j < 16; j++) {
            if (i+j < buffer_size) {
                printf("%02x ", buffer[i+j]);
            } else {
                printf("   ");
            }
        }

        printf(" %.*s\n", buffer_size - i, &buffer[i]);
    }
    printf("---\n");

    // Deinitialize the block device
    printf("bd.deinit()\n");
    err = bd.deinit();
    printf("bd.deinit -> %d\n", err);

    printf("--- done! ---\n");
}

