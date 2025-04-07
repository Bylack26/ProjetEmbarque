#include "comm.h"
#include "uart.h"

bool_t ring_empty(struct ring_buffer* buffer) {
    return (buffer->head==buffer->tail);
}

bool_t ring_full(struct ring_buffer* buffer) {
    int next = (buffer->head + 1) % RING_SIZE;
    return (next==buffer->tail);
}

void ring_put(uint8_t bits, struct ring_buffer* buffer) {
    uint32_t next = (buffer->head + 1) % RING_SIZE;
    buffer->buffer[buffer->head] = bits;
    buffer->head = next;
}
uint8_t ring_get(struct ring_buffer* buffer) {
    uint8_t bits;
    uint32_t next = (buffer->tail + 1) % RING_SIZE;
    bits = buffer->buffer[buffer->tail];
    buffer->tail = next;
    return bits;
}