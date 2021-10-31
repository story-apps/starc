/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2013  Monash University
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See the file LICENSE.LGPL distributed with the library.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the 
 * library.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * Author(s):  Michael Wybrow
*/


#ifndef AVOID_TIMER_H
#define AVOID_TIMER_H

#include <ctime>

namespace Avoid {

//#define AVOID_PROFILE

#ifndef AVOID_PROFILE

  #define TIMER_START(r, t) do {} while(0)
  #define TIMER_STOP(r) do {} while(0)
  #define TIMER_VAR_ADD(r, n, v) do {} while(0)
  #define TIMER_VAR_MAX(r, n, v) do {} while(0)

#else
   
  #define TIMER_START(r, t) r->timers.initialise(t); r->timers.start()
  #define TIMER_STOP(r) r->timers.stop()
  #define TIMER_VAR_ADD(r, n, v) r->timers.varIncrement(n, v);
  #define TIMER_VAR_MAX(r, n, v) r->timers.varMax(n, v)

typedef unsigned long long int bigclock_t;

enum TimerIndex 
{
    tmAdd,
    tmDel,
    tmMov,
    tmPth,
    tmOrthogGraph,
    tmOrthogRoute,
    tmOrthogCentre,
    tmOrthogNudge,
    tmHyperedgeForest,
    tmHyperedgeMTST,
    tmHyperedgeImprove,
    tmHyperedgeAlt,
    tmCount
};

static const bool timerStart = true;
static const bool timerDelay = false;

static const size_t TIMER_VARIABLES_COUNT = 2;

class Timer
{
    public:
        Timer();
        void initialise(const TimerIndex t);
        void start(void);
        void stop(void);
        void reset(void);
        void varIncrement(size_t i, unsigned int val);
        void varMax(size_t i, unsigned int val);

        void print(TimerIndex, FILE *fp);
        void printAll(FILE *fp);
        void printHyperedgePaper(FILE *fp);

    private:
        clock_t m_start_time[tmCount];
        bigclock_t m_total_time[tmCount];
        int m_tally[tmCount];
        clock_t m_max_time[tmCount];
        unsigned int m_variables[tmCount][TIMER_VARIABLES_COUNT];

        bool m_is_running;
        TimerIndex m_type;
        TimerIndex m_last_type;
};

#endif

}

#endif
