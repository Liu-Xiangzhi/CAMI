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

#ifndef CAMI_AM_OBJ_MAN_H
#define CAMI_AM_OBJ_MAN_H

#include <config.h>
#include "object.h"
#include <lib/utils.h>
#include <queue>
#include <map>
#include <string>
#include <cstring>
#include <lib/format.h>

namespace cami::am {
class AbstractMachine;
namespace detail {
struct alignas(alignof(Object)) FakeObject
{
    [[maybe_unused]] char _[sizeof(Object)];
};

} // namespace detail

/* In normal state:
 *   alloc -- small --> alloc eden --> -- enough --> finish
 *         |                           +- not enough --> minor gc --> -- success --> finish
 *         |                                                          +- failed --> treat as large & retry
 *         +- large --> alloc old generation --> -- enough --> finish
 *                                               +- not enough --> major gc --> -- enough --> finish
 *                                                                              +- not enough --> OOM
 *
 * minor gc -- survivor space enough, no promotion  --> do nothing extra
 *          |- survivor space not enough            --> -- old_gen enough      --> move to old_gen
 *          |                                           +- old_gen not enough  --> major gc --> -- enough --> move to old_gen
 *          |                                                                                   +- not enough --> FAILED
 *          +- survivor space enough, promotion     --> -- old_gen enough     --> do promotion
 *                                                      +- old_gen not enough --> major gc --> -- enough --> do promotion
 *                                                                                             +- not enough --> reject promotion
 *
 * major gc --> mark and rearrange
 */

using namespace lib::literals;

class ObjectManager
{
    static constexpr uint64_t EDEN_SIZE = CAMI_OBJECT_MANAGE_EDEN_SIZE;
    static constexpr uint64_t SURVIVOR_SIZE = EDEN_SIZE / 8;
    static constexpr uint64_t OLD_GENERATION_SIZE = CAMI_OBJECT_MANAGE_OLD_GENERATION_SIZE;
    static constexpr uint64_t LARGE_OBJ_THRESHOLD = CAMI_OBJECT_MANAGE_LARGE_OBJECT_THRESHOLD / sizeof(Object);
    static constexpr uint64_t PROMOTE_THRESHOLD = CAMI_OBJECT_MANAGE_PROMOTE_THRESHOLD;
public:
    class Page
    {
        detail::FakeObject* storage;
        uint8_t* bitmap;
    public:
        const uint64_t max_size;
        uint64_t usage = 0;

    public:
        explicit Page(uint64_t max_size, bool use_bitmap = true)
                : storage(new detail::FakeObject[max_size]), max_size(max_size),
                  bitmap(use_bitmap ? new uint8_t[lib::roundUpDiv(max_size, 8)] : nullptr) {}

        Page(const Page&) = delete;
        Page(Page&&) noexcept = delete;
        Page& operator=(const Page&) = delete;
        Page& operator=(Page&&) noexcept = delete;

        ~Page()
        {
            delete[] this->storage;
            delete[] this->bitmap;
        }

        Object& operator[](uint64_t idx)
        {
            using namespace std::string_literals;
            ASSERT(idx < this->max_size, lib::format("index out of boundary. idx: ${}, len: ${}", idx, this->max_size));
            return reinterpret_cast<Object&>(this->storage[idx]);
        }

        Object* data()
        {
            return reinterpret_cast<Object*>(this->storage);
        }

        void resetBitmap()
        {
            std::memset(this->bitmap, 0, lib::roundUpDiv(this->max_size, 8));
        }

        bool testBitmap(Object* object)
        {
            return testBitmap(this->getIndex(reinterpret_cast<detail::FakeObject*>(object)));
        }

        void setBitmap(Object* object)
        {
            setBitmap(this->getIndex(reinterpret_cast<detail::FakeObject*>(object)));
        }

        void unsetBitmap(Object* object)
        {
            unsetBitmap(this->getIndex(reinterpret_cast<detail::FakeObject*>(object)));
        }

        bool testBitmap(uint64_t idx)
        {
            ASSERT(idx < this->max_size, "index out of boundary");
            return this->bitmap[idx / 8] & (1 << (idx % 8));
        }

        void setBitmap(uint64_t idx)
        {
            ASSERT(idx < this->max_size, "index out of boundary");
            this->bitmap[idx / 8] |= 1 << (idx % 8);
        }

        void unsetBitmap(uint64_t idx)
        {
            ASSERT(idx < this->max_size, "index out of boundary");
            this->bitmap[idx / 8] &= ~(1 << (idx % 8));
        }

    private:
        uint64_t getIndex(detail::FakeObject* object)
        {
            ASSERT(object >= this->storage && object < this->storage + this->max_size,
                   "cannot get index of object belonging to another page");
            return object - this->storage;
        }
    };

private:
    AbstractMachine& am;
    struct State
    {
        int survivor_idx = 0;
        enum
        {
            eden, old_generation, permanent
        } alloc = eden;

        struct
        {
            bool majored = false; // true if major gc is performed
            std::deque<Object*> root_reachable{};

            void reset()
            {
                this->majored = false;
                this->root_reachable.clear();
            }
        } gc;
    } state;
    Page eden{EDEN_SIZE / sizeof(Object)};
    Page survivor[2]{Page{SURVIVOR_SIZE / sizeof(Object)},
                     Page{SURVIVOR_SIZE / sizeof(Object)}};
    Page old_generation{OLD_GENERATION_SIZE / sizeof(Object)};
    Page permanent;
public:
    friend class lib::ToString<ObjectManager>;

    explicit ObjectManager(AbstractMachine& am, uint64_t permanent_obj_num)
            : am(am), permanent(permanent_obj_num) {}

    ~ObjectManager();

public:
    Object* new_(std::string name, const ts::Type& type, uint64_t address);
    Object* newPermanent(std::string name, const ts::Type& type, uint64_t address);
    // cleanup will NOT dealloc memory
    void cleanup(Object* object);

    void forceGC()
    {
        this->state.gc.reset();
        this->majorGC();
        this->minorGC();
    }

    lib::Optional<Object*> getReferencedObject(const Object* obj) const;

    [[nodiscard]] const Page& getEden() const noexcept
    {
        return this->eden;
    }

    [[nodiscard]] const Page& getSurvivor() const noexcept
    {
        return this->survivor[this->state.survivor_idx];
    }

    [[nodiscard]] const Page& getOldGeneration() const noexcept
    {
        return this->old_generation;
    }

    [[nodiscard]] const Page& getPermanent() const noexcept
    {
        return this->permanent;
    }

private:
    Object* newSmall(std::string name, const ts::Type& type, uint64_t address);
    Object* newLarge(std::string name, const ts::Type& type, uint64_t address);
    Object* createObject(const ts::Type& type, uint64_t address);
    lib::Array<Object*> createSubObject(const ts::Type& type, uint64_t address);
    Object* allocOneObject();
    bool minorGC();
    void majorGC();
    void minorGC_mark();
    std::pair<uint64_t, uint64_t> minorGC_statistic();
    bool minorGC_arrange(uint64_t total_survivor_cnt, uint64_t promote_cnt);
    void markRootReachable();
    void markReachable(Object* object);
    bool isMarked(Object* object);
    void topdownSearchMark(std::deque<Object*>& root);
    void crossGenerationSearchMark(Page& page);
    bool backwardsReachableTest(Object* object); // backwards search, used by minor GC
    // YG means young generation
    void arrangeYGToSurvivor(Page& page, std::map<Object*, Object*>& mapper, bool promote);
    void arrangeYGToOldGeneration(Page& page, std::map<Object*, Object*>& mapper);
    std::map<Object*, Object*> shrinkOldGeneration();
    void moveObject(Object* src, Object* dest, std::map<Object*, Object*>& mapper);
    void innerObjectRefRelocate(Object* obj, Object* origin);
    void amRefRelocate(const std::map<Object*, Object*>& mapper);
    static void checkMemoryLeak(Object* object);
    bool belongToEden(Object* obj);
    bool belongToSurvivor(Object* obj);
    bool belongToOldGeneration(Object* obj);
    bool belongToPermanent(Object* obj);
    static bool belongTo(Object* obj, Page& page);

    Page& currentSurvivor()
    {
        return this->survivor[this->state.survivor_idx];
    };
};

} // namespace cami::am

CAMI_DECLARE_FORMATTER(cami::am::ObjectManager);

#endif //CAMI_AM_OBJ_MAN_H
