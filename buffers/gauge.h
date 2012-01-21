#pragma once

#include "su3.h"

typedef su3 su3_tuple[4];

typedef struct
{
  su3_tuple **reserve;
  unsigned int max;
  unsigned int allocated;
  int stack;
} gauge_buffers_t;

typedef struct
{
  su3_tuple *field;
} gauge_field_t;

typedef struct
{
  su3_tuple **field_array;
  unsigned int length;
} gauge_field_array_t;

extern gauge_buffers_t g_gauge_buffers;

void initialize_gauge_buffers(unsigned int max);

gauge_field_t get_gauge_field();
void return_gauge_field(gauge_field_t *gauge_field);

gauge_field_array_t get_gauge_field_array(unsigned int length);
void return_gauge_field_array(gauge_field_array_t *gauge_field_array);

void finalize_gauge_buffers();
