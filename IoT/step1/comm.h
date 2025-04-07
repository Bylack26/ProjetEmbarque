#ifndef COMM_H
#define COMM_H

#include <stdint.h>
#include <stddef.h>

#define RING_SIZE 255
#define QUEUE_SIZE 32
typedef uint8_t bool_t;

struct ring_buffer {
  volatile uint32_t head, tail;
  volatile uint8_t buffer[RING_SIZE];
};

bool_t ring_empty(struct ring_buffer* buffer);

bool_t ring_full(struct ring_buffer* buffer);

void ring_put(uint8_t bits, struct ring_buffer* buffer);

uint8_t ring_get(struct ring_buffer* buffer);

#endif COMM_H