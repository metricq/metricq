// Copyright (c) 2019, ZIH,
// Technische Universitaet Dresden,
// Federal Republic of Germany
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

#include <metricq/metadata.hpp>

namespace metricq
{

Metadata::Metadata(const metricq::json& m) : metadata_(m)
{
}

void Metadata::json(const metricq::json& m)
{
    if (m.is_null())
    {
        metadata_ = metricq::json::object();
        return;
    }
    assert(m.is_object());
    metadata_ = m;
}

const metricq::json& Metadata::json() const
{
    return metadata_;
}

metricq::json& Metadata::operator[](const std::string& key)
{
    return metadata_[key];
}

const metricq::json& Metadata::operator[](const std::string& key) const
{
    return metadata_.at(key);
}

/*
 * Standardized metadata methods
 */
void Metadata::unit(const std::string& u)
{
    (*this)["unit"] = u;
}

std::string Metadata::unit() const
{
    if (metadata_.count("unit"))
    {
        return (*this)["unit"];
    }
    return "";
}

void Metadata::description(const std::string& d)
{
    (*this)["description"] = d;
}

std::string Metadata::description() const
{
    if (metadata_.count("description"))
    {
        return (*this)["description"];
    }
    return "";
}

void Metadata::rate(double r)
{
    (*this)["rate"] = r;
}

double Metadata::rate() const
{
    if (metadata_.count("rate"))
    {
        return (*this)["rate"];
    }
    return nan("");
}

void Metadata::scope(Metadata::Scope s)
{
    if (s == Metadata::Scope::last)
    {
        (*this)["scope"] = "last";
    }
    else if (s == Metadata::Scope::next)
    {
        (*this)["scope"] = "next";
    }
    else if (s == Metadata::Scope::point)
    {
        (*this)["scope"] = "point";
    }
    else
    {
        assert(false);
    }
}

void Metadata::operator()(Metadata::Scope s)
{
    scope(s);
}

Metadata::Scope Metadata::scope() const
{
    if (!metadata_.count("scope"))
    {
        return Metadata::Scope::unknown;
    }

    std::string s = (*this)["scope"];

    if (s == "last")
    {
        return Metadata::Scope::last;
    }
    else if (s == "next")
    {
        return Metadata::Scope::next;
    }
    else if (s == "point")
    {
        return Metadata::Scope::point;
    }

    // tis really bad
    assert(false);
    return Metadata::Scope::unknown;
}

} // namespace metricq
