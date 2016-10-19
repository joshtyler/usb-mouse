/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "usb_dev.h"
#include "usb_mouse.h"
//Dummy define of yield (for now)
#define yield(...)
#include <string.h> // for memcpy()

#ifdef MOUSE_INTERFACE // defined by usb_dev.h -> usb_desc.h

// which buttons are currently pressed
uint8_t usb_mouse_buttons_state=0;

// Set the mouse buttons.  To create a "click", 2 calls are needed,
// one to push the button down and the second to release it
int usb_mouse_buttons(uint8_t left, uint8_t middle, uint8_t right, uint8_t back, uint8_t forward)
{
        uint8_t mask=0;

        if (left) mask    |= 1;
        if (middle) mask  |= 4;
        if (right) mask   |= 2;
        if (back) mask    |= 8;
        if (forward) mask |= 16;
        usb_mouse_buttons_state = mask;
        return usb_mouse_move(0, 0, 0, 0);
}


// Maximum number of transmit packets to queue so we don't starve other endpoints for memory
#define TX_PACKET_LIMIT 1

static uint8_t transmit_previous_timeout=0;

// When the PC isn't listening, how long do we wait before discarding data?
#define TX_TIMEOUT_MSEC 30

//48MHz CPU
#define TX_TIMEOUT (TX_TIMEOUT_MSEC * 428)


// Move the mouse.  x, y and wheel are -127 to 127.  Use 0 for no movement.
int usb_mouse_move(int8_t x, int8_t y, int8_t wheel, int8_t horiz)
{
        uint32_t wait_count=0;
        usb_packet_t *tx_packet;

        //serial_print("move");
        //serial_print("\n");
        if (x == -128) x = -127;
        if (y == -128) y = -127;
        if (wheel == -128) wheel = -127;
        if (horiz == -128) horiz = -127;

        while (1) {
                if (!usb_configuration) {
                        return -1;
                }
                if (usb_tx_packet_count(MOUSE_ENDPOINT) < TX_PACKET_LIMIT) {
                        tx_packet = usb_malloc();
                        if (tx_packet) break;
                }
                if (++wait_count > TX_TIMEOUT || transmit_previous_timeout) {
                        transmit_previous_timeout = 1;
                        return -1;
                }
                yield();
        }
        transmit_previous_timeout = 0;
        *(tx_packet->buf + 0) = 1;
        *(tx_packet->buf + 1) = usb_mouse_buttons_state;
        *(tx_packet->buf + 2) = x;
        *(tx_packet->buf + 3) = y;
        *(tx_packet->buf + 4) = wheel;
        *(tx_packet->buf + 5) = horiz; // horizontal scroll
        tx_packet->len = 6;
        usb_tx(MOUSE_ENDPOINT, tx_packet);
        return 0;
}


#endif // MOUSE_INTERFACE
