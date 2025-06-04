#include "idcsim_profiling.h"
#include <cassert>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <iomanip>

#define NOMINMAX
#ifdef _WIN32
#include "windows.h"
#endif

namespace
{
    const long long g_frequency = [ ]( ) -> long long
    {
#ifdef _WIN32
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency( &frequency );
        return frequency.QuadPart;
#elif __linux
        return 1;
#endif
    }( );
}

namespace idcsim
{
    bool qpc_clock::is_steady = true;

    qpc_clock::time_point qpc_clock::now( )
    {
#ifdef _WIN32
        LARGE_INTEGER counter;
        QueryPerformanceCounter( &counter );
        return time_point( duration( counter.QuadPart * static_cast< rep >( period::den ) / g_frequency ) );
#elif __linux
        return time_point(std::chrono::system_clock::now() ) ;
#endif
    }

    Profiler& Profiler::instance( )
    {
        static Profiler p;
        return p;
    }

    Profiler::~Profiler( )
    {
        Log( "fmu_interface_profiling.log" );
    }

    void Profiler::EnterScope( const std::string& identifier )
    {
        assert( m_scope_ptr != nullptr );

        auto scope_ptr = m_scope_ptr->FindChild( identifier );

        if( !scope_ptr )
        {
            if( m_scope_ptr->child_scope_ptr )
            {
                Scope* child_ptr = m_scope_ptr->child_scope_ptr;

                while( child_ptr )
                {
                    if( child_ptr->sibling_scope_ptr == nullptr )
                    {
                        child_ptr->sibling_scope_ptr = new Scope( identifier );
                        child_ptr->sibling_scope_ptr->parent_scope_ptr = m_scope_ptr;
                        scope_ptr = child_ptr->sibling_scope_ptr;
                        break;
                    }

                    child_ptr = child_ptr->sibling_scope_ptr;
                }
            }
            else
            {
                m_scope_ptr->child_scope_ptr = new Scope( identifier );
                m_scope_ptr->child_scope_ptr->parent_scope_ptr = m_scope_ptr;
                scope_ptr = m_scope_ptr->child_scope_ptr;
            }
        }

        assert( scope_ptr != nullptr );

        m_scope_ptr = scope_ptr;
    }

    void Profiler::LeaveScope( )
    {
        assert( m_scope_ptr != nullptr );
        assert( m_scope_ptr != m_root_scope_ptr.get( ) );

        // just to not make the application crash because of the profiler
        if( m_scope_ptr->parent_scope_ptr != nullptr )
        {
            m_scope_ptr = m_scope_ptr->parent_scope_ptr;
        }
    }

    void Profiler::StartProfiling( )
    {
        m_scope_ptr->start_time = qpc_clock::now( );
    }

    void Profiler::StopProfiling( ) const
    {
        m_scope_ptr->number_of_calls++;

        auto time = qpc_clock::now( ) - m_scope_ptr->start_time;

        m_scope_ptr->time += time;
        m_scope_ptr->max_time = max( time, m_scope_ptr->max_time );
        m_scope_ptr->min_time = min( time, m_scope_ptr->min_time );
    }

    void Profiler::Log( const std::string& filename )
    {
        assert( m_root_scope_ptr );

        std::ofstream ofstr( filename, std::ios_base::out | std::ios_base::app );

        if( ofstr )
        {
            auto now_c = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );

            ofstr << "********************************************************************************" << std::endl;
#ifdef _WIN32
            ofstr << "Profiling session from " << std::put_time( localtime( &now_c ), "%c" ) << std::endl;
#elif __linux
//std::put_time has not been implemented till GCC5.0
//TODO: fix this with older gnuc version
#if ( __GNUC__ >= 5 )
            ofstr << "Profiling session from " << std::put_time( localtime( &now_c ), "%c" ) << std::endl;
#endif
#endif
            ofstr << "********************************************************************************" << std::endl;

            auto ptr = m_root_scope_ptr->child_scope_ptr;

            while( ptr )
            {
                ptr->Log( ofstr );
                ptr = ptr->sibling_scope_ptr;
            }
        }
    }

    Profiler::Profiler( )
        : m_root_scope_ptr( new Scope( "root" ) )
    {
        m_scope_ptr = m_root_scope_ptr.get( );
    }

    Profiler::Scope::Scope( const std::string& _name )
        : name( _name )
        , min_time( std::numeric_limits< qpc_clock::rep >::max( ) )
    {
    }

    Profiler::Scope::~Scope( )
    {
        if( sibling_scope_ptr )
        {
            delete( sibling_scope_ptr );
            sibling_scope_ptr = nullptr;
        }

        if( child_scope_ptr )
        {
            delete child_scope_ptr;
            child_scope_ptr = nullptr;
        }
    }

    void Profiler::Scope::Log( std::ostream& ostr ) const
    {
        std::string prefix;

        for( auto n = 0u; n < Level( ); ++n )
        {
            prefix += '\t';
        }

        auto ms = std::chrono::duration_cast< std::chrono::milliseconds >( time );

        ostr << prefix << "<" << name << ">" << std::endl;

        ostr << prefix << '\t' << "Number of calls: " << number_of_calls << std::endl;

        if( ms.count( ) > 0 )
        {
            ostr << prefix << '\t' << "Total Time: " << ms.count( ) << "ms" << std::endl;
        }
        else
        {
            ostr << prefix << '\t' << "Total Time: " << "N/A" << std::endl;
        }

        if( ms.count( ) > 0 )
        {
            ostr << prefix << '\t' << "Avg. Time/Call: " << std::fixed << std::setprecision( 2 ) << ( double( ms.count( ) ) / double( number_of_calls ) ) << "ms" << std::endl;
        }
        else
        {
            ostr << prefix << '\t' << "Avg. Time/Call: " << "N/A" << std::endl;
        }

        if( ms.count( ) > 0 )
        {
            ostr << prefix << '\t' << "Min Time: " << std::chrono::duration_cast< std::chrono::milliseconds >( min_time ).count( ) << "ms" << std::endl;
        }
        else
        {
            ostr << prefix << '\t' << "Min Time: " << "N/A" << std::endl;
        }

        if( ms.count( ) > 0 )
        {
            ostr << prefix << '\t' << "Max Time: " << std::chrono::duration_cast< std::chrono::milliseconds >( max_time ).count( ) << "ms" << std::endl;
        }
        else
        {
            ostr << prefix << '\t' << "Max Time: " << "N/A" << std::endl;
        }

        auto ptr = child_scope_ptr;

        while( ptr )
        {
            ptr->Log( ostr );
            ptr = ptr->sibling_scope_ptr;
        }

        ostr << prefix << "</" << name << ">" << std::endl;
    }

    size_t Profiler::Scope::Level( ) const
    {
        if( parent_scope_ptr == nullptr )
        {
            return 0;
        }

        return parent_scope_ptr->Level( ) + 1;
    }

    Profiler::Scope* Profiler::Scope::FindChild( const std::string& name ) const
    {
        Scope* scope_ptr = nullptr;

        if( child_scope_ptr )
        {
            Scope* child_ptr = child_scope_ptr;

            while( child_ptr )
            {
                if( child_ptr->name == name )
                {
                    scope_ptr = child_ptr;
                    break;
                }

                child_ptr = child_ptr->sibling_scope_ptr;
            }
        }

        return scope_ptr;
    }
}
