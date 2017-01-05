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


// $Revision: 5466 $ $Date:: 2017-01-04 #$ $Author: serge $

#ifndef JOBMAN_I_JOB_H
#define JOBMAN_I_JOB_H

#include <mutex>                        // std::mutex
#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK

#include "namespace_lib.h"              // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

template <class JOB_ID>
class Job
{
public:
    Job( JOB_ID parent_job_id, JOB_ID child_job_id = 0 );

    // interface IJob
    JOB_ID get_parent_job_id() const;
    JOB_ID get_child_job_id() const;
    void set_child_job_id( JOB_ID id );

protected:
    mutable std::mutex      mutex_;

    JOB_ID                parent_job_id_;
    JOB_ID                child_job_id_;
};

template <class JOB_ID>
Job<JOB_ID>::Job( JOB_ID parent_job_id, JOB_ID child_job_id ):
    parent_job_id_( parent_job_id ),
    child_job_id_( child_job_id )
{
}

template <class JOB_ID>
JOB_ID Job<JOB_ID>::get_child_job_id() const
{
    MUTEX_SCOPE_LOCK( mutex_ );
    return child_job_id_;
}

template <class JOB_ID>
JOB_ID Job<JOB_ID>::get_parent_job_id() const
{
    MUTEX_SCOPE_LOCK( mutex_ );
    return parent_job_id_;
}

template <class JOB_ID>
void Job<JOB_ID>::set_child_job_id( JOB_ID id )
{
    MUTEX_SCOPE_LOCK( mutex_ );
    child_job_id_  = id;
}

NAMESPACE_JOBMAN_END

#endif  // JOBMAN_I_JOB_H
