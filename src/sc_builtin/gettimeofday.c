/* public domain: https://stackoverflow.com/a/26085827 */

#include <sc.h>

int
gettimeofday (struct timeval *tp, struct timezone *tzp)
{
  if (tp) {
    /* Note: some broken versions only have 8 trailing zeros,
       the correct epoch has 9 trailing zeros. */
    /* This magic number is the number of 100 nanosecond intervals since
       January 1, 1601 (UTC) until 00:00:00 January 1, 1970 */
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME          system_time;
    FILETIME            file_time;
    uint64_t            time;

    GetSystemTime (&system_time);
    SystemTimeToFileTime (&system_time, &file_time);
    time = ((uint64_t) file_time.dwLowDateTime);
    time += ((uint64_t) file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
  }
  if (tzp) {
    TIME_ZONE_INFORMATION timezone;
    GetTimeZoneInformation (&timezone);
    tzp->tz_minuteswest = timezone.Bias;
    tzp->tz_dsttime = 0;
  }
  return 0;
}
