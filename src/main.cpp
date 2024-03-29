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

#include <string>
#include <string_view>
#include <foundation/logger.h>
#include <translate/pipe.h>
#include <translate/linker.h>
#include <launcher.h>

using namespace cami;

int cami_main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    try {
        return cami_main(argc, argv);
    } catch (std::exception& e) {
        log::unbuffered.eprintln(e.what());
    }
    return -1;
}

int cami_main(int argc, char* argv[])
{
    if (argc < 3) {
        log::unbuffered.eprintln("invalid argument number.\nUsage: cami (run|test_translate) <file_name>");
        return 1;
    }
    std::string_view command = argv[1];
    std::string_view file_name = argv[2];
    if (command == "run"sv) {
        Launcher::launch(file_name);
    } else if (command == "test_translate"sv) {
        using namespace tr;
        std::string output_name = std::string{"deasm_"}.append(file_name);
        auto mbc = file_name | read_file | assemble;
        Linker::link({mbc.get()}, {MBC::Type::fix_address_executable}) | deassemble | output_name;
    } else {
        log::unbuffered.eprintln("Unknown command '${}'", command);
        return 1;
    }
    return 0;
}
