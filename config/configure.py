#  Copyright (c) 2024. Liu Xiangzhi
#  This file is part of CAMI.
#
#  CAMI is free software: you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software Foundation,
#  either version 2 of the License, or any later version.
#
#  CAMI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
#  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with CAMI.
#  If not, see <https://www.gnu.org/licenses/>.
import tomllib
from os import path
import re

root = path.dirname(path.dirname(__file__))
replace_dict = {
    "black": "\"\\033[30m\"",
    "red": "\"\\033[31m\"",
    "green": "\"\\033[32m\"",
    "yellow": "\"\\033[33m\"",
    "blue": "\"\\033[34m\"",
    "magenta": "\"\\033[35m\"",
    "cyan": "\"\\033[36m\"",
    "white": "\"\\033[37m\"",
    "log_level_fatal": "0",
    "log_level_error": "1",
    "log_level_warning": "2",
    "log_level_info": "3",
    "log_level_verbose": "4",
    "log_level_debug": "5",
}


class Generator:
    def __init__(self, config):
        self.config = config
        self._tmp_str_array = []
        self._results = []

    def generate(self):
        self.__dfs(self.config, self.__generate_value)
        return '\n'.join(self._results)

    def __generate_value(self, value):
        def_name = '_'.join(self._tmp_str_array)
        if isinstance(value, bool):
            if value:
                self._results.append(f'#define {def_name}')
            return
        def_value = ''
        if isinstance(value, str):
            add_quote = False
            if len(value) < 3:
                if len(value) == 1:
                    if value[0] == '#':
                        add_quote = True
                        value = ''
                elif len(value) == 2:
                    if value == '##':
                        value = '#'
                    elif value[0] == '#':
                        add_quote = True
                        value = value[1:]
            elif value[0] == '#':
                if value[1] != '#':
                    add_quote = True
                    value = value[1:]
                elif value[2] != '#':
                    value = value[1:]
                else:
                    add_quote = True
                    value = value[2:]
            def_value = re.sub(r'@(\w+)', lambda s: replace_dict[s.group(1)], value)
            if add_quote:
                def_value = f'"{def_value}"'
        else:
            def_value = str(value)
        self._results.append(f'#define {def_name} {def_value}')

    def __dfs(self, config_values, f):
        for key, value in config_values.items():
            self._tmp_str_array.append(key.upper())
            if isinstance(value, dict):
                self.__dfs(value, f)
            else:
                f(value)
            self._tmp_str_array.pop()


def main(output_header_file) -> None:
    with open(path.join(root, "config/config.toml"), 'rb') as f:
        config = tomllib.load(f)
        cpp_header_config = Generator(config).generate()
    with open(output_header_file, 'r') as f:
        result_header = f.read().replace('@def@', cpp_header_config)
    with open(path.splitext(output_header_file)[0], 'w') as f:
        f.write(result_header)


if __name__ == "__main__":
    main(path.join(root, "include/config.h.in"))
    exit(0)
