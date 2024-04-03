/*******************************************************************************
 * Copyright (c) 2024. Liu Xiangzhi
 * This file is part of CAMI.
 *
 * CAMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 2 of the License, or any later version.
 *
 * CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with CAMI.
 * If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef CAMI_AM_TRACE_DATA_H
#define CAMI_AM_TRACE_DATA_H

#include <lib/utils.h>
#include <lib/compiler_guarantee.h>
#include <lib/array.h>
#include "exception.h"

namespace cami::am {
class FullExprInfo
{
public:
    // number of evaluation of identifier or function call or object deletion in one full expression
    const uint64_t trace_event_cnt;
    lib::Array<uint8_t> sequence_after_graph;
    lib::Array<std::pair<uint64_t, uint64_t>> source_location; // (line, colum) indexed by inner_id

public:
    FullExprInfo(const uint64_t trace_event_cnt, lib::Array<uint8_t> graph,
                 lib::Array<std::pair<uint64_t, uint64_t>> source_location)
            : trace_event_cnt(trace_event_cnt), sequence_after_graph(std::move(graph)),
              source_location(std::move(source_location))
    {
        ASSERT(this->sequence_after_graph.length() >= lib::roundUpDiv(trace_event_cnt * trace_event_cnt, 8),
               "invalid graph bitmap length");
        ASSERT(this->source_location.length() == this->trace_event_cnt, "invalid source_location length");
    }

    bool isSequenceAfter(uint32_t id1, uint32_t id2)
    {
        COMPILER_GUARANTEE(id1 < this->trace_event_cnt && id2 < this->trace_event_cnt, lib::format(
                "Value of id of tag1 or tag2(${}, ${}) out of boundary(${})", id1, id2, this->trace_event_cnt));
        auto idx = id1 * this->trace_event_cnt + id2;
        return this->sequence_after_graph[idx / 8] & (1 << (idx % 8));
    }
};

class InnerID
{
    uint32_t id;

    explicit InnerID(uint32_t id) : id(id) {}

public:
    // just borrow a bit from inner id.
    static InnerID newCoexisting(uint32_t id)
    {
        return InnerID{(id << 1) | 1};
    }

    static InnerID newMutualExclude(uint32_t id)
    {
        return InnerID{id << 1};
    }

    [[nodiscard]] bool isCoexisting() const noexcept
    {
        return this->id & 1;
    }

    [[nodiscard]] uint32_t value() const noexcept
    {
        return this->id >> 1;
    }
};

struct TraceLocation
{
    const uint64_t exec_id;
    const uint32_t full_expr_id;
    const InnerID inner_id;
};

struct TraceContext
{
    TraceContext& caller;
    TraceLocation call_point;
    const uint32_t func_id;
private:
    uint32_t ref_cnt = 1;
private:
    ~TraceContext()
    {
        this->caller.release();
    }

    // only used by `dummy`
    TraceContext() : caller(*this), func_id(-1), ref_cnt(INT32_MAX),
                     call_point{.exec_id=UINT64_MAX, .full_expr_id = UINT32_MAX,
                             .inner_id = InnerID::newMutualExclude(INT32_MAX)} {}

public:
    static TraceContext dummy;
public:
    TraceContext(TraceContext& caller, TraceLocation location, uint32_t func_id) // NOLINT
            : caller(caller), call_point(location), func_id(func_id)
    {
        this->caller.acquire();
    }

    TraceContext(const TraceContext&) = delete;
    TraceContext(TraceContext&&) = delete;
    TraceContext& operator=(const TraceContext&) = delete;
    TraceContext& operator=(TraceContext&&) = delete;

    void acquire()
    {
        if (this->ref_cnt++ == UINT32_MAX) {
            throw ReferenceCountingOverflowException{"TraceContext"};
        }
    }

    void release()
    {
        if (--this->ref_cnt == 0 && !this->isDummy()) {
            delete this;
        }
    }

    [[nodiscard]] bool isDummy() const noexcept
    {
        return this == &dummy;
    }
};
} // namespace cami::am

#endif //CAMI_AM_TRACE_DATA_H
