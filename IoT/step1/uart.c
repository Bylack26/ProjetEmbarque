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
#include "isr.h"
static struct uart uarts[NUARTS];
static struct event_uart events[NUARTS];
static struct queue event_queue;

static void uart_config(uint32_t uartno, void* bar) {
  struct uart*uart = &uarts[uartno];
  uart->uartno = uartno;
  uart->bar = bar;
  uart->rx.head = 0;
  uart->rx.tail = 0;
  uart->ready_to_write = 1;
  mmio_write32(uart->bar, UARTIMSC, 0x0000);
  // no hardware initialization necessary
  // when running on QEMU, the UARTs are
  // already initialized, as long as we
  // do not rely on interrupts.
}

void uarts_init() {
  event_queue.head = 0;
  event_queue.tail = 0;

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

void uart_init(uint8_t no, void (*rl)(uint8_t no, void *cookie), void (*wl)(uint8_t no, void *cookie), void *cookie){
  // Set the listeners for the UARTno
  events[no] = (struct event_uart){no, rl, wl, cookie};
}


bool_t uart_read(uint8_t no, uint8_t* bits){
  // This function can be called even if no data are bufferized (will return 0)
  if(ring_empty(&uarts[no].rx)){
    return 0;
  }else{
    *bits = ring_get(&uarts[no].rx);
  }
  return 1;
}

bool_t uart_write(uint8_t no, uint8_t* bits, uint32_t nb_to_write){
  // this function can be called even if no space is avaible, the writing will resume when the ring will be emptied
  core_disable_irqs();
  uint32_t i = 0;
  if(!uarts[no].ready_to_write){
    struct write_cookie re_write_cookie = {no, bits, nb_to_write};
    struct event re_write_event = {&re_write_cookie, re_write, 0};
    event_put(re_write_event);
    core_enable_irqs();
    return 0;
  }
  while(!ring_full(&uarts[no].tx) && i < nb_to_write){
    // Write the maximum number of bits
    ring_put(bits[i], &uarts[no].tx);
    i++;
  }
  if(i < nb_to_write && ring_full(&uarts[no].tx)){
    // If the ring is full then we can't put more bits in it.
    // An event is triggered to empty the buffer
    struct write_cookie cookie= {no, bits+i, nb_to_write-i};
    struct empty_cookie emp_cookie = {&cookie, events->wl};
    struct event new_event = {&emp_cookie, empty_react, 0};
    event_put( new_event);
    // The writing will restart once the buffer fill level is below the threshold
    core_enable_irqs();
    return 0;
  }
  core_enable_irqs();
  return 1;
}

void empty_react(void* cookie){
  core_disable_irqs();
  struct empty_cookie* casted = (struct write_cookie*) cookie;
  for( uint8_t i =0; i < RING_SIZE -1; i++){
    uart_send(casted->w_c->no, ring_get(&uarts[casted->w_c->no].tx));
  }
  //uart_send_string(casted->w_c->no, &uarts[casted->w_c->no].tx);
  uarts[casted->w_c->no].ready_to_write = 1;
  casted->wl(casted->w_c->no, casted->w_c);
  core_enable_irqs();
}

void re_write(void* cookie){
  //Re start a write that had not started
  struct write_cookie* casted = (struct write_cookie*) cookie;
  uart_write(casted->no, casted->bits, casted->max);
}



bool_t triggers(uint8_t no){
  return !ring_empty(&uarts[no].rx) && events[no].rl != NULL;
}

struct event_uart get_events(uint8_t no){
  return events[no];
}

struct uart* get_uart(uint8_t no){
  return &uarts[no];
}

bool_t event_empty() {
    return (event_queue.head==event_queue.tail);
}

bool_t event_full() {
    int next = (event_queue.head + 1) % RING_SIZE;
    return (next==event_queue.tail);
}

void event_put(struct event bits) {
    uint32_t next = (event_queue.head + 1) % RING_SIZE;
    event_queue.events[event_queue.head] = bits;
    event_queue.head = next;
}

struct event event_get() {
    uint32_t next = (event_queue.tail + 1) % RING_SIZE;
    struct event bits = event_queue.events[event_queue.tail];
    event_queue.tail = next;
    return bits;
}

struct event_uart* get_uart_event(uint8_t no){
  return &events[no];
}