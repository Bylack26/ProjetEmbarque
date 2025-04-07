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

#include "main.h"
#include "uart.h"
#include "uart-mmio.h"





static struct uart uarts[NUARTS];
static struct event_uart events[NUARTS];


static void uart_config(uint32_t uartno, void* bar) {
  struct uart*uart = &uarts[uartno];
  uart->uartno = uartno;
  uart->bar = bar;
  mmio_write32(uart->bar, UARTIMSC, 0x0000);
  // no hardware initialization necessary
  // when running on QEMU, the UARTs are
  // already initialized, as long as we
  // do not rely on interrupts.
}

void uarts_init() {
  uart_config(UART0,UART0_BASE_ADDRESS);
  uart_config(UART1,UART1_BASE_ADDRESS);
  uart_config(UART2,UART2_BASE_ADDRESS);
}

void uart_enable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // nothing to do here, as long as
  // we do not rely on interrupts
  mmio_write32(uart->bar, UARTIMSC, UARTIMSCRXIM);
}

void uart_disable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // nothing to do here, as long as
  // we do not rely on interrupts
  mmio_write32(uart->bar, UARTIMSC, 0);
}

void uart_receive(uint8_t uartno, char *pt) {
  struct uart*uart = &uarts[uartno];
  // TODO: not implemented yet...
  // On ne lit pas les de données s'il n'y en a aucune dans la file
  while((mmio_read32(uarts[uartno].bar, UART_FR) & EMPTY))
  ;
  *pt = (char)mmio_read8(uarts[uartno].bar, UART_DR);
}

/**
 * Sends a character through the given uart, this is a blocking call
 * until the character has been sent.
 */
void uart_send(uint8_t uartno, char s) {
  struct uart* uart = &uarts[uartno];
  // TODO: not implemented yet...
  // On n'envoi pas de données s'il la file d'émission est pleine
  while((mmio_read32(uarts[uartno].bar, UART_FR) & FULL))
  ;
  mmio_write8(uarts[uartno].bar, UART_DR, s);
  // On attend que la file d'émission ne soit plus occupé à envoyer (car on veut que l'émission soit bloquante)
  while((mmio_read32(uarts[uartno].bar, UART_FR) & BUSY))
  ;
}

/**
 * This is a wrapper function, provided for simplicity,
 * it sends a C string through the given uart.
 */
void uart_send_string(uint8_t uartno, const char *s) {
  while (*s != '\0') {
    uart_send(uartno, *s);
    s++;
  }
}

void uart_init(uint8_t no, void (*rl)(void *cookie), void (*wl)(void *cookie), void *cookie){
  events[no] = (struct event_uart){no, rl, wl, cookie};
}


bool_t uart_read(uint8_t no, uint8_t* bits){
    return 1;
}

bool_t uart_write(uint8_t no, uint8_t* bits){
    return 1;
}




