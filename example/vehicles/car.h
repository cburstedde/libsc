/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef CAR_H
#define CAR_H

#include <sc.h>
#include <sc_object.h>

/*
  car is a subclass of sc_object_t
  car implements interface vehicle
*/

typedef struct Car
{
  float               speed;
  float               wheelsize;
}
Car;

typedef struct CarKlass
{
  int                 repairs;
}
CarKlass;

extern const char  *car_type;

/* construction */
sc_object_t        *car_klass_new (sc_object_t * d);
sc_object_t        *car_new (sc_object_t * d, float wheelsize);

/* data */
Car                *car_register_data (sc_object_t * o);
Car                *car_get_data (sc_object_t * o);

CarKlass           *car_register_klass_data (sc_object_t * o);
CarKlass           *car_get_klass_data (sc_object_t * o);

/* virtual method prototypes */
float               car_wheelsize (sc_object_t * o);

#endif /* !CAR_H */
