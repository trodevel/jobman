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


// $Revision: 5479 $ $Date:: 2017-01-04 #$ $Author: serge $

#ifndef GENERIC_JOB_MAN_T_H
#define GENERIC_JOB_MAN_T_H

#include <map>                          // std::map
#include <stdexcept>                    // std::logic_error
#include <cassert>                      // assert
#include <mutex>                        // std::mutex
#include <algorithm>                    // std::transform
#include <functional>                   // std::bind

#include "../utils/mutex_helper.h"      // MUTEX_SCOPE_LOCK

#define JOBMAN_ASSERT(_x)               assert(_x)
#include "namespace_lib.h"              // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

template <class JOB, class JOB_ID>
class JobManT
{
protected:

    typedef std::map<JOB_ID, JOB>   MapIdToJob;

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

    bool insert_job( JOB_ID parent_id, JOB job );
    bool remove_job( JOB_ID parent_id );
    template <typename ITERATOR>
    bool remove_jobs( ITERATOR begin, ITERATOR end );
    bool remove_job_by_child_id( JOB_ID child_id );
    bool assign_child_id( JOB_ID parent_id, JOB_ID child_id );

    bool has_parent_job_id( JOB_ID id ) const;
    JOB get_job_by_parent_job_id( JOB_ID id );
    const JOB get_job_by_parent_job_id( JOB_ID id ) const;
    JOB get_job_by_child_job_id( JOB_ID id );

    JOB_ID get_child_id_by_parent_id( JOB_ID id );
    JOB_ID get_parent_id_by_child_id( JOB_ID id );

    void get_all_jobs( std::vector<JOB> & res ) const;
    template< typename _PRED>
    void find_jobs( std::vector<JOB> & res, _PRED pred ) const;

    template< typename _OutputIterator, typename _PRED>
    void find_job_ids( _OutputIterator res, _PRED pred ) const;

    const MapIdToJob & get_job_map_and_lock() const;
    void unlock() const;

protected:
    bool remove_job__( JOB_ID parent_id );
    JOB get_job_by_parent_job_id__( JOB_ID id );
    const JOB get_job_by_parent_job_id__( JOB_ID id ) const;
    bool insert_job_to_child_map( JOB_ID child_id, JOB job );

    void get_all_jobs__( std::vector<JOB> & res ) const;

protected:
    mutable std::mutex          mutex_;

    MapIdToJob                  map_parent_id_to_job_;
    MapIdToJob                  map_child_id_to_job_;
};

template <class JOB, class JOB_ID>
JobManT<JOB,JOB_ID>::JobManT()
{
}

template <class JOB, class JOB_ID>
JobManT<JOB,JOB_ID>::~JobManT()
{
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::insert_job( JOB_ID parent_id, JOB job )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    if( map_parent_id_to_job_.count( parent_id ) > 0 )
    {
        throw exception( "job " + std::to_string( parent_id ) + " already exists" );
    }

    if( map_parent_id_to_job_.insert( typename MapIdToJob::value_type( parent_id, job ) ).second == false )
    {
        throw fatal_exception( "cannot insert parent job " + std::to_string( parent_id ) );
    }

    JOB_ID child_id = job->get_child_job_id();

    if( child_id != 0 )
        insert_job_to_child_map( child_id, job );

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::remove_job( JOB_ID parent_id )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return remove_job__( parent_id );
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::remove_job__( JOB_ID parent_id )
{
    JOB_ID child_id  = 0;

    {
        typename MapIdToJob::iterator it = map_parent_id_to_job_.find( parent_id );
        if( it == map_parent_id_to_job_.end() )
        {
            throw exception( "cannot find parent job " + std::to_string( parent_id ) );
        }

        JOB job = (*it).second;

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

template <class JOB, class JOB_ID>
template <typename ITERATOR>
bool JobManT<JOB,JOB_ID>::remove_jobs( ITERATOR begin, ITERATOR end )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    for( auto it = begin; it != end; ++it )
    {
        remove_job__( *it );
    }

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::remove_job_by_child_id( JOB_ID child_id )
{
    return remove_job( get_parent_id_by_child_id( child_id ) );
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::assign_child_id( JOB_ID parent_id, JOB_ID child_id )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    JOBMAN_ASSERT( parent_id );
    JOBMAN_ASSERT( child_id );

    JOB job = get_job_by_parent_job_id__( parent_id );

    JOB_ID curr_child_id = job->get_child_job_id();

    if( curr_child_id != 0 )
    {
        throw exception( "cannot assign child " + std::to_string( child_id ) + " to job id " + std::to_string( parent_id ) +
                " as it already has a child " + std::to_string( curr_child_id ) );
    }

    job->set_child_job_id( child_id );

    insert_job_to_child_map( child_id, job );

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::has_parent_job_id( JOB_ID id ) const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return map_parent_id_to_job_.count( id ) > 0;
}

template <class JOB, class JOB_ID>
JOB JobManT<JOB,JOB_ID>::get_job_by_parent_job_id( JOB_ID id )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return get_job_by_parent_job_id__( id );
}

template <class JOB, class JOB_ID>
const JOB JobManT<JOB,JOB_ID>::get_job_by_parent_job_id( JOB_ID id ) const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    return get_job_by_parent_job_id__( id );
}

template <class JOB, class JOB_ID>
JOB JobManT<JOB,JOB_ID>::get_job_by_parent_job_id__( JOB_ID id )
{
    // private: no mutex

    typename MapIdToJob::iterator it = map_parent_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_parent_id_to_job_.end() );

    return (*it).second;
}

template <class JOB, class JOB_ID>
const JOB JobManT<JOB,JOB_ID>::get_job_by_parent_job_id__( JOB_ID id ) const
{
    // private: no mutex

    typename MapIdToJob::const_iterator it = map_parent_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_parent_id_to_job_.end() );

    return (*it).second;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::insert_job_to_child_map( JOB_ID child_id, JOB job )
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

template <class JOB, class JOB_ID>
JOB JobManT<JOB,JOB_ID>::get_job_by_child_job_id( JOB_ID id )
{
    MUTEX_SCOPE_LOCK( mutex_ );

    typename MapIdToJob::iterator it = map_child_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_child_id_to_job_.end() );

    return (*it).second;
}

template <class JOB, class JOB_ID>
JOB_ID JobManT<JOB,JOB_ID>::get_child_id_by_parent_id( JOB_ID id )
{
    return get_job_by_parent_job_id( id )->get_child_job_id();
}

template <class JOB, class JOB_ID>
JOB_ID JobManT<JOB,JOB_ID>::get_parent_id_by_child_id( JOB_ID id )
{
    return get_job_by_child_job_id( id )->get_parent_job_id();
}

template <class JOB, class JOB_ID>
void JobManT<JOB,JOB_ID>::get_all_jobs( std::vector<JOB> & res ) const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    get_all_jobs__( res );
}

template <class JOB, class JOB_ID>
void JobManT<JOB,JOB_ID>::get_all_jobs__( std::vector<JOB> & res ) const
{
    std::transform(
            map_parent_id_to_job_.begin(), map_parent_id_to_job_.end(),
            std::back_inserter( res ), [] ( const typename MapIdToJob::value_type & p ) { return p.second; } );
}

template <class JOB, class JOB_ID>
template <typename _PRED>
void JobManT<JOB,JOB_ID>::find_jobs( std::vector<JOB> & res, _PRED pred ) const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    std::vector<JOB> temp;
    get_all_jobs__( temp );

    std::copy_if( temp.begin(), temp.end(), std::back_inserter( res ), pred );
}

template <class JOB, class JOB_ID>
template< typename _OutputIterator, typename _PRED>
void JobManT<JOB,JOB_ID>::find_job_ids( _OutputIterator res, _PRED pred ) const
{
    MUTEX_SCOPE_LOCK( mutex_ );

    std::vector<JOB> temp;
    get_all_jobs__( temp );

    std::vector<JOB> temp2;

    std::copy_if( temp.begin(), temp.end(), std::back_inserter( temp2 ), pred );

    std::transform(
                temp2.begin(), temp2.end(),
                res, [] ( const JOB & p ) { return p->get_parent_job_id(); } );
}

template <class JOB, class JOB_ID>
const typename JobManT<JOB,JOB_ID>::MapIdToJob & JobManT<JOB,JOB_ID>::get_job_map_and_lock() const
{
    MUTEX_LOCK( mutex_ );

    return map_parent_id_to_job_;
}

template <class JOB, class JOB_ID>
void JobManT<JOB,JOB_ID>::unlock() const
{
    MUTEX_UNLOCK( mutex_ );
}

NAMESPACE_JOBMAN_END

#endif  // GENERIC_JOB_MAN_T_H

