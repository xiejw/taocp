// vim: ft=cpp
//
// A simple wall-clock timer on CLOCK_MONOTONIC. Header-only.
//
// Usage:
//
//   taocp::sat::Timer timer;
//
//   timer.start( );
//   emitBench( &solver, &bench );
//   timer.stopAndReport( "emitBench" );
//
//   timer.start( );  // one Timer can be reused
//   auto res = solver.searchOneSolution( );
//   timer.stopAndReport( "searchOneSolution" );
//
// Prints, e.g.: time spent on `emitBench`: 0.011 ms
//
// Recommendations:
//
// - Keep I/O (printing clauses, logging) out of the timed region; it can
//   easily cost more than the work being measured.
// - Measure optimized builds (`make RELEASE=1`); debug timings mislead.
// - Small benchmarks finish in microseconds. For stable numbers, time a loop
//   of many runs and divide, rather than trusting a single run.
// - This is wall-clock time: other processes and the OS add noise. Run a few
//   times and look at the minimum.
// - The message is printed in pink using ANSI escapes, even when stdout is
//   not a terminal.
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <string_view>

namespace taocp::sat {

/// ANSI escape for pink text (256-color 213).
inline constexpr const char *kColorPink = "\033[38;5;213m";
/// ANSI escape that resets the text color.
inline constexpr const char *kColorReset = "\033[0m";

/// Measures elapsed wall-clock time. Call `start` and then `stopAndReport`.
struct Timer {
    /// Records the current time as the start of the measurement.
    auto start( ) -> void { start_ns = nowNs( ); }

    /// Prints the time elapsed since `start` to stdout, as
    /// "time spent on `<msg>`: <t> ms".
    auto stopAndReport( std::string_view msg ) -> void
    {
        uint64_t elapsed_ns = nowNs( ) - start_ns;
        ::printf( "time spent on `%s%.*s%s`: %.3f ms\n", kColorPink,
                  static_cast<int>( msg.size( ) ), msg.data( ), kColorReset,
                  static_cast<double>( elapsed_ns ) / 1e6 );
    }

  private:
    uint64_t start_ns = 0;

    static auto nowNs( ) -> uint64_t
    {
        struct timespec ts;
        ::clock_gettime( CLOCK_MONOTONIC, &ts );
        return static_cast<uint64_t>( ts.tv_sec ) * 1000000000ull +
               static_cast<uint64_t>( ts.tv_nsec );
    }
};

}  // namespace taocp::sat
