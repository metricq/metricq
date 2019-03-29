// Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of metricq nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include "stress_test_source.hpp"

#include <metricq/logger/nitro.hpp>

#include <nitro/broken_options/parser.hpp>

#include <chrono>
#include <iostream>
#include <string>

using Log = metricq::logger::nitro::Log;

int main(int argc, char* argv[])
{
    metricq::logger::nitro::set_severity(nitro::log::severity_level::info);

    nitro::broken_options::parser parser;
    parser.option("server", "The metricq management server to connect to.")
        .default_value("amqp://localhost")
        .short_name("s");
    parser.option("token", "The token used for source authentication against the metricq manager.")
        .default_value("source-stress-test");
    parser.toggle("verbose").short_name("v");
    parser.toggle("trace").short_name("t");
    parser.toggle("quiet").short_name("q");
    parser.toggle("help").short_name("h");

    try
    {
        auto options = parser.parse(argc, argv);

        if (options.given("help"))
        {
            parser.usage();
            return 0;
        }

        if (options.given("trace"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::trace);
        }
        else if (options.given("verbose"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::debug);
        }
        else if (options.given("quiet"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::warn);
        }

        metricq::logger::nitro::initialize();

        StressTestSource source(options.get("server"), options.get("token"));

        Log::debug() << "starting main loop.";
        source.main_loop();
        auto end = metricq::Clock::now();
        Log::debug() << "exiting main loop.";
        // Use begin time after connection
        // But wait for connection all send buffers to empty and connection to close at the end
        auto seconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(end - source.first_time_)
                .count();
        Log::info() << "published " << source.total_values << " values total " << seconds << ": "
                    << (source.total_values / seconds) << " values/s";
    }
    catch (nitro::broken_options::parsing_error& e)
    {
        std::cerr << e.what() << '\n';
        parser.usage();
        return 1;
    }
    catch (std::exception& e)
    {
        Log::error() << "Unhandled exception: " << e.what();
    }
}
