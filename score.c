#include <stdio.h>
#include <time.h>
#include <math.h>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "limits.h"
#include "types.h"
#include "engine.c"
#include "score_feed.h"

void feed(unsigned begin, unsigned end);
long long timediff(struct timespec start, struct timespec end);
void print_cpuaffinity();

/* This script provides indicative unofficial score. 
   Lowest score wins.

   Run Ubuntu 10.10 with the kernel option isocpus=1 
   and match "Detailed information about judging 
   platform", on the challenge webpage, to make this as 
   close to the judging environment as possible.
*/

void current_utc_time(struct timespec *ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_MONOTONIC_RAW, ts);
#endif
}


int msg_batch_size = 200;
int replays = 2000;
void execution(t_execution exec) {};

int main() {
  print_cpuaffinity();
  int raw_feed_len = sizeof(raw_feed)/sizeof(t_order);
  int samples = replays * (raw_feed_len/msg_batch_size);
  long long late[samples]; // batch latency measurements
  
 
  struct timespec begin;
  struct timespec end;

  int j; for (j = 0; j < replays; j++) {
  init();
  int i; for (i = msg_batch_size; i < raw_feed_len; i += msg_batch_size) {
    current_utc_time(&begin);

    feed(i-msg_batch_size, i);

    current_utc_time(&end);
    late[i/msg_batch_size - 1 + (j*(raw_feed_len/msg_batch_size))] = timediff(begin, end);
  } 
  destroy();
  }

  long long late_total = 0LL;
  int i; for (i = 0; i < samples; i++) { late_total += late[i]; }
  double late_mean = ((double)late_total) / ((double)samples);
  double late_centered = 0;
  double long late_sqtotal = 0LL;
  for (i = 0; i < samples; i++) {
    late_centered = ((double)late[i]) - late_mean;
    late_sqtotal += late_centered*late_centered/((double)samples);
  }
  double late_sd = sqrt(late_sqtotal);
  printf("mean(latency) = %1.2f, sd(latency) = %1.2f\n", late_mean, late_sd);

  double score = 0.5 * (late_mean + late_sd);  
  printf("You scored %1.2f. Try to minimize this.\n", score);
  printf("%1.2f\n", late_mean);
  printf("%1.2f\n", late_sd);
  printf("%1.2f", score);
}

void feed(unsigned begin, unsigned end) {
  int i; for(i = begin; i < end; i++) {
    if (raw_feed[i].price == 0) {
      cancel(raw_feed[i].size);
    } else {
      limit(raw_feed[i]);
    }
  }
}

long long timediff(struct timespec start, struct timespec end) {
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return (temp.tv_sec*1000000000) + temp.tv_nsec;
}

void print_cpuaffinity() {
#ifndef __MACH__
  unsigned long mask = 2; /* processor 1 (0-indexed) */
  unsigned int len = sizeof(mask);
  if (sched_getaffinity(0, len, &mask) < 0) {
    perror("sched_getaffinity");
  }
  printf("my affinity mask is: %08lx\n", mask);
#endif
}

