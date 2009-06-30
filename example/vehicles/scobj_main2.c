
#include <sc.h>
#include <sc_object.h>

int
main (int argc, char ** argv)
{
  int                 i;
  sc_object_system_t *s;
  sc_object_t        *object_klass;
  sc_object_t        *o[2];

  sc_init (MPI_COMM_NULL, true, true, NULL, SC_LP_DEFAULT);

  s = sc_object_system_new ();

  object_klass = sc_object_new_klass (s);

  o[0] = sc_object_new (object_klass);
  o[1] = sc_object_new (object_klass);

  SC_INFO ("Write klass\n");
  sc_object_write (object_klass, stdout);

  SC_INFO ("Write and destroy objects\n");
  for (i = 0; i < 2; ++i) {
    sc_object_write (o[i], stdout);
    sc_object_unref (o[i]);
  }

  sc_object_unref (object_klass);

  sc_object_system_destroy (s);

  sc_finalize ();

  return 0;
}
