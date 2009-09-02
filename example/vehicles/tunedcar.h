/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TUNED_CAR_H
#define TUNED_CAR_H

#include <car.h>
#include <sc.h>
#include <sc_object.h>

/*
  tuned_car is a subclass of car
  tuned_car implements interface vehicle
*/

typedef struct TunedCar
{
  int                 faster;
  int                 tickets;
}
TunedCar;

extern const char  *tuned_car_type;

/* construction */
sc_object_t        *tuned_car_klass_new (sc_object_t * d);
sc_object_t        *tuned_car_new (sc_object_t * d, int faster);

/* data */
TunedCar           *tuned_car_get_data (sc_object_t * o);

/* virtual method prototypes */
int                 tuned_car_tickets (sc_object_t * o);

#endif /* !TUNED_CAR_H */
