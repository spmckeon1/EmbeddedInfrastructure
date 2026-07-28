#pragma once


enum class IntervalType
{
    IT_SECOND,
    IT_MINUTE,
    IT_HOUR,
    IT_DAY
};

struct RunTime
{
    IntervalType intvType;
    uint16_t     intvToRun;
    int32_t      lastRun = -1;
};
class Scheduler
{
public:
  bool isTimeToRun(RunTime& rt);

private:
  int32_t getCurrentTick(const RunTime& rt);
  void initializeLastRun(RunTime& rt);
};

extern Scheduler scheduler;
