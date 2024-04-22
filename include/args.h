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

#ifndef CAMI_ARGS_H
#define CAMI_ARGS_H

#include <string>
#include <string_view>
#include <stdexcept>

namespace cami {

class CommandLineException : public std::runtime_error
{
public:
    explicit CommandLineException(std::string_view arg) : std::runtime_error(std::string{"command line argument error: "}.append(arg)) {}
};

struct Argument
{
    enum class SubCommand
    {
        run, test_translation, none
    } sub_command = SubCommand::none;
    union
    {
        struct
        {
            std::string_view file_name;
        } run;
        struct
        {
            std::string_view file_name;
        } test_translation;
    };

    Argument() {}
};

class CommandLineParser
{
    int argc;
    char** argv;
    int cur_arg = 1;
    Argument* result = nullptr;
public:
    CommandLineParser(int argc, char** argv) : argc(argc), argv(argv) {}

public:
    Argument parse();
    void parseSubCommand();
private:
    void help();
    void version();
    void run();
    void test_translation();
    void show();
    static void config();

    [[nodiscard]] bool hasNextArg() const noexcept
    {
        return this->cur_arg < this->argc;
    }

    std::string_view nextArg(std::string_view report_info = "too less argument")
    {
        if (this->cur_arg >= this->argc) {
            throw CommandLineException{report_info};
        }
        return {this->argv[this->cur_arg++]};
    }

    [[nodiscard]] std::string_view programName() const noexcept
    {
        return this->argc > 0 ? this->argv[0] : "cami";
    }
};

} // namespace cami

#endif //CAMI_ARGS_H
