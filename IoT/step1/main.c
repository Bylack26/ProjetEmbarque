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
#include "isr.h"

extern uint32_t irq_stack_top;
extern uint32_t stack_top;

void check_stacks() {
  void *memsize = (void*)MEMORY;
  void *addr;
  addr = &stack_top;
  if (addr >= memsize)
    panic();
  // addr = &irq_stack_top;
  // if (addr >= memsize)
  //   panic();
}

/**
 * This is the C entry point,
 * upcalled once the hardware has been setup properly
 * in assembly language, see the startup.s file.
 */
void _start(void) {
  char c;
  check_stacks();
  uarts_init(); // Configuration of all UARTs (for now)
  uart_enable(UART0); // Enable interuptions for this UART
  vic_setup_irqs(); // Setup the vic for the interuptions
  // uart_send_string(UART0, "\nThe system is now running... \n"); //Just a print to be sure
  uart_init(0, &read_listener, &write_listener, &c);
  for (;;) {
    
    core_halt();
    event_pop();
  }

}

void panic() {
  for(;;)
    ;
}

void read_listener(uint8_t no, void* cookie){
  uart_read(no, cookie); 
  // Use a function with the character read
  uart_write(no, (uint8_t *)cookie, 1);
  //uart_send(no, *((char *)cookie));
}

void write_listener(uint8_t no, void* cookie){
  struct write_cookie* casted = (struct write_cookie*) cookie;
  uart_write(casted->no, casted->bits, casted->max);
}

void event_pop(){
  core_disable_irqs();
  if(!event_empty()){
    struct event event_to_process = event_get();
    event_to_process.react(event_to_process.cookie);
  }
  core_enable_irqs();
}

void trigger_listener(){
  for( uint8_t i =0; i < NUARTS; i++){
    if(triggers(i)){
      struct event_uart ua_ev = get_events(i);
      ua_ev.rl(i, ua_ev.cookie);
    }
  }
  
}
