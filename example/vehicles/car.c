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

#include <car.h>
#include <vehicle.h>

const char         *car_type = "car";

static              bool
is_type_fn (sc_object_t * o, const char *type)
{
  SC_LDEBUG ("car is_type\n");

  return !strcmp (type, car_type) || !strcmp (type, vehicle_type);
}

static void
initialize_fn (sc_object_t * o, sc_object_arguments_t * args)
{
  Car                *car = car_get_data (o);

  SC_LDEBUG ("car initialize\n");

  car->speed = 0;
  car->wheelsize = 0.;

  if (args != NULL) {
    car->wheelsize = (float) sc_object_arguments_double (args, "wheelsize");
  }
}

static void
write_fn (sc_object_t * o, sc_object_t * m, FILE * out)
{
  Car                *car = car_get_data (o);

  fprintf (out, "Car (wheel size %f) speeds at %f km/h\n",
           car->wheelsize, car->speed);
}

static float
wheelsize_fn (sc_object_t * o, sc_object_t * m)
{
  Car                *car = car_get_data (o);

  SC_LDEBUG ("car wheelsize\n");

  return car->wheelsize;
}

static void
accelerate_fn (sc_object_t * o, sc_object_t * m)
{
  Car                *car = car_get_data (o);
  CarKlass           *car_klass;

  SC_LDEBUG ("car accelerate\n");

  car->speed += 10;

  car_klass = car_get_klass_data (m);
  ++car_klass->repairs;
}

sc_object_t        *
car_klass_new (sc_object_t * d)
{
  bool                a1, a2, a3, a4, a5;
  sc_object_t        *o;
  CarKlass           *car_klass;

  SC_ASSERT (d != NULL);
  SC_ASSERT (sc_object_is_type (d, sc_object_type));

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
    sc_object_method_register (o, (sc_object_method_t) car_wheelsize,
                               (sc_object_method_t) wheelsize_fn);
  a5 =
    sc_object_method_register (o, (sc_object_method_t) vehicle_accelerate,
                               (sc_object_method_t) accelerate_fn);
  SC_ASSERT (a1 && a2 && a3 && a4 && a5);

  sc_object_initialize (o, NULL);
  car_klass = car_get_klass_data (o);
  car_klass->repairs = 0;

  return o;
}

sc_object_t        *
car_new (sc_object_t * d, float wheelsize)
{
  return sc_object_new_from_klass_values (d, "g:wheelsize",
                                          (double) wheelsize, NULL);
}

Car                *
car_get_data (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, car_type));

  return (Car *) sc_object_get_data (o, (sc_object_method_t) car_get_data,
                                     sizeof (Car));
}

CarKlass           *
car_get_klass_data (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, car_type));

  return (CarKlass *) sc_object_get_data (o, (sc_object_method_t)
                                          car_get_klass_data,
                                          sizeof (CarKlass));
}

float
car_wheelsize (sc_object_t * o)
{
  sc_object_method_t  oinmi;
  sc_object_t        *m;

  SC_ASSERT (sc_object_is_type (o, car_type));

  oinmi = sc_object_delegate_lookup (o, (sc_object_method_t) car_wheelsize,
                                     false, &m);
  SC_ASSERT (oinmi != NULL);

  return ((float (*)(sc_object_t *, sc_object_t *)) oinmi) (o, m);
}
