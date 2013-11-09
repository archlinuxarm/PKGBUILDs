//
//  Copyright (C) 2010 Tim Blechmann
//  adapted from code by Stephane Letz
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef NOVA_TT_THREAD_PRITORITY_MACH_HPP
#define NOVA_TT_THREAD_PRITORITY_MACH_HPP

#include <cassert>
#include <utility>

#include <pthread.h>

#include <CoreServices/../Frameworks/CarbonCore.framework/Headers/MacTypes.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>

#define NOVA_TT_PRIORITY_PERIOD_COMPUTATION_CONSTRAINT

namespace nova
{


inline int thread_priority(void)
{
    pthread_t this_thread = pthread_self();
    int policy;
    struct sched_param param;

    int status = pthread_getschedparam(this_thread, &policy, &param);
    assert(status == 0);

    return param.sched_priority;
}

inline std::pair<int, int> thread_priority_interval(void)
{
    return std::make_pair(0, 0);
}

inline bool thread_set_priority(int priority)
{
    /* for non-realtime priorities, we can use  */
    pthread_t this_thread = pthread_self();

    struct sched_param parm;
    parm.sched_priority = priority;

    int status = pthread_setschedparam(this_thread, SCHED_OTHER, &parm);

    return status == 0;
}

inline bool thread_set_priority_rt(int period, int computation, int constraint, bool preemptible)
{
    pthread_t this_thread = pthread_self();

    thread_time_constraint_policy_data_t policy;
    policy.period = period;
    policy.computation = computation;
    policy.constraint = constraint;
    policy.preemptible = 0;
    kern_return_t res = thread_policy_set(pthread_mach_thread_np(this_thread),
                                          THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&policy,
                                          THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    return res == KERN_SUCCESS;
}

}  /* namespace nova */

#endif /* NOVA_TT_THREAD_PRITORITY_MACH_HPP */
