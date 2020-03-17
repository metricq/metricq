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

#include "summary.hpp"

#include <algorithm>
#include <cmath>

Summary Summary::calculate(std::vector<metricq::TimeValue>&& tv_pairs,
                           metricq::Duration start_delta, metricq::Duration stop_delta)
{
    auto begin = tv_pairs.begin();
    auto end = tv_pairs.end();

    auto start = tv_pairs.front().time;
    auto stop = tv_pairs.back().time;

    tv_pairs.erase(std::remove_if(begin, end,
                                  [&start, stop, start_delta, stop_delta](metricq::TimeValue& tv) {
                                      return tv.time <= start + start_delta ||
                                             tv.time >= stop - stop_delta;
                                  }),
                   end);

    begin = tv_pairs.begin();
    end = tv_pairs.end();

    assert(tv_pairs.size() > 0);

    Summary summary{};

    summary.num_timepoints = tv_pairs.size();
    summary.duration = tv_pairs.back().time - tv_pairs.front().time;

    auto sum_over_nths = [&begin, end, summary](auto fn) {
        double acc = 0.0;
        for (auto it = begin; it != end; ++it)
        {
            acc += fn(it->value);
        }
        return acc / summary.num_timepoints;
    };

    summary.average = sum_over_nths([](metricq::Value v) { return v; });

    summary.stddev = std::sqrt(sum_over_nths([&summary](metricq::Value v) {
        double centered = v - summary.average;
        return centered * centered;
    }));

    summary.absdev =
        sum_over_nths([&summary](metricq::Value v) { return std::abs(v - summary.average); });

    std::sort(begin, end, [](metricq::TimeValue& tv1, metricq::TimeValue& tv2) {
        return tv1.value < tv2.value;
    });

    std::size_t n = summary.num_timepoints;
    if (n % 2 != 0)
    {
        // odd amount of values, use central value as median
        summary.quart50 = tv_pairs[n / 2 + 1].value;
    }
    else
    {
        // even amount of values, use average of the two central values
        summary.quart50 = 0.5 * (tv_pairs[n / 2].value + tv_pairs[n / 2 + 1].value);
    }

    summary.quart25 = tv_pairs[n / 4].value;
    summary.quart75 = tv_pairs[(n * 3) / 4].value;

    summary.minimum = tv_pairs.front().value;
    summary.maximum = tv_pairs.back().value;
    summary.range = summary.maximum - summary.minimum;

    return summary;
}
