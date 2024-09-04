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

#include <args.h>
#include <iostream>
#include <map>
#include <lib/format.h>
#include <lib/utils.h>
#include <config.h>
#include <am/ub.h>

#define VERSION "0.2.1"

using namespace cami;
namespace {
constexpr const char* logo_lines[] = {
        "\033[31m     ,gggg,  \033[32m           ,ggg,  \033[33m ,ggg, ,ggg,_,ggg,  \033[34m      ,a8a, ",
        "\033[31m   ,88\"\"\"Y8b,\033[32m          dP\"\"8I  \033[33mdP\"\"Y8dP\"\"Y88P\"\"Y8b \033[34m     ,8\" \"8,",
        "\033[31m  d8\"     `Y8\033[32m         dP   88  \033[33mYb, `88'  `88'  `88 \033[34m     d8   8b",
        "\033[31m d8'   8b  d8\033[32m        dP    88  \033[33m `\"  88    88    88 \033[34m     88   88",
        "\033[31m,8I    \"Y88P'\033[32m       ,8'    88  \033[33m     88    88    88 \033[34m     88   88",
        "\033[31mI8'          \033[32m       d88888888  \033[33m     88    88    88 \033[34m     Y8   8P",
        "\033[31md8           \033[32m __   ,8\"     88  \033[33m     88    88    88 \033[34m     `8, ,8'",
        "\033[31mY8,          \033[32mdP\"  ,8P      Y8  \033[33m     88    88    88 \033[34m8888  \"8,8\" ",
        "\033[31m`Yba,,_____, \033[32mYb,_,dP       `8b,\033[33m     88    88    Y8,\033[34m`8b,  ,d8b, ",
        "\033[31m  `\"Y8888888 \033[32m \"Y8P\"         `Y8\033[33m     88    88    `Y8\033[34m  \"Y88P\" \"Y8",
};
constexpr const char* banner_info_lines[] = {
        " |C Abstract Machine Interpreter",
        " |______________________________________________________________________",
        (" |  version: " VERSION),
        " |  github:  www.github.com/Liu-Xiangzhi/CAMI",
        " |  license: GPL v2 or later version",
        " |  we have:",
        " |  * more reasonable operational semantic that conforming to C standard",
        " |  * \033[32mSOUND & COMPLETE UB detection ability\033[0m(under our semantic)",
        " |  * inefficient execution but trying to speed up!",
        " |",
};
} // anonymous namespace

Argument CommandLineParser::parse()
{
    Argument argument;
    this->result = &argument;
    this->parseSubCommand();
    return argument;
}

void CommandLineParser::parseSubCommand()
{
    static const std::map<std::string_view, void (CommandLineParser::*)()> handler{
            {"help",             &CommandLineParser::help},
            {"-h",               &CommandLineParser::help},
            {"--help",           &CommandLineParser::help},
            {"version",          &CommandLineParser::version},
            {"-v",               &CommandLineParser::version},
            {"--version",        &CommandLineParser::version},
            {"run",              &CommandLineParser::run},
            {"test_translation", &CommandLineParser::test_translation},
            {"show",             &CommandLineParser::show},
    };
    auto sub_command = this->nextArg("missing sub-command");
    auto itr = handler.find(sub_command);
    if (itr == handler.end()) {
        throw CommandLineException{lib::format("Unknown sub command `${}`. Type \"${} help\" for more help", sub_command, this->argv[0])};
    }
    (this->*itr->second)();
    if (this->hasNextArg()) {
        std::cerr << "Too much arguments. Extra arguments ignored." << std::endl;
    }
}

void CommandLineParser::help()
{
    if (!this->hasNextArg()) {
        std::cout << lib::format(R"(${} <sub_command> [arguments]...
    type ${} help <sub_command> for  help info of <sub_command>
sub-commands:
    run                  launch abstract machine
    test_translation     (temporary sub-command, will be replaced by 'assemble' and 'deassemble')
                         do assemble => link => deassemble to test translate module.
    help                 print this help info
    show                 show config, version,..., etc. of CAMI
    -h                   alias of 'help'
    --help               alias of 'help'
    -v                   alias of 'show version'
    --version            alias of 'show version'
see https://www.github.com/Liu-Xiangzhi/CAMI for github repository
)", this->programName(), this->programName());
        return;
    }
    auto sub_command = this->nextArg();
    if (sub_command == "run") {
        std::cout << R"(cami run <bytecode_path>
    load bytecode and launch abstract machine.
    <bytecode_path> can be both text form or binary form(not supported now), and can be object file
    or linked file. if <bytecode_path> is object file, abstract machine launcher will automatically
    find those dependent files, recursively, and link then together. Abstract machine actually only
    accept linked bytecode. Options of abstract machine is hard-coded now in order to facilitate
    implementation, and it will be runtime-configurable in next few versions.
)";
    } else if (sub_command == "test_translation") {
        std::cout << R"(cami test_translation <bytecode_path>
    read text form bytecode, assemble it, link and deassemble.
    this sub-command is to be removed.
)";
    } else if (sub_command == "show") {
        std::cout << R"(cami show (version|banner|logo|config|ub [ub_number])
    print version, banner, logo or config info of CAMI or list UB(s).
    banner is a mixture of logo, version and some other info.
    config tells configure items in `config.toml`(see ReadMe.md in repository).
    ub_number should be in range [1, 218], all UBs will be listed if ub_number is omitted.
)";
    } else {
        throw CommandLineException{lib::format("no help info for sub-command `${}`", sub_command)};
    }
}

void CommandLineParser::version() // NOLINT
{
    std::cout << "CAMI version" VERSION "\n"
                 R"(Copyright (c) 2024. Liu Xiangzhi
CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
)";
}

void CommandLineParser::run()
{
    this->result->sub_command = Argument::SubCommand::run;
    this->result->run.file_name = this->nextArg("missing bytecode path");
}

void CommandLineParser::test_translation()
{
    this->result->sub_command = Argument::SubCommand::test_translation;
    this->result->test_translation.file_name = this->nextArg("missing bytecode path");
}

void CommandLineParser::show()
{
    auto show_object = this->nextArg("missing object of `show` sub-command");
    if (show_object == "version") {
        this->version();
    } else if (show_object == "banner") {
        std::cout << '\n';
        for (int i = 0; i < 10; ++i) {
            std::cout << ::logo_lines[i] << "\033[0m" << ::banner_info_lines[i] << std::endl;
        }
        std::cout << '\n';
    } else if (show_object == "logo") {
        for (auto item: ::logo_lines) {
            std::cout << item << std::endl;
        }
    } else if (show_object == "config") {
        CommandLineParser::config();
    } else if (show_object == "ub") {
        if (this->hasNextArg()) {
            int ub_no;
            try {
                ub_no = std::stoi(std::string{this->nextArg()});
            } catch (const std::exception& e) {
                throw CommandLineException{e.what()};
            }
            if (ub_no > 218 || ub_no < 1) {
                throw CommandLineException{"invalid UB number, should in range [1, 218]"};
            }
            std::cout << lib::format("${}\n", am::UB{ub_no});
        } else {
            for (int i = 1; i <= 218; ++i) {
                std::cout << lib::format("${}\n", am::UB{i});
            }
        }
    } else {
        throw CommandLineException{lib::format("Unknown show object `${}`", show_object)};
    }
}

void CommandLineParser::config()
{
#define STR_(...) "" #__VA_ARGS__
#define STR(...) STR_(__VA_ARGS__)
#define DEFINED(cami_macro) (sizeof STR(cami_macro) == 1)
    const auto readable = [](uint64_t value) -> std::string {
        const char unit[] = {' ', 'K', 'M', 'G', 'T', 'P', 'E'};
        int i;
        for (i = 0; i < 7; ++i) {
            if (value < 1024) {
                break;
            }
            value /= 1024;
        }
        return std::to_string(value) + unit[i];
    };
    using namespace lib::literals;
    std::cout << std::boolalpha
              << "disable_compiler_guarantee_check: " << DEFINED(CAMI_DISABLE_COMPILER_GUARANTEE_CHECK) << '\n'
              << "enable_auto_cache: " << DEFINED(CAMI_ENABLE_AUTO_CACHE) << '\n'
              << "cache_path: " << CAMI_CACHE_PATH << '\n'
              << "enable_log_file: " << DEFINED(CAMI_ENABLE_LOG_FILE) << '\n'
              << "log_file_path: " << CAMI_LOG_FILE_PATH << '\n'
              << "log.time_format: " << CAMI_LOG_TIME_FORMAT << '\n'
              << "log.level: " << CAMI_LOG_LEVEL << '\n'
              << "log.color.fatal: " << CAMI_LOG_COLOR_FATAL  "this color\033[0m\n"
              << "log.color.error: " << CAMI_LOG_COLOR_ERROR "this color\033[0m\n"
              << "log.color.warning: " << CAMI_LOG_COLOR_WARNING  "this color\033[0m\n"
              << "log.color.info: " << CAMI_LOG_COLOR_INFO  "this color\033[0m\n"
              << "log.color.verbose: " << CAMI_LOG_COLOR_VERBOSE  "this color\033[0m\n"
              << "log.color.debug: " << CAMI_LOG_COLOR_DEBUG  "this color\033[0m\n"
              << "object_manage.eden_size: " << readable(CAMI_OBJECT_MANAGE_EDEN_SIZE) << '\n'
              << "object_manage.old_generation_size: " << readable(CAMI_OBJECT_MANAGE_OLD_GENERATION_SIZE) << '\n'
              << "object_manage.large_object_threshold: " << readable(CAMI_OBJECT_MANAGE_LARGE_OBJECT_THRESHOLD) << '\n'
              << "object_manage.promote_threshold: " << readable(CAMI_OBJECT_MANAGE_PROMOTE_THRESHOLD) << '\n'
              << "memory.heap.page_size: " << readable(CAMI_MEMORY_HEAP_PAGE_SIZE) << '\n'
              << "memory.heap.page_table_level: " << readable(CAMI_MEMORY_HEAP_PAGE_TABLE_LEVEL) << '\n'
              << "memory.heap.allocator: " << STR(CAMI_MEMORY_HEAP_ALLOCATOR) << '\n'
              << "memory.mmio.max_file: " << readable(CAMI_MEMORY_MMIO_MAX_FILE) << '\n'
              << "file_system.root: " << CAMI_FILE_SYSTEM_ROOT << std::endl;
#undef DEFINED
#undef STR
#undef STR_
}
