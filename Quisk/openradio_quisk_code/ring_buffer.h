#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define TX_BUFFER_SIZE  64

struct ring_buffer {
  unsigned char buffer[TX_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

void store_char(unsigned char c, struct ring_buffer *buffer);
unsigned char read_char(struct ring_buffer *buffer);
unsigned int data_waiting(struct ring_buffer *buffer);
void store_string(char* string);

#endif /*RING_BUFFER_H*/
