
#include <sc3_log.h>
#include <sc3_trace.h>
#ifdef SC_HAVE_JANSSON_H
#include <jansson.h>
#endif

typedef struct jrw_global
{
  sc3_allocator_t    *a;
  sc3_log_t          *l;
}
jrw_global_t;

static sc3_error_t *
process_options (sc3_trace_t * t, jrw_global_t * g, int argc, char **argv)
{
  sc3_trace_t         stacktrace;
  SC3E (sc3_trace_push (&t, &stacktrace, "process options", NULL));

  return NULL;
}

static sc3_error_t *
global_init (sc3_trace_t * t, jrw_global_t * g)
{
  sc3_trace_t         stacktrace;
  SC3E (sc3_trace_push (&t, &stacktrace, "global init", NULL));

  SC3A_CHECK (g != NULL);

  SC3E (sc3_allocator_new (NULL, &g->a));
  SC3E (sc3_allocator_setup (g->a));

  SC3E (sc3_log_new (NULL, &g->l));
  SC3E (sc3_log_setup (g->l));

  sc3_logf (g->l, SC3_LOG_GLOBAL, SC3_LOG_DEBUG, t->depth, "%s", t->func);

  return NULL;
}

static sc3_error_t *
global_reset (sc3_trace_t * t, jrw_global_t * g)
{
  sc3_trace_t         stacktrace;
  SC3E (sc3_trace_push (&t, &stacktrace, "global reset", NULL));

  sc3_logf (g->l, SC3_LOG_GLOBAL, SC3_LOG_DEBUG, t->depth, "%s", t->func);

  SC3E (sc3_log_destroy (&g->l));
  SC3E (sc3_allocator_destroy (&g->a));
  return NULL;
}

static sc3_error_t *
single_program (sc3_trace_t * t, int argc, char **argv)
{
  jrw_global_t        jrwg, *g = &jrwg;
  sc3_trace_t         stacktrace;
  SC3E (sc3_trace_push (&t, &stacktrace, "single program", NULL));

  SC3E (global_init (t, g));
  SC3E (process_options (t, g, argc, argv));

  SC3E (global_reset (t, g));
  return NULL;
}

int
main (int argc, char **argv)
{
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  int                 mpirank;

  SC3X (sc3_MPI_Init (&argc, &argv));
  SC3X (sc3_MPI_Comm_rank (mpicomm, &mpirank));
  if (mpirank == 0) {
    SC3X (single_program (NULL, argc, argv));
  }
  SC3X (sc3_MPI_Finalize ());
  return EXIT_SUCCESS;
}
