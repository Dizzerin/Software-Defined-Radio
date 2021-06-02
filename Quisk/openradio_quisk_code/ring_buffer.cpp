/*
   Ring Buffer Functions.
   Copied from the Arduino HardwareSerial libs.
   Copyright (c) 2006 Nicholas Zambetti.  All right reserved.	

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   For a full copy of the GNU Lesser General Public License, 
   see <http://www.gnu.org/licenses/>.
*/

#include "ring_buffer.h"

void store_char(unsigned char c, struct ring_buffer *buffer)
{
  int i = (unsigned int)(buffer->head + 1) % TX_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != buffer->tail) {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
}

unsigned char read_char(struct ring_buffer *buffer){
  // if the head isn't ahead of the tail, we don't have any characters
  if (buffer->head == buffer->tail) {
    return 0;
  } else {
    unsigned char c = buffer->buffer[buffer->tail];
    buffer->tail = (unsigned int)(buffer->tail + 1) % TX_BUFFER_SIZE;
    return c;
  }
}

unsigned int data_waiting(struct ring_buffer *buffer){
    return (unsigned int)(TX_BUFFER_SIZE + buffer->head - buffer->tail) % TX_BUFFER_SIZE;
}

