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

#include <logger.h>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <lib/utils.h>
#include <config.h>

using namespace cami;
const char* const Logger::color_table[] = {
        CAMI_LOG_COLOR_DEBUG,
        CAMI_LOG_COLOR_VERBOSE,
        CAMI_LOG_COLOR_INFO,
        CAMI_LOG_COLOR_WARNING,
        CAMI_LOG_COLOR_ERROR,
        CAMI_LOG_COLOR_FATAL,
};
const char* const Logger::level_table[] = {
        "[debug]",
        "[verbose]",
        "[info]",
        "[warning]",
        "[error]",
        "[fatal]",
};
Logger log::buffered{&std::cout}; // NOLINT
Logger log::unbuffered{&std::cerr, true, false, true}; // NOLINT
Logger log::file{ // NOLINT
#ifdef CAMI_ENABLE_LOG_FILE
        []() -> std::ostream* {
            auto log_file = new std::ofstream{CAMI_LOG_FILE_PATH};
            if (!log_file->is_open()) {
                std::fprintf(stderr, "cannot open log file '%s', use std::clog instead", CAMI_LOG_FILE_PATH);
                return &std::clog;
            }
            return log_file;
        }(),
#else
        &std::clog,
#endif
        false, true, true};

void Logger::do_print(Logger::Level level, std::string_view sv)
{
    if (this->colored_print) {
        (*this->os) << Logger::color_table[lib::log2(static_cast<int>(level))];
    }
    (*this->os) << Logger::level_table[lib::log2(static_cast<int>(level))];
    if (this->print_time) {
        this->printTime();
    }
    (*this->os) << sv;
    if (this->colored_print) {
        (*this->os) << "\033[0m";
    }
    if (this->auto_flush) {
        this->flush();
    }
}

void Logger::printTime() const
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm* ptm = std::localtime(&tt);
    (*this->os) << std::put_time(ptm, CAMI_LOG_TIME_FORMAT) << ": ";
}
