#include "sleepHelper.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

// made this last time for A1
void milisecondSleep(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;

    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

void secondSleep(int delayInSeconds){
    // there's 1000 ms in 1 sec
    milisecondSleep(1000 * delayInSeconds);
}