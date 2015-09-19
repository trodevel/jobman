/*

IJob.

Copyright (C) 2014 Sergey Kolevatov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

*/


// $Revision: 2577 $ $Date:: 2015-09-18 #$ $Author: serge $

#ifndef JOBMAN_I_JOB_H
#define JOBMAN_I_JOB_H

#include <mutex>                        // std::mutex
#include <stdint.h>                     // uint32_t
#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK

#include "namespace_lib.h"              // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

class Job
{
public:
    Job( uint32_t parent_job_id, uint32_t child_job_id = 0 );

    // interface IJob
    uint32_t get_parent_job_id() const;
    uint32_t get_child_job_id() const;
    void set_child_job_id( uint32_t id );

protected:
    mutable std::mutex      mutex_;

    uint32_t                parent_job_id_;
    uint32_t                child_job_id_;
};

inline Job::Job( uint32_t parent_job_id, uint32_t child_job_id ):
    parent_job_id_( parent_job_id ),
    child_job_id_( child_job_id )
{
}

inline uint32_t Job::get_child_job_id() const
{
    MUTEX_SCOPE_LOCK( mutex_ );
    return child_job_id_;
}
inline uint32_t Job::get_parent_job_id() const
{
    MUTEX_SCOPE_LOCK( mutex_ );
    return parent_job_id_;
}
inline void Job::set_child_job_id( uint32_t id )
{
    MUTEX_SCOPE_LOCK( mutex_ );
    child_job_id_  = id;
}

NAMESPACE_JOBMAN_END

#endif  // JOBMAN_I_JOB_H
