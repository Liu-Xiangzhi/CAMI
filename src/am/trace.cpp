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

#include <set>
#include <trace.h>
#include <formatter.h>
#include <fetch_decode.h>
#include <lib/list.h>

using namespace cami;
using namespace am;

TraceContext TraceContext::dummy{};
namespace {
TraceContext& findShortestCommonAncestor(TraceContext& _a, TraceContext& _b)
{
    auto a = &_a;
    auto b = &_b;
    // note that for root TraceContext r, it statisfy that `&r.caller == &r`
    std::set<TraceContext*> found;
    const auto isFound = [&](TraceContext* context) {
        return found.find(context) != found.end();
    };
    while (true) {
        if (isFound(a)) {
            return *a;
        }
        found.insert(a);
        a = &a->caller;
        if (isFound(b)) {
            return *b;
        }
        found.insert(b);
        b = &b->caller;
    }
}
} // anonymous namespace

bool Trace::isIndeterminatelySequenced(AbstractMachine& am, const Object::Tag& new_, const Object::Tag& old)
{
    auto& context = findShortestCommonAncestor(new_.context, old.context);
    const auto isSequenced = [&](const TraceLocation& new_loc, const TraceLocation& old_loc) {
        ASSERT(new_loc.exec_id >= old_loc.exec_id, "you created a time machine?!");
        if (new_loc.exec_id > old_loc.exec_id) {
            return true;
        }
        ASSERT(new_loc.full_expr_id == old_loc.full_expr_id, "same execute id ==> same full expression id");
        return am.static_info.functions[context.func_id].full_expr_infos[new_loc.full_expr_id]
                .isSequenceAfter(new_loc.inner_id.value(), old_loc.inner_id.value());
    };
    const auto findDivergence = [&](const Object::Tag& t) -> TraceContext& {
        auto ctx = &t.context;
        for (; &ctx->caller != &context; ctx = &ctx->caller) {}
        return *ctx;
    };
    if (&context == &new_.context && &context == &old.context) {
        return !isSequenced(new_.access_point, old.access_point);
    }
    if (&context == &old.context) {
        return !isSequenced(findDivergence(new_).call_point, old.access_point);
    }
    if (&context == &new_.context) {
        return !isSequenced(new_.access_point, findDivergence(old).call_point);
    }
    return !isSequenced(findDivergence(new_).call_point, findDivergence(old).call_point);
}

void Trace::updateTag(AbstractMachine& am, Object& obj, const Object::Tag& tag)
{
    if (obj.tags.empty()) {
        obj.tags.insert_head(tag);
        return;
    }
    if (tag.isCoexisting() && obj.tags.head().isCoexisting()) {
        obj.tags.erase_if([&](const Object::Tag& t) {
            return !Trace::isIndeterminatelySequenced(am, tag, t);
        });
        obj.tags.insert_head(tag);
        return;
    }
    for (const auto& item: obj.tags) {
        if (Trace::isIndeterminatelySequenced(am, tag, item)) {
            Formatter formatter{&am};
            throw UBException{{UB::refer_del_obj, UB::use_ptr_value_which_ref_del_obj, UB::unsequenced_access}, lib::format(
                    "Object `${name}` is unsequenced accessed(read/modify/delete/indeterminatelize)\n${}\n${}",
                    obj, formatter.tag(tag), formatter.tag(item))};
        }
    }
    obj.tags.clear();
    obj.tags.insert_head(tag);
}

void Trace::attachTag(AbstractMachine& am, Object& object, const Object::Tag& tag)
{
    const auto overlap = [](Object& a, Object& b) {
        auto l1 = a.address;
        auto r1 = l1 + a.effective_type.size();
        auto l2 = b.address;
        auto r2 = l2 + b.effective_type.size();
        return !(l1 >= r2 || r1 <= l2);
    };
    applyBottom(object.top(), [&](Object& o) {
        if (overlap(o, object)) {
            Trace::updateTag(am, o, tag);
        }
    });
}
