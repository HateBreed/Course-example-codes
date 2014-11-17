#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

void timespec_difference(struct timespec *start, struct timespec *end, struct timespec **res) {
	// Starting time in nanoseconds is more than ending time
  if (end->tv_nsec - start->tv_nsec < 0) {
    (*res)->tv_sec = end->tv_sec - start->tv_sec - 1; // Second off
    (*res)->tv_nsec = end->tv_nsec - start->tv_nsec + 1.0e9; // Second on (in ns)
  }
  else {
    (*res)->tv_sec = end->tv_sec - start->tv_sec;
    (*res)->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
}

uint64_t timespec_difference_us(struct timespec *start, struct timespec *end) {

  uint64_t difference = 0;
  struct timespec *temp = (struct timespec*)malloc(sizeof(struct timespec));

  timespec_difference(start,end,&temp);
  // seconds -> microseconds + nanoseconds -> microseconds
  difference = temp->tv_sec*1.0e6 + temp->tv_nsec/1.0e3;
  free(temp);
#ifdef __TIMER_DEBUG
  printf("timespec_difference_us: difference=%lu (sec=%f nsec=%ld)\n",
  difference,difftime(end->tv_sec,start->tv_sec),end->tv_nsec - start->tv_nsec);
#endif
  return difference;
}

uint64_t timespec_difference_ms(struct timespec *start, struct timespec *end) {

  uint64_t difference = 0;
  struct timespec *temp = (struct timespec*)malloc(sizeof(struct timespec));

  timespec_difference(start,end,&temp);
  difference = temp->tv_nsec/1.0e6 + temp->tv_sec*1.0e9;
  free(temp);
#ifdef __TIMER_DEBUG
  printf("timespec_difference_ms: difference=%lu (sec=%f nsec=%ld)\n",
    difference,difftime(end->tv_sec,start->tv_sec),end->tv_nsec - start->tv_nsec);
#endif
  return difference;
}

void timeval_difference(struct timeval *start, struct timeval *end, struct timeval **res) {
  // Starting time in microseconds is more than ending time
  if(end->tv_usec - start->tv_usec < 0) {
    (*res)->tv_sec = end->tv_sec - start->tv_sec - 1; // Second off
    (*res)->tv_usec = end->tv_usec - start->tv_usec + 1.0e6; // Second on (in us)
  }
  else {
    (*res)->tv_sec = end->tv_sec - start->tv_sec;
    (*res)->tv_usec = end->tv_usec - start->tv_usec;
  }
}

uint64_t timeval_difference_us(struct timeval *start, struct timeval *end) {

  uint64_t difference = 0;
  int multip = 1.0; // No change
  struct timeval *temp = (struct timeval*)malloc(sizeof(struct timeval));

  timeval_difference(start,end,&temp);
  // seconds -> microseconds + nanoseconds -> microseconds
  difference = (temp->tv_sec * multip) + (temp->tv_usec / multip);
  free(temp);
#ifdef __TIMER_DEBUG
  printf("timeval_difference_us: difference=%lu (sec=%f usec=%ld)\n",
    difference,difftime(end->tv_sec,start->tv_sec),end->tv_usec - start->tv_usec);
#endif
  return difference;
}

uint64_t timeval_difference_ms(struct timeval *start, struct timeval *end) {

  uint64_t difference = 0;
  int multip = 1.0e3;
  struct timeval *temp = (struct timeval*)malloc(sizeof(struct timeval));

  timeval_difference(start,end,&temp);
  // seconds to milliseconds + microseconds -> milliseconds
  difference = (temp->tv_sec * multip) + (temp->tv_usec / multip);
  free(temp);
#ifdef __TIMER_DEBUG
  printf("timeval_difference_us: difference=%lu (sec=%f usec=%ld)\n",
    difference,difftime(end->tv_sec,start->tv_sec),end->tv_usec - start->tv_usec);
#endif
  return difference;
}
																		

int main() {

  clockid_t clockid = 0;
  if(clock_getcpuclockid(0,&clockid) == 0) {
    switch(clockid) {
      case CLOCK_MONOTONIC:
        printf("CLOCK_MONOTONIC\n");
        break;
      case CLOCK_REALTIME:
        printf("CLOCK_REALTIME\n");
        break;
      default:
        printf("Got some other clock %d, setting to CLOCK_REALTIME\n",clockid);
        clockid = CLOCK_REALTIME;
    }
  }
	
  struct timespec start, stop;
  struct timeval t_start, t_stop;
  memset(&start,0,sizeof(struct timespec));
  memset(&stop,0,sizeof(struct timespec));
  memset(&t_start,0,sizeof(struct timeval));
  memset(&t_stop,0,sizeof(struct timeval));

  clock_gettime(clockid, &start);
  gettimeofday(&t_start,NULL);

  usleep(100*1000);

  clock_gettime(clockid, &stop);
  gettimeofday(&t_stop,NULL);

  printf("clock_gettime() difference %lu ms (%lu us)\n",timespec_difference_ms(&start,&stop),timespec_difference_us(&start,&stop));
  printf("gettimeofday() difference %lu ms (%lu us)\n",timeval_difference_ms(&t_start,&t_stop),timeval_difference_us(&t_start,&t_stop));
	
  return 0;
}
