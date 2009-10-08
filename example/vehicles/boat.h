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

#ifndef BOAT_H
#define BOAT_H

#include <sc.h>
#include <sc_object.h>

/*
  boat is a subclass of sc_object_t
  boat implements interface vehicle
*/

typedef struct Boat
{
  float               speed;
  const char         *name;     /* names are static strings */
}
Boat;

extern const char  *boat_type;

/* construction */
sc_object_t        *boat_klass_new (sc_object_t * d);
sc_object_t        *boat_new (sc_object_t * d, const char *name);

/* data */
Boat               *boat_register_data (sc_object_t * o);
Boat               *boat_get_data (sc_object_t * o);

#endif /* !BOAT_H */
