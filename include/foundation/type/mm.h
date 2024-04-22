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

#ifndef CAMI_FOUNDATION_TYPE_MM_H
#define CAMI_FOUNDATION_TYPE_MM_H

#include <unordered_set>
#include <vector>
#include <map>
#include "def.h"

namespace cami::ts {
namespace detail {
struct TypeHash
{
    size_t operator()(const Type* type) const noexcept
    {
        return type->hash();
    }
};

struct TypeEqual
{
    bool operator()(const Type* lhs, const Type* rhs) const noexcept
    {
        return lhs->equals(*rhs);
    }
};

extern const size_t page_size;

class TrivialDeconstructedPool
{
    std::vector<uint8_t*> pool;
    size_t usage = page_size;
    size_t prev_usage = 0;
public:
    ~TrivialDeconstructedPool();

public:
    uint8_t* alloc(size_t size, size_t align);
    void dealloc();

private:
    void new_page();
    [[nodiscard]] uint8_t* getCurrentPage() const noexcept;
};

class TypePoolBase
{
protected:
    struct PageInfo
    {
        uint8_t* addr;
        size_t usage;
    };
    std::vector<PageInfo> pool;
    size_t usage = page_size;
public:

    uint8_t* alloc(size_t size);

private:
    void new_page();
protected:
    [[nodiscard]] uint8_t* getCurrentPage() const noexcept;
};

template<typename T>
class TypePool : public TypePoolBase
{
public:
    ~TypePool();
    template<typename ...ARGS>
    T* alloc(ARGS&& ... args);
    void dealloc();
};

} // namespace detail


class MemoryManager
{
    std::unordered_set<Type*, detail::TypeHash, detail::TypeEqual> index;
    std::map<std::string_view, Struct*> struct_mapper;
    std::map<std::string_view, Union*> union_mapper;
    detail::TrivialDeconstructedPool trivial_deconstructed_area; // basic pointer array qualified
    detail::TypePool<Function> function_area;
    detail::TypePool<Struct> struct_area;
    detail::TypePool<Union> union_area;
    Basic* basic_area; // subarea of trivial_deconstructed_area, used for speeding up `getBasicType`
public:
    MemoryManager();
public:
    const Basic& getInvalid();
    const Basic& getDissociativePointer();
    const Basic& getBasicType(Kind kind);
    const Pointer& getPointer(const Type& referenced);
    const Array& getArray(const Type& element, size_t len);
    const Function& getFunction(const Type& returned, lib::Array<const Type*> params);
    const Qualify& getQualifiedType(const Type& qualified, Qualifier qualifier);
    void createStruct(std::string name, lib::Array<const Type*> members);
    void createUnion(std::string name, lib::Array<const Type*> members);
    const Struct& declareStruct(std::string name);
    const Union& declareUnion(std::string name);
private:
    template<typename>
    friend class detail::TypePool;

    template<class T>
    static void delete_(T* ptr)
    {
        ptr->~T();
    }

    template<class T, typename ...ARGS>
    static void new_(T* ptr, ARGS&& ...args)
    {
        new(ptr) T{std::forward<ARGS>(args)...};
    }
};

extern MemoryManager type_manager;
} // namespace cami::ts

#endif //CAMI_FOUNDATION_TYPE_MM_H
