//  supercollider-style synthdef
//  Copyright (C) 2008-2012 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef SC_SYNTHDEF_HPP
#define SC_SYNTHDEF_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "utilities/malloc_aligned.hpp"
#include "utilities/named_hash_entry.hpp"

#include "SC_Types.h"
#include "SC_Wire.h"

namespace nova {

class sc_synthdef
{
    typedef c_string string;
    typedef std::vector<float, aligned_allocator<float> > fvector;
    typedef std::vector<c_string, aligned_allocator<string> > svector;

    typedef std::vector<char, aligned_allocator<char> > char_vector;

public:
    typedef std::map<string, int32_t, std::less<string>, aligned_allocator<string> > parameter_map_t;

    struct input_spec
    {
        input_spec(int32_t source, int32_t index):
            source(source), index(index)
        {}

        bool operator<(input_spec const & rhs) const
        {
            if (source < rhs.source)
                return true;
            if (source > rhs.source)
                return false;
            return index < rhs.index;
        }

        int32_t source;   /* index of ugen or -1 for constant */
        int32_t index;    /* number of output or constant index */
    };
    typedef std::vector<input_spec, aligned_allocator<input_spec> > input_spec_vector;

    struct unit_spec_t
    {
        explicit unit_spec_t(const char *& buffer, int version);

        unit_spec_t(string const & name, int16_t rate, int16_t special_index,
                    input_spec_vector const & in_specs,
                    char_vector const & out_specs):
            name(name), rate(rate), special_index(special_index),
            input_specs(in_specs), output_specs(out_specs)
        {}

        unit_spec_t(unit_spec_t && rhs):
            name(std::move(rhs.name)), rate(rhs.rate), special_index(rhs.special_index),
            input_specs(std::move(rhs.input_specs)), output_specs(std::move(rhs.output_specs)),
            buffer_mapping(std::move(rhs.buffer_mapping)), prototype(rhs.prototype)
        {}

        unit_spec_t(unit_spec_t const & rhs) = default;
        unit_spec_t & operator=(unit_spec_t const & rhs) = default;

        string name;
        int16_t rate;           /* 0: scalar rate, 1: buffer rate, 2: full rate, 3: demand rate */
        int16_t special_index;

        input_spec_vector input_specs;
        char_vector output_specs;      /* calculation rates */
        std::vector<int32_t, aligned_allocator<int32_t> > buffer_mapping;

        std::size_t memory_requirement(void)
        {
            return input_specs.size()  * (sizeof(Wire*) + sizeof(float*)) +
                   output_specs.size() * (sizeof(Wire*) + sizeof(float*)) +
                   output_specs.size() * sizeof(Wire);
        }

        class sc_ugen_def * prototype;
    };

    friend class sc_synth_definition;
    friend class sc_synth;
    friend class sc_ugen_factory;
    friend class sc_ugen_def;

    typedef std::vector<unit_spec_t, aligned_allocator<unit_spec_t> > graph_t;
    typedef std::vector<int32_t, aligned_allocator<int32_t> > calc_units_t;

    sc_synthdef(const char *& buffer, int version);

    sc_synthdef(sc_synthdef && rhs):
        name_(std::move(rhs.name_)), constants(std::move(rhs.constants)), parameters(std::move(rhs.parameters)),
        parameter_map(std::move(rhs.parameter_map)), graph(std::move(rhs.graph)), buffer_count(rhs.buffer_count),
        calc_unit_indices(std::move(rhs.calc_unit_indices)), memory_requirement_(rhs.memory_requirement_)
    {}

    sc_synthdef(sc_synthdef const & rhs) = default;
    sc_synthdef& operator=(sc_synthdef const & rhs) = default;

    std::string dump(void) const;

    string const & name(void) const
    {
        return name_;
    }

    std::size_t parameter_count(void) const
    {
        return parameters.size();
    }

    std::size_t unit_count(void) const
    {
        return graph.size();
    }

    std::size_t calc_unit_count(void) const
    {
        return calc_unit_indices.size();
    }

    std::size_t memory_requirement(void) const
    {
        assert(memory_requirement_);
        return memory_requirement_;
    }

private:
    void read_synthdef(const char *&, int version);

    /** assign buffers, collect memory requirement & cache ugen prototype */
    void prepare(void);

    string name_;
    fvector constants;
    fvector parameters;
    parameter_map_t parameter_map;

    graph_t graph;
    std::uint16_t buffer_count;
    calc_units_t calc_unit_indices; /**< indices of the units, that need to be calculated */
    std::size_t memory_requirement_;
};

std::vector<sc_synthdef> read_synthdefs(const char * buf_ptr);
std::vector<sc_synthdef> read_synthdef_file(boost::filesystem::path const & filename);

} /* namespace nova */

#endif /* SC_SYNTHDEF_HPP */
