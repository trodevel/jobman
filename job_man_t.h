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


// $Revision: 5767 $ $Date:: 2017-02-13 #$ $Author: serge $

#ifndef GENERIC_JOB_MAN_T_H
#define GENERIC_JOB_MAN_T_H

#include <map>                          // std::map
#include <stdexcept>                    // std::logic_error
#include <cassert>                      // assert
#include <algorithm>                    // std::transform
#include <functional>                   // std::bind

#define JOBMAN_ASSERT(_x)               assert(_x)
#include "namespace_lib.h"              // NAMESPACE_JOBMAN_START

NAMESPACE_JOBMAN_START

template <class JOB, class JOB_ID>
class JobManT
{
protected:

    typedef std::map<JOB_ID, JOB>       MapIdToJob;
    typedef std::map<JOB_ID, JOB_ID>    MapIdToId;

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

    bool insert_job( JOB_ID id, JOB job, JOB_ID child_id = 0 );
    bool remove_job( JOB_ID id );
    template <typename ITERATOR>
    bool remove_jobs( ITERATOR begin, ITERATOR end );
    bool remove_job_by_child_id( JOB_ID child_id );
    bool assign_id_with_child_id( JOB_ID id, JOB_ID child_id );
    bool unassign_child_id_from_id( JOB_ID id );

    bool has_job( JOB_ID id ) const;
    JOB get_job( JOB_ID id );
    const JOB get_job( JOB_ID id ) const;
    JOB get_job_by_child_id( JOB_ID id );

    JOB_ID get_child_id_by_id( JOB_ID id );
    JOB_ID get_id_by_child_id( JOB_ID id );

    void get_all_jobs( std::vector<JOB> & res ) const;
    template< typename _PRED>
    void find_jobs( std::vector<JOB> & res, _PRED pred ) const;

    template< typename _OutputIterator, typename _PRED>
    void find_job_ids( _OutputIterator res, _PRED pred ) const;

    const MapIdToJob & get_job_map() const;

protected:

    MapIdToJob          map_id_to_job_;
    MapIdToId           map_child_id_to_id_;
    MapIdToId           map_id_to_child_id_;
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
bool JobManT<JOB,JOB_ID>::insert_job( JOB_ID id, JOB job, JOB_ID child_id )
{
    if( map_id_to_job_.insert( typename MapIdToJob::value_type( id, job ) ).second == false )
    {
        throw fatal_exception( "job " + std::to_string( id ) + " already exists" );
    }

    if( child_id != 0 )
        assign_id_with_child_id( id, child_id );

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::remove_job( JOB_ID id )
{
    {
        auto it = map_id_to_job_.find( id );
        if( it == map_id_to_job_.end() )
        {
            throw exception( "cannot find parent job " + std::to_string( id ) );
        }

        JOB job = (*it).second;

        map_id_to_job_.erase( it );
    }

    unassign_child_id_from_id( id );

    return true;
}

template <class JOB, class JOB_ID>
template <typename ITERATOR>
bool JobManT<JOB,JOB_ID>::remove_jobs( ITERATOR begin, ITERATOR end )
{
    for( auto it = begin; it != end; ++it )
    {
        remove_job( *it );
    }

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::remove_job_by_child_id( JOB_ID child_id )
{
    return remove_job( get_id_by_child_id( child_id ) );
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::assign_id_with_child_id( JOB_ID id, JOB_ID child_id )
{
    JOBMAN_ASSERT( child_id );
    JOBMAN_ASSERT( id );

    if( map_child_id_to_id_.insert( typename MapIdToId::value_type( child_id, id ) ).second == false )
    {
        throw fatal_exception(
                        "child job " + std::to_string( child_id ) +
                        " is already present ( id " + std::to_string( id ) + " )" );
    }

    if( map_id_to_child_id_.insert( typename MapIdToId::value_type( id, child_id ) ).second == false )
    {
        throw fatal_exception(
                        "job " + std::to_string( id ) +
                        " has already child id ( child id " + std::to_string( child_id ) + " )" );
    }

    return true;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::unassign_child_id_from_id( JOB_ID id )
{
    auto it = map_id_to_child_id_.find( id );

    if( it == map_id_to_child_id_.end() )
    {
        return false;
    }

    auto child_id = it->second;

    map_id_to_child_id_.erase( it );

    auto it_child = map_child_id_to_id_.find( child_id );

    if( it_child == map_child_id_to_id_.end() )
    {
        throw fatal_exception(
            "child id " + std::to_string( child_id ) +
            " with parent id " + std::to_string( id ) +
            " not found in the child map" );
    }

    map_child_id_to_id_.erase( it_child );

    return false;
}

template <class JOB, class JOB_ID>
bool JobManT<JOB,JOB_ID>::has_job( JOB_ID id ) const
{
    return map_id_to_job_.count( id ) > 0;
}

template <class JOB, class JOB_ID>
JOB JobManT<JOB,JOB_ID>::get_job( JOB_ID id )
{
    auto it = map_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_id_to_job_.end() );

    return (*it).second;
}

template <class JOB, class JOB_ID>
const JOB JobManT<JOB,JOB_ID>::get_job( JOB_ID id ) const
{
    auto it = map_id_to_job_.find( id );

    JOBMAN_ASSERT( it != map_id_to_job_.end() );

    return (*it).second;
}

template <class JOB, class JOB_ID>
JOB JobManT<JOB,JOB_ID>::get_job_by_child_id( JOB_ID child_id )
{
    auto id = get_id_by_child_id( child_id );

    return get_job( id );
}

template <class JOB, class JOB_ID>
JOB_ID JobManT<JOB,JOB_ID>::get_child_id_by_id( JOB_ID id )
{
    auto it = map_id_to_child_id_.find( id );

    if( it != map_id_to_child_id_.end() )
        return (*it).second;

    return 0;
}

template <class JOB, class JOB_ID>
JOB_ID JobManT<JOB,JOB_ID>::get_id_by_child_id( JOB_ID id )
{
    auto it = map_child_id_to_id_.find( id );

    if( it != map_child_id_to_id_.end() )
        return (*it).second;

    return 0;
}

template <class JOB, class JOB_ID>
void JobManT<JOB,JOB_ID>::get_all_jobs( std::vector<JOB> & res ) const
{
    std::transform(
            map_id_to_job_.begin(), map_id_to_job_.end(),
            std::back_inserter( res ), [] ( const typename MapIdToJob::value_type & p ) { return p.second; } );
}

template <class JOB, class JOB_ID>
template <typename _PRED>
void JobManT<JOB,JOB_ID>::find_jobs( std::vector<JOB> & res, _PRED pred ) const
{
    std::vector<JOB> temp;
    get_all_jobs( temp );

    std::copy_if( temp.begin(), temp.end(), std::back_inserter( res ), pred );
}

template <class JOB, class JOB_ID>
template< typename _OutputIterator, typename _PRED>
void JobManT<JOB,JOB_ID>::find_job_ids( _OutputIterator res, _PRED pred ) const
{
    std::vector<JOB> temp;
    get_all_jobs( temp );

    std::vector<JOB> temp2;

    std::copy_if( temp.begin(), temp.end(), std::back_inserter( temp2 ), pred );

    std::transform(
                temp2.begin(), temp2.end(),
                res, [] ( const JOB & p ) { return p->get_job_id(); } );
}

template <class JOB, class JOB_ID>
const typename JobManT<JOB,JOB_ID>::MapIdToJob & JobManT<JOB,JOB_ID>::get_job_map() const
{
    return map_id_to_job_;
}

NAMESPACE_JOBMAN_END

#endif  // GENERIC_JOB_MAN_T_H

