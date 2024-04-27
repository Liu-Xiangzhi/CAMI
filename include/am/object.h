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

#ifndef CAMI_AM_OBJECT_H
#define CAMI_AM_OBJECT_H

#include <utility>
#include <set>
#include <functional>
#include <lib/array.h>
#include <lib/downcast.h>
#include <lib/optional.h>
#include <lib/list.h>
#include <foundation/type/def.h>
#include <lib/format.h>
#include "trace_data.h"

namespace cami::am {
class ObjectManager;

struct Entity
{
    std::string name;
    const ts::Type& effective_type;
    // only modified by linker/spawn
    // TODO: make `address` const again & distinguish function static_info and function instance(for each program)
    uint64_t address;
public:
    Entity(std::string name, const ts::Type& type, uint64_t address)
            : name(std::move(name)), effective_type(type), address(address) {}

    Entity(const Entity& that) = default;

    Entity(Entity&& that) noexcept
            : name(std::move(that.name)), effective_type(that.effective_type), address(that.address) {}

    Entity& operator=(Entity&&) = delete;

protected:
    DEBUG_VIRTUAL ~Entity() = default;
};

class Object : public Entity
{
public:
    enum class Status : uint8_t
    {
        well,
        destroyed,
        indeterminate,
        non_value_representation, uninitialized,
    };

    struct Tag
    {
        TraceContext& context;
        TraceLocation access_point;

        Tag(TraceContext& context, const TraceLocation& access_point) // NOLINT
                : context(context), access_point(access_point)
        {
            this->context.acquire();
        }

        Tag(const Tag& that) : context(that.context), access_point(that.access_point)
        {
            this->context.acquire();
        }

        ~Tag()
        {
            this->context.release();
        }

        [[nodiscard]] bool isCoexisting() const noexcept
        {
            return this->access_point.inner_id.isCoexisting();
        }
    };

public:
    Status status = Status::uninitialized;
    uint8_t age = 0; // used by ObjectManager only
    lib::List<Tag> tags;
    lib::Optional<Object*> super_object;
    lib::Array<Object*> sub_objects;
    std::set<Object*> referenced_by;
public:
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
private:
    friend class am::ObjectManager;

    Object(std::string name, const ts::Type& type, uint64_t address,
           lib::Optional<Object*> super_object, lib::Array<Object*> sub_objects)
            : Entity(std::move(name), type, address),
              super_object(super_object), sub_objects(std::move(sub_objects)) {}

    Object(Object&& that) noexcept : Entity(std::move(that.name), that.effective_type, that.address),
                                     status(that.status), age(that.age), tags(std::move(that.tags)),
                                     super_object(that.super_object), sub_objects(std::move(that.sub_objects)),
                                     referenced_by(std::move(that.referenced_by)) {}

public:
    [[nodiscard]] bool isIndeterminateRepresentation() const noexcept
    {
        return this->status >= Status::indeterminate;
    }

    Object& top() noexcept;
    Object& topOfSameAddress() noexcept;

    [[nodiscard]] const Object& top() const noexcept
    {
        return const_cast<Object*>(this)->top();
    }

    [[nodiscard]] const Object& topOfSameAddress() const noexcept
    {
        return const_cast<Object*>(this)->topOfSameAddress();
    }

    [[nodiscard]] uint64_t size() const noexcept
    {
        return this->effective_type.size();
    }
};

void applyRecursively(Object& object, const std::function<void(Object&)>& func);
void applyBottom(Object& object, const std::function<void(Object&)>& func);
bool checkStatusForRead(Object& object);
void copyStatus(Object& from, Object& to);
void updateCommonInitialSequenceStatus(Object& object);

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::Entity);

CAMI_DECLARE_FORMATTER(cami::am::Object);

CAMI_DECLARE_FORMATTER(cami::am::Object::Status);

#endif //CAMI_AM_OBJECT_H
