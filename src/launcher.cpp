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

#include <launcher.h>
#include <string_view>
#include <queue>
#include <am/am.h>
#include <foundation/exception.h>
#include <translate/pipe.h>
#include <translate/linker.h>

using namespace cami;
using std::operator ""sv;
using namespace tr;

namespace {
bool endWith(std::string_view sv, std::string_view suffix)
{
    if (sv.length() < suffix.length()) {
        return false;
    }
    auto d1 = sv.data() + sv.length() - 1;
    auto d2 = suffix.data() + suffix.length() - 1;
    while (d2 != suffix.data()) {
        if (*d1-- != *d2--) {
            return false;
        }
    }
    return true;
}
}

void Launcher::launch(std::string_view file_name, FileType file_type)
{
    if (file_type == FileType::detect) {
        file_type = Launcher::detectFileType(file_name);
    }
    auto mbc = Launcher::loadFile(file_name, file_type == FileType::text);
    if (mbc->attribute.type == MBC::Type::shared_object) {
        throw std::runtime_error{"bytecode of `shared_object` type is not supported yet"};
    }
    if (mbc->attribute.type == MBC::Type::object_file) {
        mbc = Launcher::linkFile(down_cast<std::unique_ptr<UnlinkedMBC>>(std::move(mbc)));
    }
    am::AbstractMachine abstract_machine{down_cast<std::unique_ptr<LinkedMBC>>(std::move(mbc))};
    abstract_machine.run();
}

FileType Launcher::detectFileType(std::string_view file_name)
{
    if (endWith(file_name, ".tbc"sv)) {
        return FileType::text;
    } else if (!endWith(file_name, ".cbc"sv)) {
        throw CannotDetectFileTypeException{file_name};
    }
    return FileType::binary;
}

std::unique_ptr<tr::MBC> Launcher::loadFile(std::string_view file_name, bool text_file)
{
    if (text_file) {
        using tr::operator|;
        return file_name | read_file | assemble;
    } else {
        // binary file
        throw std::runtime_error{"launch binary bytecode is not supported yet"};
    }
}

std::unique_ptr<LinkedMBC> Launcher::linkFile(std::unique_ptr<tr::UnlinkedMBC> mbc)
{
    std::queue<std::string> queue;
    for (const auto& item: mbc->attribute.static_links) {
        queue.push(item);
    }
    std::map<std::string, std::unique_ptr<tr::UnlinkedMBC>> loaded_files{};
    loaded_files.emplace(mbc->source_name, std::move(mbc));
    while (!queue.empty()) {
        auto file_name = queue.front();
        queue.pop();
        if (loaded_files.find(file_name) != loaded_files.end()) {
            continue;
        }
        auto bc = Launcher::loadFile(file_name, Launcher::detectFileType(file_name) == FileType::text);
        for (const auto& item: bc->attribute.static_links) {
            queue.push(item);
        }
        if (bc->attribute.type != MBC::Type::object_file) {
            throw std::runtime_error{"Cannot static link already linked bytecode: " + file_name};
        }
        loaded_files.emplace(file_name, down_cast<std::unique_ptr<UnlinkedMBC>>(std::move(bc)));
    }
    std::vector<std::unique_ptr<UnlinkedMBC>> mbcs(loaded_files.size());
    uint64_t cnt = 0;
    for (auto& item: loaded_files) {
        mbcs[cnt++] = std::move(item.second);
    }
    auto linked_mbc = Linker::link(std::move(mbcs), {MBC::Type::executable});
    return down_cast<std::unique_ptr<LinkedMBC>>(std::move(linked_mbc));
}
