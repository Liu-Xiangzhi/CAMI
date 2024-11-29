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

#ifndef CAMI_FOUNDATION_LOGGER_H
#define CAMI_FOUNDATION_LOGGER_H

#include <config.h>
#include <lib/format.h>
#include <iostream>
#include <stdexcept>

namespace cami {

class Logger
{
public:
    enum class Level
    {
        fatal = 32, error = 16, warning = 8, info = 4, verbose = 2, debug = 1,
        mask_none = 0,
        mask_all = 63,
        mask_warning = 8 + 16 + 32,
        mask_error = 16 + 32,
    };
private:
    static const char* const color_table[6];
    static const char* const level_table[6];
public:
    std::ostream* os;
    bool colored_print;
    bool print_time;
    bool auto_flush;
    Level mask;
public:
    explicit Logger(std::ostream* os, bool colored_print = true, bool print_time = false, bool auto_flush = false,
                    Level mask = Level::mask_all)
            : os(os), colored_print(colored_print), print_time(print_time), auto_flush(auto_flush), mask(mask) {}

    ~Logger()
    {
        if (this->os != &std::cout && this->os != &std::cerr && this->os != &std::clog) {
            delete this->os;
            this->os = nullptr;
        }
    }

public:
    void flush() const
    {
        (*this->os) << std::flush;
    }

    void do_print(Level level, std::string_view sv);
    template<typename ...ARGS>
    void print(Level level, std::string_view format_string, ARGS&& ...args);

    template<typename ...ARGS>
    void println(Level level, std::string_view format_string, ARGS&& ...args)
    {
        this->print(level, format_string, std::forward<ARGS>(args)...);
        (*this->os) << '\n';
    }

    template<typename ...ARGS>
    void fprint(std::string_view format_string, ARGS&& ...args)
    {
        this->print(Level::fatal, format_string, std::forward<ARGS>(args)...);
    }

    template<typename ...ARGS>
    void eprint(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 1
        this->print(Level::error, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void wprint(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 2
        this->print(Level::warning, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void iprint(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 3
        this->print(Level::info, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void vprint(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 4
        this->print(Level::verbose, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void dprint(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 5
        this->print(Level::debug, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void pprint(std::string_view format_string, ARGS&& ...args)
    {
        (*this->os) << lib::format(format_string, std::forward<ARGS>(args)...);
    }

    template<typename ...ARGS>
    void fprintln(std::string_view format_string, ARGS&& ...args)
    {
        this->println(Level::fatal, format_string, std::forward<ARGS>(args)...);
    }

    template<typename ...ARGS>
    void eprintln(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 1
        this->println(Level::error, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void wprintln(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 2
        this->println(Level::warning, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void iprintln(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 3
        this->println(Level::info, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void vprintln(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 4
        this->println(Level::verbose, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void dprintln(std::string_view format_string, ARGS&& ...args)
    {
#if CAMI_LOG_LEVEL >= 5
        this->println(Level::debug, format_string, std::forward<ARGS>(args)...);
#endif
    }

    template<typename ...ARGS>
    void pprintln(std::string_view format_string, ARGS&& ...args)
    {
        (*this->os) << lib::format(format_string, std::forward<ARGS>(args)...) << '\n';
    }

private:
    void printTime() const;
};

inline int operator&(Logger::Level a, Logger::Level b)
{
    return static_cast<int>(a) & static_cast<int>(b);
}

inline int operator|(Logger::Level a, Logger::Level b)
{
    return static_cast<int>(a) | static_cast<int>(b);
}

inline int operator^(Logger::Level a, Logger::Level b)
{
    return static_cast<int>(a) ^ static_cast<int>(b);
}

template<typename... ARGS>
void Logger::print(Logger::Level level, std::string_view format_string, ARGS&& ... args)
{
    if (level == Level::fatal || (level & this->mask)) {
        this->do_print(level, lib::format(format_string, std::forward<ARGS>(args)...));
    }
    if (level == Level::fatal) {
        throw std::runtime_error{"fatal error happened"};
    }
}

namespace log {
extern Logger buffered;
extern Logger unbuffered;
extern Logger file;
}
} // namespace cami

#endif //CAMI_FOUNDATION_LOGGER_H
