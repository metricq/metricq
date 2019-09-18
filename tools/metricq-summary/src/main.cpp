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

#include "run_command.hpp"
#include "summary.hpp"

#include <nitro/broken_options/parser.hpp>

#include <metricq/logger/nitro.hpp>
#include <metricq/simple.hpp>

#include <iomanip>
#include <iostream>
#include <string>

using Log = metricq::logger::nitro::Log;

struct Config
{
    std::string url;
    std::string token;
    std::chrono::minutes timeout;
    std::vector<std::string> metrics;
    std::vector<std::string> cmdline;

    Config(int argc, const char* argv[]);
};

Config::Config(int argc, const char* argv[])
{
    metricq::logger::nitro::set_severity(nitro::log::severity_level::info);

    nitro::broken_options::parser parser;
    parser.option("server", "The metricq management server to connect to.")
        .default_value("amqp://localhost")
        .env("METRICQ_SERVER")
        .short_name("s");
    parser.option("token", "Token used for source authentication against the metricq manager.")
        .default_value("summary");
    parser.multi_option("metrics", "MetricQ metrics!").short_name("m").env("METRICQ_METRICS");
    parser.option("timeout", "Time-To-Live for data queue (in minutes)")
        .default_value("30")
        .short_name("T");
    parser.toggle("verbose").short_name("v");
    parser.toggle("trace").short_name("t");
    parser.toggle("quiet").short_name("q");
    parser.toggle("help").short_name("h");

    parser.accept_positionals();
    parser.positional_name("command [args]");

    try
    {
        auto options = parser.parse(argc, argv);

        if (options.given("help"))
        {
            parser.usage();
            std::exit(EXIT_SUCCESS);
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

        url = options.get("server");
        token = options.get("token");
        metrics = options.get_all("metrics");

        try
        {
            timeout = std::chrono::minutes(std::stoul(options.get("timeout")));
        }
        catch (const std::logic_error&)
        {
            std::cerr << "Invalid timeout: " << options.get("timeout") << '\n';
            std::exit(EXIT_FAILURE);
        }

        cmdline = options.positionals();
        if (cmdline.size() == 0)
        {
            std::cerr << "No command specified" << '\n';
            parser.usage();
            std::exit(EXIT_FAILURE);
        }
    }
    catch (nitro::broken_options::parsing_error& e)
    {
        std::cerr << "Error parsing arguments: " << e.what() << '\n';
        parser.usage();
        std::exit(EXIT_FAILURE);
    }
    catch (std::exception& e)
    {
        Log::error() << "Unhandled exception: " << e.what();
        parser.usage();
        std::exit(EXIT_FAILURE);
    }
}

void summary_csv_print(std::ostream& os,
                       std::unordered_map<std::string, std::vector<metricq::TimeValue>>&& stats)
{
    std::cout << "metric,"
                 "num_timepoints,"
                 "duration_ns,"
                 "average,stddev,absdev,"
                 "quart25,quart50,quart75,"
                 "minimum,maximum,range\n";

    for (auto& [metric, values] : stats)
    {
        if (values.empty())
        {
            Log::warn() << "no values recorded for metric " << metric;
            continue;
        }

        Summary sum = Summary::calculate(std::move(values));
        auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(sum.duration).count();
        // clang-format off
        os << std::quoted(metric) << ','
           << sum.num_timepoints << ','
           << duration_ns << ','
           << sum.average << ','
           << sum.stddev << ','
           << sum.absdev << ','
           << sum.quart25 << ','
           << sum.quart50 << ','
           << sum.quart75 << ','
           << sum.minimum << ','
           << sum.maximum << ','
           << sum.range << '\n';
        // clang-format on
    }
}

int main(int argc, const char* argv[])
{
    try
    {
        Config cfg{ argc, argv };
        auto queue = metricq::subscribe(cfg.url, cfg.token, cfg.metrics, cfg.timeout);
        Log::info() << "Subscribed to queue " << queue;

        Log::info() << "Executing " << cfg.cmdline[0] << "...";
        int status = run_command(cfg.cmdline);

        Log::info() << "Draining queue " << queue;
        auto stats = metricq::drain(cfg.url, cfg.token, cfg.metrics, queue);

        summary_csv_print(std::cout, std::move(stats));

        return status;
    }
    catch (const std::runtime_error& e)
    {
        Log::error() << e.what();
        std::exit(EXIT_FAILURE);
    }
}
