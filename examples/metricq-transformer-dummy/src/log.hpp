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
#pragma once

#include <nitro/log/attribute/jiffy.hpp>
#include <nitro/log/attribute/severity.hpp>
#include <nitro/log/filter/severity_filter.hpp>
#include <nitro/log/log.hpp>
#include <nitro/log/sink/stdout.hpp>

namespace detail
{
using record = nitro::log::record<nitro::log::tag_attribute, nitro::log::message_attribute,
                                  nitro::log::severity_attribute, nitro::log::jiffy_attribute>;

template <typename Record>
class log_formater
{
public:
    std::string format(Record& r)
    {
        std::stringstream s;

        s << "[" << r.jiffy() << "][";

        if (!r.tag().empty())
        {
            s << r.tag() << "][";
        }

        s << r.severity() << "]: " << r.message() << '\n';

        return s.str();
    }
};

template <typename Record>
using log_filter = nitro::log::filter::severity_filter<Record>;
} // namespace detail

using Log = nitro::log::logger<detail::record, detail::log_formater, nitro::log::sink::StdOut,
                               detail::log_filter>;

inline void set_severity(nitro::log::severity_level level)
{
    nitro::log::filter::severity_filter<detail::record>::set_severity(level);
}

void initialize_logger();
