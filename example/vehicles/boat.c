
#include <boat.h>
#include <vehicle.h>

const char         *boat_type = "boat";

static              bool
is_type_fn (sc_object_t * o, const char *type)
{
  SC_LDEBUG ("boat is_type\n");

  return !strcmp (type, boat_type) || !strcmp (type, vehicle_type);
}

static void
initialize_fn (sc_object_t * o, sc_object_arguments_t * args)
{
  Boat               *boat = boat_get_data (o);

  SC_LDEBUG ("boat initialize\n");

  boat->speed = 0;
  boat->name = "<undefined>";

  if (args != NULL) {
    boat->name = sc_object_arguments_string (args, "name");
  }
}

static void
write_fn (sc_object_t * o, sc_object_t * m, FILE * out)
{
  Boat               *boat = boat_get_data (o);

  fprintf (out, "Boat \"%s\" speeds at %f km/h\n", boat->name, boat->speed);
}

static void
accelerate_fn (sc_object_t * o, sc_object_t * m)
{
  Boat               *boat = boat_get_data (o);

  SC_LDEBUG ("boat accelerate\n");

  boat->speed += 6;
}

sc_object_t        *
boat_klass_new (sc_object_t * d)
{
  bool                a1, a2, a3, a4;
  sc_object_t        *o;

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
    sc_object_method_register (o, (sc_object_method_t) vehicle_accelerate,
                               (sc_object_method_t) accelerate_fn);
  SC_ASSERT (a1 && a2 && a3 && a4);

  sc_object_initialize (o, NULL);

  return o;
}

sc_object_t        *
boat_new (sc_object_t * d, const char *name)
{
  return sc_object_new_from_klass_values (d, "s:name", name, NULL);
}

Boat               *
boat_get_data (sc_object_t * o)
{
  SC_ASSERT (sc_object_is_type (o, boat_type));

  return (Boat *) sc_object_get_data (o, (sc_object_method_t) boat_get_data,
                                      sizeof (Boat));
}
