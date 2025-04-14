/*
 * Copyright: Olivier Gruber (olivier dot gruber at acm dot org)
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UART_H_
#define UART_H_

/*
 * Defines the number of available UARTs
 * and their respective numéro.
 */
#define NUARTS 3
#define UART0 0
#define UART1 1
#define UART2 2

#define SEUIL RING_SIZE/2
#define QUEUE_SIZE 32
#include "comm.h"

struct uart {
  uint8_t uartno; // the UART numbuer
  void* bar;      // base address register for this UART
  struct ring_buffer rx;
  struct ring_buffer tx;
  bool_t ready_to_write;
};

struct event_uart {
  uint8_t no;
  void (*rl)(uint8_t no, void *cookie);
  void (*wl)(uint8_t no, void *cookie);
  void *cookie;
};

struct event {
  void* cookie;
  void (*react)(void* cookie);
  uint32_t eta; // Estimated Time of Arrival
};

struct queue {
  uint32_t head,tail;
  struct event events[QUEUE_SIZE];

};

struct write_cookie{
  uint8_t no;
  uint8_t* bits;
  uint32_t max;
};

struct empty_cookie{
  struct write_cookie* w_c;
  void (*wl)(uint8_t no, void *cookie);
};

struct read_cookie{
  uint8_t no;
  char* c;
};




/*
 * Receives a one-byte character, which is compatible
 * with ASCII encoding. This function blocks, spinning,
 * until there is one character available, that is,
 * there is at least one character available in the
 * UART RX FIFO queue.
 */
void uart_receive(uint8_t uartno, char *pt);

/**
 * Write a one-byte character through the given uart,
 * this is a blocking call. This is compatible with a
 * basic ASCII encoding. This function blocks, spinning,
 * until there is room in the UART TX FIFO queue to send
 * the character.
 */
void uart_send(uint8_t uartno, char s);

/**
 * This is a wrapper function, provided for simplicity,
 * it sends the given C string through the given uart,
 * using the function uart_send.
 */
void uart_send_string(uint8_t uartno, const char *s);

/*
 * Global initialization for all the UARTs
 */
void uarts_init();

/*
 * Enables the UART, identified by the given numéro.
 * Nothing to do on QEMU until we use interrupts...
 * You can enable or disable the individual interrupts by changing
 */
void uart_enable(uint32_t uartno);

/*
 * Disables the UART, identified by the given numéro.
 * Nothing to do on QEMU until we use interrupts...
 */
void uart_disable(uint32_t uartno);

void uart_init(uint8_t no,
               void (*rl)(uint8_t no, void *cookie),
               void (*wl)(uint8_t no, void *cookie),
               void *cookie);
               
bool_t uart_read(uint8_t no, uint8_t* bits);// Put the character read from the buffer inside the bits

bool_t uart_write(uint8_t no, uint8_t* bits, uint32_t nb_to_write); //Write the given bits into the buffer

bool_t triggers(uint8_t no);

struct event_uart get_events(uint8_t no);

struct uart* get_uart(uint8_t no);

bool_t event_empty();

bool_t event_full();

void event_put(struct event bits);

struct event event_get();

struct event_uart* get_uart_event(uint8_t no);

void re_write(void* cookie);

void empty_react(void* cookie);
#endif /* UART_H_ */
