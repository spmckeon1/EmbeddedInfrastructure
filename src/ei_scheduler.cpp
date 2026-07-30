//
//  ei_scheduler.c
//  
//
//  Created by Stephen McKeon on 7/25/26.
//

#include <Arduino.h>
#include <ei_time.h>
#include <ei_scheduler.h>

Scheduler scheduler;

int32_t Scheduler::getCurrentTick(const RunTime& rt) {
    switch (rt.intvType) {
        case IntervalType::IT_SECOND:
            if ((60 % rt.intvToRun) == 0)
                return eiTime.second();
            return eiTime.now();
        case IntervalType::IT_MINUTE:
            if ((60 % rt.intvToRun) == 0)
                return eiTime.minute();
            return eiTime.now() / 60;
        case IntervalType::IT_HOUR:
            if ((24 % rt.intvToRun) == 0)
                return eiTime.hour();
            return eiTime.now() / 3600;
        case IntervalType::IT_DAY:
            return eiTime.now() / 86400;
    }
    return 0;
}

/*---------------  IS IT TIME TO RUN A PROCESS  AT A DA, HOUR, MINUTE, OR SECOND INTERVAL  ---------------*/

bool Scheduler::isTimeToRun(RunTime& rt) {
    if (rt.lastRun == -1) {
        initializeLastRun(rt);
        return false;
    }
    int32_t current = getCurrentTick(rt);
    if (current == rt.lastRun)
        return false;
    if ((current % rt.intvToRun) != 0)
        return false;
    rt.lastRun = current;
    return true;
}

/*-----  SETUP A NEW RunTime STRUCT    -----*/

void Scheduler::initializeLastRun(RunTime& rt) {
    int32_t current = getCurrentTick(rt);
    rt.lastRun = current - (current % rt.intvToRun);
}
