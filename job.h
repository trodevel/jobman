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


// $Id: job.h 1252 2014-12-04 19:42:08Z serge $

#ifndef JOBMAN_I_JOB_H
#define JOBMAN_I_JOB_H

#include "../utils/types.h"         // uint32

#include "namespace_lib.h"          // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

class Job
{
public:
    Job( uint32 parent_id, uint32 child_id = 0 );

    // interface IJob
    uint32 get_parent_id() const;
    uint32 get_child_id() const;
    void set_child_id( uint32 id );

protected:
    uint32      parent_id_;
    uint32      child_id_;
};

inline Job::Job( uint32 parent_id, uint32 child_id ):
    parent_id_( parent_id ),
    child_id_( child_id )
{
}

inline uint32 Job::get_child_id() const
{
    return child_id_;
}
inline uint32 Job::get_parent_id() const
{
    return parent_id_;
}
inline void Job::set_child_id( uint32 id )
{
    child_id_  = id;
}

NAMESPACE_JOBMAN_END

#endif  // JOBMAN_I_JOB_H
