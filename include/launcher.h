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

#ifndef CAMI_LAUNCHER_H
#define CAMI_LAUNCHER_H

#include <config.h>
#include <string>
#include <memory>
#include <string_view>
#include <translate/bytecode.h>

namespace cami {

enum class FileType
{
    text, binary, detect
};

class Launcher
{
public:
    static void launch(std::string_view file_name, FileType file_type = FileType::detect);
private:
    static std::unique_ptr<tr::MBC> loadFile(std::string_view file_name, bool text_file);
    static std::unique_ptr<tr::MBC> linkFile(std::unique_ptr<tr::MBC> mbc);
    static FileType detectFileType(std::string_view file_name);
};

} // namespace cami

#endif //CAMI_LAUNCHER_H
