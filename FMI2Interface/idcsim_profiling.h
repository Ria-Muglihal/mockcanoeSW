#ifndef __IDCSIM_PROFILING_H__
#define __IDCSIM_PROFILING_H__

#include <chrono>
#include <memory>
#include <string>

namespace idcsim
{
    // there is an issue with std::chrono::high_precision_clock prior to VS2015 thus we define our own highp clock
    struct qpc_clock
    {
        typedef std::chrono::nanoseconds duration; // nanoseconds resolution
        typedef duration::rep rep;
        typedef duration::period period;
#ifdef _WIN32
		typedef std::chrono::time_point< qpc_clock, duration > time_point;
#elif __linux
		typedef  std::chrono::time_point<std::chrono::system_clock,duration> time_point;
#endif 
        static bool is_steady;
        static time_point now( );
    };

    class Profiler
    {
    public:
        static Profiler& instance( );

        ~Profiler( );

        void EnterScope( const std::string& identifier );

        void LeaveScope( );

        void StartProfiling( );

        void StopProfiling( ) const;

        ///! Logs the current profiling data to the end of the specified file
        void Log( const std::string& filename );

    private:
        Profiler( );

        class Scope
        {
        public:
            explicit Scope( const std::string& _name );

            ~Scope( );

            void Log( std::ostream& ostr ) const;

            size_t Level( ) const;

            Scope* FindChild( const std::string& name ) const;

            std::string name;
			qpc_clock::duration time = std::chrono::nanoseconds::zero();
			qpc_clock::duration max_time = std::chrono::nanoseconds::zero();
			qpc_clock::duration min_time = std::chrono::nanoseconds::zero();
            qpc_clock::time_point start_time;
            size_t number_of_calls = 0;

            Scope* sibling_scope_ptr = nullptr;
            Scope* child_scope_ptr = nullptr;
            Scope* parent_scope_ptr = nullptr;
        };

        Scope* m_scope_ptr = nullptr;
        std::unique_ptr< Scope > m_root_scope_ptr;
    };
}

#ifdef IDCSIM_ENABLE_PROFILING
/// Creates or enters the scope S and starts profiling
#define IDCSIM_START_PROFILING( S ) \
    idcsim::Profiler::instance( ).EnterScope( S ); \
    idcsim::Profiler::instance( ).StartProfiling( );

/// Leaves the current scope and stops profiling
#define IDCSIM_STOP_PROFILING( ) \
    idcsim::Profiler::instance( ).StopProfiling( ); \
    idcsim::Profiler::instance( ).LeaveScope( );
#else
#define IDCSIM_START_PROFILING( S )
#define IDCSIM_STOP_PROFILING( )
#endif
#endif
