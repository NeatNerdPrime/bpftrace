#include <sys/time.h>
#include <unistd.h>

#include "sdt.h"

static long myclock()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  DTRACE_PROBE2(tracetest, testprobe, tv.tv_sec, "Hello world");
  DTRACE_PROBE2(tracetest, testprobe, tv.tv_sec, "Hello world2");
  DTRACE_PROBE2(tracetest, testprobe2, tv.tv_sec, "Hello world3");
  DTRACE_PROBE2(tracetest, testprobe3, tv.tv_sec, "Hello world4");
  DTRACE_PROBE2(tracetest, testprobe3, tv.tv_sec, "Hello world5");
  DTRACE_PROBE2(tracetest, testprobe3, tv.tv_sec, "Hello world6");
  return tv.tv_sec;
}

int main()
{
  while (1) {
    myclock();
  }
  return 0;
}
