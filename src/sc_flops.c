/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

#include <sc_flops.h>

#ifdef SC_PAPI
#include <papi.h>
#endif

void
sc_flops_papi (float *rtime, float *ptime, long long *flpops, float *mflops)
{
#ifdef SC_PAPI
  int                 retval;

  retval = PAPI_flops (rtime, ptime, flpops, mflops);
  SC_CHECK_ABORT (retval == PAPI_OK, "Papi not happy");
#else
  *rtime = *ptime = *mflops = 0.;
  *flpops = 0;
#endif
}

void
sc_flops_start (sc_flopinfo_t * fi)
{
  float               rtime, ptime, mflops;
  long long           flpops;

  fi->seconds = MPI_Wtime ();
  sc_flops_papi (&rtime, &ptime, &flpops, &mflops);     /* ignore results */

  fi->cwtime = 0.;
  fi->crtime = fi->cptime = 0.;
  fi->cflpops = 0;

  fi->iwtime = 0.;
  fi->irtime = fi->iptime = fi->mflops = 0.;
  fi->iflpops = 0;
}

void
sc_flops_count (sc_flopinfo_t * fi)
{
  double              seconds;
  float               rtime, ptime;
  long long           flpops;

  seconds = MPI_Wtime ();
  sc_flops_papi (&rtime, &ptime, &flpops, &fi->mflops);

  fi->iwtime = seconds - fi->seconds;
  fi->cwtime += fi->iwtime;

  fi->iptime = ptime - fi->cptime;
  fi->cptime = ptime;

  fi->iflpops = flpops - fi->cflpops;
  fi->cflpops = flpops;

#ifdef SC_PAPI
  fi->irtime = rtime - fi->crtime;
  fi->crtime = rtime;
#else
  fi->irtime = (float) fi->iwtime;
  fi->crtime = (float) fi->cwtime;
#endif
  fi->seconds = seconds;
}
