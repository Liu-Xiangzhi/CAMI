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

#ifndef CAMI_TRANSLATE_PIPE_H
#define CAMI_TRANSLATE_PIPE_H

#include <fstream>
#include <foundation/exception.h>
#include "assembler.h"
#include "deassembler.h"

namespace cami::tr {
namespace detail {
struct ReaderTag
{
};
struct AssemblerTag
{
};
struct DeAssemblerTag
{
};
struct TBC
{
    std::string text;
    std::string_view name;
};
struct MBCToBeDeAsm
{
    const MBC& mbc;
};

} // namespace detail
constexpr detail::ReaderTag read_file;
constexpr detail::AssemblerTag assemble;
constexpr detail::DeAssemblerTag deassemble;

inline detail::TBC operator|(std::string_view file_name, detail::ReaderTag)
{
    std::ifstream input{file_name.data()};
    if (!input.is_open()) {
        throw FileCannotOpenException{file_name};
    }
    std::string text{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    return {std::move(text), file_name};
}

inline detail::TBC operator|(std::istream& is, detail::ReaderTag)
{
    std::string text{std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{}};
    return {std::move(text), "-"};
}

inline std::unique_ptr<MBC> operator|(const detail::TBC& input, detail::AssemblerTag)
{
    return Assembler{}.assemble(input.text, input.name);
}

inline detail::MBCToBeDeAsm operator|(const std::unique_ptr<MBC>& mbc, detail::DeAssemblerTag)
{
    return {*mbc};
}

inline detail::MBCToBeDeAsm operator|(const MBC& mbc, detail::DeAssemblerTag)
{
    return {mbc};
}

inline void operator|(detail::MBCToBeDeAsm mbc, std::string_view file_name)
{
    std::ofstream output{file_name.data()};
    if (!output.is_open()) {
        throw FileCannotOpenException{file_name};
    }
    DeAssembler::deassemble(mbc.mbc, output);
}

inline void operator|(detail::MBCToBeDeAsm mbc, std::ostream& output)
{
    DeAssembler::deassemble(mbc.mbc, output);
}

} // namespace cami::tr
#endif //CAMI_TRANSLATE_PIPE_H
