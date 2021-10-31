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


#include <cstdio>
#include <cstdlib>
#include <climits>

#include "libavoid/timer.h"
#include "libavoid/debug.h"
#include "libavoid/assertions.h"

namespace Avoid {

#ifdef AVOID_PROFILE

Timer::Timer()
{
    reset();
}


void Timer::reset(void)
{
    for (size_t i = 0; i < tmCount; ++i)
    {
        m_total_time[i] =  0;
        m_tally[i] = 0;
        m_max_time[i] = 0;
        for (size_t j = 0; j < TIMER_VARIABLES_COUNT; ++j)
        {
            m_variables[i][j] = 0;
        }
    }
    m_is_running = false;
    m_type = m_last_type = tmCount;
}


void Timer::initialise(const TimerIndex t)
{
    COLA_ASSERT(t != tmCount);
    COLA_ASSERT(m_type == tmCount);
    
    m_type = t;
}

void Timer::start(void)
{
    COLA_ASSERT(!m_is_running);
    m_start_time[m_type] = clock();  // CPU time
    m_is_running = true;
}


void Timer::stop(void)
{
    COLA_ASSERT(m_is_running);
    clock_t stopTime = clock();      // CPU time
    m_is_running = false;

    bigclock_t timeDiff;
    if (stopTime < m_start_time[m_type])
    {
        // Uh-oh, the clock value has wrapped around.
        //
        bigclock_t realStopTime = ((bigclock_t) stopTime) + ULONG_MAX + 1;
        timeDiff = realStopTime - m_start_time[m_type];
    }
    else
    {
        timeDiff = stopTime - m_start_time[m_type];
    }
    
    COLA_ASSERT(timeDiff < LONG_MAX);

    m_total_time[m_type] += timeDiff;
    m_tally[m_type]++;
    if (((clock_t) timeDiff) > m_max_time[m_type])
    {
        m_max_time[m_type] = (clock_t) timeDiff;
    }
    m_last_type = m_type;

    m_type = tmCount;
}

static const char* timerNames[] =
{
    "Adds",
    "Dels",
    "Movs",
    "Pths",
    "OrthogGraph",
    "OrthogRoute",
    "OrthogCentre",
    "OrthogNudge",
    "HyperedgeForest",
    "HyperedgeMTST",
    "HyperedgeImprove",
    "HyperedgeAlt"
};


void Timer::printAll(FILE *fp)
{
    for (unsigned int i = 0; i < tmCount; i++)
    {
        fprintf(fp, "%s:  ", timerNames[i]);
        print((TimerIndex) i, fp);
    }
    fprintf(fp, "\n");
}

#define toMsec(tot) ((bigclock_t) ((tot) / (((double) CLOCKS_PER_SEC) / 1000)))
#define toAvg(tot, cnt) ((((cnt) > 0) ? ((long double) (tot)) / (cnt) : 0))

void Timer::printHyperedgePaper(FILE *fp)
{
    fprintf(fp, "%5lld & %5lld & %5lld & %5lld\n\n",
        toMsec(m_total_time[tmOrthogGraph]),
        toMsec(m_total_time[tmHyperedgeForest] + m_total_time[tmHyperedgeMTST]),
        toMsec(m_total_time[tmHyperedgeAlt]),
        toMsec(m_total_time[tmHyperedgeImprove]));
}

void Timer::print(const TimerIndex t, FILE *fp)
{
    bigclock_t avg = toMsec(toAvg(m_total_time[t], m_tally[t]));
    clock_t max = toMsec(m_max_time[t]); 
    fprintf(fp, "%lld %d %lld %ld",
            toMsec(m_total_time[t]), m_tally[t], avg, max);
    
    for (size_t j = 0; j < TIMER_VARIABLES_COUNT; ++j)
    {
        if (m_variables[t][j] > 0)
        {
            fprintf(fp, ", %lu: %u", j, m_variables[t][j]);
        }
    }
    fprintf(fp, "\n");
}

void Timer::varIncrement(size_t i, unsigned int val)
{
    COLA_ASSERT(i < TIMER_VARIABLES_COUNT);
    
    if (m_is_running)
    {
        m_variables[m_type][i] += val;
    }
}

void Timer::varMax(size_t i, unsigned int val)
{
    COLA_ASSERT(i < TIMER_VARIABLES_COUNT);

    if (m_is_running)
    {
        m_variables[m_type][i] = std::max(m_variables[m_type][i], val);
    }
}

#endif

}

