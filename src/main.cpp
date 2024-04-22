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
#include <args.h>

using namespace cami;

void cami_main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    try {
        cami_main(argc, argv);
    } catch (std::exception& e) {
        log::unbuffered.eprintln(e.what());
        return -1;
    }
}

void cami_main(int argc, char* argv[])
{
    auto argument = CommandLineParser{argc, argv}.parse();
    switch (argument.sub_command) {
    case Argument::SubCommand::none:
        return;
    case Argument::SubCommand::run:
        Launcher::launch(argument.run.file_name);
        return;
    case Argument::SubCommand::test_translation: {
        using namespace tr;
        std::string output_name = std::string{"deasm_"}.append(argument.test_translation.file_name);
        std::vector<std::unique_ptr<UnlinkedMBC>> mbcs;
        mbcs.push_back(argument.test_translation.file_name | read_file | assemble);
        Linker::link(std::move(mbcs), {MBC::Type::executable}) | deassemble | output_name;
    }
        return;
    default:
        throw std::runtime_error{lib::format("Unknown command enumeration value: '${}'", static_cast<int>(argument.sub_command))};
    }
}
