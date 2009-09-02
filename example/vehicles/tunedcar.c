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

#include "car.h"
#include "tunedcar.h"
#include "vehicle.h"

const char         *tuned_car_type = "tuned_car";

static              bool
is_type_fn (sc_object_t * o, const char *type)
{
  SC_LDEBUG ("tuned_car is_type\n");

  return !strcmp (type, tuned_car_type) || !strcmp (type, vehicle_type);
}

static void
initialize_fn (sc_object_t * o, sc_object_arguments_t * args)
{
  TunedCar           *tuned_car = tuned_car_get_data (o);

  SC_LDEBUG ("tuned_car initialize\n");

  tuned_car->faster = 0;
  tuned_car->tickets = 0;

  if (args != NULL) {
    tuned_car->faster = sc_object_arguments_int (args, "faster");
  }
}

static void
write_fn (sc_object_t * o, sc_object_t * m, FILE * out)
{
  Car                *car = car_get_data (o);
  TunedCar           *tuned_car = tuned_car_get_data (o);

  fprintf (out, "Tuned car (wheel size %f tickets %d) speeds at %f km/h\n",
           car->wheelsize, tuned_car->tickets, car->speed);
}

static int
tickets_fn (sc_object_t * o, sc_object_t * m)
{
  TunedCar           *tuned_car = tuned_car_get_data (o);

  return tuned_car->tickets;
}

static void
accelerate_fn (sc_object_t * o, sc_object_t * m)
{
  int                 i;
  sc_object_method_t  oinmi;
  sc_object_t        *r;
  TunedCar           *tuned_car = tuned_car_get_data (o);

  SC_ASSERT (m != NULL);

  SC_LDEBUG ("tuned car accelerate\n");

  oinmi =
    sc_object_delegate_lookup (m, (sc_object_method_t) vehicle_accelerate,
                               true, &r);
  SC_ASSERT (sc_object_is_type (r, car_type));

  if (oinmi != NULL) {
    for (i = 0; i < tuned_car->faster; ++i)
      ((void (*)(sc_object_t *, sc_object_t *)) oinmi) (o, r);
  }
}

sc_object_t        *
tuned_car_klass_new (sc_object_t * d)
{
  bool                a1, a2, a3, a4, a5;
  sc_object_t        *o;

  SC_ASSERT (d != NULL);
  SC_ASSERT (sc_object_is_type (d, car_type));

  o = sc_object_alloc ();
  sc_object_delegate_push (o, d);

  a1 = sc_object_method_register (o, (sc_object_method_t) sc_object_is_type,
                                  (sc_object_method_t) is_type_fn);
  a2 =
    sc_object_method_register (o, (sc_object_method_t) sc_object_initialize,
                               (sc_object_method_t) initialize_fn);
  a3 =
    sc_object_method_register (o, (sc_object_method_t) sc_object_write,
                               (sc_object_method_t) write_fn);
  a4 =
    sc_object_method_register (o, (sc_object_method_t) tuned_car_tickets,
                               (sc_object_method_t) tickets_fn);
  a5 =
    sc_object_method_register (o, (sc_object_method_t) vehicle_accelerate,
                               (sc_object_method_t) accelerate_fn);
  SC_ASSERT (a1 && a2 && a3 && a4 && a5);

  sc_object_initialize (o, NULL);

  return o;
}

sc_object_t        *
tuned_car_new (sc_object_t * d, int faster)
{
  return sc_object_new_from_klass_values (d, "g:wheelsize", 21.,
                                          "i:faster", faster, NULL);
}

TunedCar           *
tuned_car_get_data (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, tuned_car_type));

  return (TunedCar *) sc_object_get_data (o, (sc_object_method_t)
                                          tuned_car_get_data,
                                          sizeof (TunedCar));
}

int
tuned_car_tickets (sc_object_t * o)
{
  sc_object_method_t  oinmi;
  sc_object_t        *m;

  SC_ASSERT (sc_object_is_type (o, tuned_car_type));

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) tuned_car_tickets,
                               false, &m);
  SC_ASSERT (oinmi != NULL);

  return ((int (*)(sc_object_t *, sc_object_t *)) oinmi) (o, m);
}
