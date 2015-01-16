/*
Generic Job Manager.

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


// $Revision: 1404 $ $Date:: 2015-01-16 #$ $Author: serge $

#ifndef GENERIC_JOB_MAN_T_H
#define GENERIC_JOB_MAN_T_H

#include <map>                          // std::map
#include <stdexcept>                    // std::logic_error
#include <cassert>                      // assert
#include <boost/thread.hpp>             // boost::mutex

#include "../utils/wrap_mutex.h"        // SCOPE_LOCK

#define JOBMAN_ASSERT(_x)               assert(_x)
#include "namespace_lib.h"              // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

template <class _JOB>
class JobManT
{
public:
    struct exception: std::logic_error
    {
        exception( const std::string & arg ):
            std::logic_error( arg )
        {
        }
    };

    struct fatal_exception: std::logic_error
    {
        fatal_exception( const std::string & arg ):
            std::logic_error( arg )
        {
        }
    };

public:
    JobManT();
    ~JobManT();

    bool insert_job( uint32 parent_id, _JOB job );
    bool remove_job( uint32 parent_id );
    bool remove_job_by_child_id( uint32 child_id );
    bool assign_child_id( uint32 parent_id, uint32 child_id );

    _JOB get_job_by_parent_job_id( uint32 id );
    _JOB get_job_by_child_job_id( uint32 id );

    uint32 get_child_id_by_parent_id( uint32 id );
    uint32 get_parent_id_by_child_id( uint32 id );

protected:
    _JOB get_job_by_parent_job_id__( uint32 id );
    bool insert_job_to_child_map( uint32 child_id, _JOB job );

protected:

    typedef std::map<uint32, _JOB>   MapIdToJob;

protected:
    mutable boost::mutex        mutex_;

    MapIdToJob                  map_parent_id_to_job_;
    MapIdToJob                  map_child_id_to_job_;
};

template <class _JOB>
JobManT<_JOB>::JobManT()
{
}

template <class _JOB>
JobManT<_JOB>::~JobManT()
{
}

template <class _JOB>
bool JobManT<_JOB>::insert_job( uint32 parent_id, _JOB job )
{
    SCOPE_LOCK( mutex_ );

    if( map_parent_id_to_job_.count( parent_id ) > 0 )
    {
        throw exception( "job " + std::to_string( parent_id ) + " already exists" );
    }

    if( map_parent_id_to_job_.insert( typename MapIdToJob::value_type( parent_id, job ) ).second == false )
    {
        throw fatal_exception( "cannot insert parent job " + std::to_string( parent_id ) );
    }

    uint32 child_id = job->get_child_job_id();

    if( child_id != 0 )
        insert_job_to_child_map( child_id, job );

    return true;
}

template <class _JOB>
bool JobManT<_JOB>::remove_job( uint32 parent_id )
{
    SCOPE_LOCK( mutex_ );

    uint32 child_id  = 0;

    {
        typename MapIdToJob::iterator it = map_parent_id_to_job_.find( parent_id );
        if( it == map_parent_id_to_job_.end() )
        {
            throw exception( "cannot find parent job " + std::to_string( parent_id ) );
        }

        _JOB job = (*it).second;

        child_id  = job->get_child_job_id();

        map_parent_id_to_job_.erase( it );
    }

    if( child_id != 0 )
    {
        typename MapIdToJob::iterator it = map_child_id_to_job_.find( child_id );
        if( it == map_child_id_to_job_.end() )
        {
            throw exception(
                    "cannot find child job " + std::to_string( child_id ) +
                    " referenced in parent job " + std::to_string( child_id ) );
        }

        JOBMAN_ASSERT( parent_id == (*it).second->get_parent_job_id() );

        map_child_id_to_job_.erase( it );
    }

    return true;
}

template <class _JOB>
bool JobManT<_JOB>::remove_job_by_child_id( uint32 child_id )
{
    return remove_job( get_parent_id_by_child_id( child_id ) );
}

template <class _JOB>
bool JobManT<_JOB>::assign_child_id( uint32 parent_id, uint32 child_id )
{
    SCOPE_LOCK( mutex_ );

    JOBMAN_ASSERT( parent_id );
    JOBMAN_ASSERT( child_id );

    _JOB job = get_job_by_parent_job_id__( parent_id );

    uint32 curr_child_id = job->get_child_job_id();

    if( curr_child_id != 0 )
    {
        throw exception( "cannot assign child " + std::to_string( child_id ) + " to job id " + std::to_string( parent_id ) +
                " as it already has a child " + std::to_string( curr_child_id ) );
    }

    job->set_child_job_id( child_id );

    insert_job_to_child_map( child_id, job );

    return true;
}

template <class _JOB>
_JOB JobManT<_JOB>::get_job_by_parent_job_id( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    return get_job_by_parent_job_id__( id );
}

template <class _JOB>
_JOB JobManT<_JOB>::get_job_by_parent_job_id__( uint32 id )
{
    // private: no mutex

    typename MapIdToJob::iterator it = map_parent_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_parent_id_to_job_.end() );

    return (*it).second;
}

template <class _JOB>
bool JobManT<_JOB>::insert_job_to_child_map( uint32 child_id, _JOB job )
{
    // private: no mutex lock

    JOBMAN_ASSERT( child_id );
    JOBMAN_ASSERT( child_id == job->get_child_job_id() );

    typename MapIdToJob::iterator it = map_child_id_to_job_.find( child_id );
    if( it != map_child_id_to_job_.end() )
    {
        throw fatal_exception(
                "child job " + std::to_string( child_id ) +
                " is already present ( parent id " + std::to_string( job->get_parent_job_id() ) + " )" );
    }

    if( map_child_id_to_job_.insert( typename MapIdToJob::value_type( child_id, job ) ).second == false )
    {
        throw fatal_exception( "unknown error - cannot insert child job " + std::to_string( child_id ) );
    }

    return true;
}

template <class _JOB>
_JOB JobManT<_JOB>::get_job_by_child_job_id( uint32 id )
{
    SCOPE_LOCK( mutex_ );

    typename MapIdToJob::iterator it = map_child_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_child_id_to_job_.end() );

    return (*it).second;
}

template <class _JOB>
uint32 JobManT<_JOB>::get_child_id_by_parent_id( uint32 id )
{
    return get_job_by_parent_job_id( id )->get_child_job_id();
}

template <class _JOB>
uint32 JobManT<_JOB>::get_parent_id_by_child_id( uint32 id )
{
    return get_job_by_child_job_id( id )->get_parent_job_id();
}


NAMESPACE_JOBMAN_END

#endif  // GENERIC_JOB_MAN_T_H
