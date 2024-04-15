from cfg import Config
from engine import Engine
from printer import Printer
import argument
import getopt
import os
import sys

def get_help_info(self_name:str):
    return f'Usage: {self_name} [-h|--help]' +\
           f'       {self_name} [-c <config_path>|--config=<config_path>]' +\
            '                   [-t <testsuit_dir>|--testsuite=<testsuit_dir>]' +\
            '                   [-o <out_dir>|--out=<out_dir>]'+\
            '                   [-v|--verbose]' +\
            '                   [--verbose-output-path=<verbose_output_path>]'

def parser_cli(argv: list[str]):
    opts, _ = getopt.getopt(argv[1:], 'hvc:t:o:', ['help', 'verbose', 'config=', 'testsuit=', 'out=', 'verbose-output-path='])
    for opt, arg in opts:
        match opt:
            case '-h' | '--help':
                print(get_help_info(argv[0]))
            case '-c' | '--config':
                argument.config_path = os.path.abspath(arg)
            case '-t' | '--testsuit':
                argument.test_suite_dir = os.path.abspath(arg)
            case '-o' | '--out':
                argument.out_dir = os.path.abspath(arg)
            case '-v' | '--verbose':
                argument.verbose = True
            case '--verbose-output-path':
                argument.verbose_output_path = os.path.abspath(arg)
            case _:
                raise ValueError(f'Unknown option: {opt}')

def main(argv: list[str]):
    parser_cli(argv)
    with open(argument.config_path, 'r') as f:
        config = Config.model_validate_json(f.read())
    result = Engine(config).run(argument.eval_root, argument.test_suite_dir, argument.out_dir)
    Printer(argument.test_suite_dir).print(result)


if __name__ == "__main__":
    main(sys.argv)
    exit(0)
