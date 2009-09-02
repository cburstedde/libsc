
#include <vehicle.h>

const char         *vehicle_type = "vehicle";

void
vehicle_accelerate (sc_object_t * o)
{
  sc_object_method_t  oinmi;

  oinmi =
    sc_object_delegate_lookup (o, (sc_object_method_t) vehicle_accelerate);

  if (oinmi != NULL) {
    ((void (*)(sc_object_t *)) oinmi) (o);
  }
}
