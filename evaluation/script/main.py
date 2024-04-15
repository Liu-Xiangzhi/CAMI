from cfg import Config
from engine import Engine
from printer import Printer
import argument
import getopt
import os
import sys

def get_help_info(self_name:str):
    return f'Usage: {self_name} [-h|--help]\n' +\
           f'       {self_name} [-d|--show-default]\n'+\
           f'       {self_name} [-c <config_path>|--config=<config_path>]\n' +\
            '                   [-t <testsuit_dir>|--testsuite=<testsuit_dir>]\n' +\
            '                   [-o <out_dir>|--out=<out_dir>]\n'+\
            '                   [-v|--verbose]\n' +\
            '                   [-j <jobs_cnt>|--parallel <jobs_cnt>]\n' +\
            '                   [--verbose-output-path=<verbose_output_path>]'+\
            '                   [-r|--retain-output]'

def parser_cli(argv: list[str]):
    try:
        opts, _ = getopt.getopt(argv[1:], 'hdvrc:t:o:j:', ['help', 'show-default', 'retain-output', 'verbose', 'config=', 'testsuit=', 'out=', 'verbose-output-path=', 'parallel='])
    except getopt.GetoptError as e:
        print(e.msg)
        exit(-1)
    for opt, arg in opts:
        match opt:
            case '-h' | '--help':
                print(get_help_info(argv[0]))
                exit(0)
            case '-d' | '--show-default':
                print(f'eval_root:\t\t{argument.eval_root}')
                print(f'test_suite_dir:\t\t{argument.test_suite_dir}')
                print(f'out_dir:\t\t{argument.out_dir}')
                print(f'config_path:\t\t{argument.config_path}')
                print(f'verbose:\t\t{argument.verbose}')
                print(f'verbose_output_path:\t{argument.verbose_output_path}')
                print(f'parallel:\t\t{argument.parallel}')
                print(f'auto_clean:\t\t{argument.auto_clean}')
                exit(0)
            case '-r' | '--retain-output':
                argument.auto_clean = False
            case '-c' | '--config':
                argument.config_path = os.path.abspath(arg)
            case '-t' | '--testsuit':
                argument.test_suite_dir = os.path.abspath(arg)
            case '-o' | '--out':
                argument.out_dir = os.path.abspath(arg)
            case '-v' | '--verbose':
                argument.verbose = True
            case '-j' | '--parallel':
                argument.parallel = int(arg)
            case '--verbose-output-path':
                argument.verbose_output_path = os.path.abspath(arg)
            case _:
                raise ValueError(f'Unknown option: {opt}')

def cleanup_binary():
    import shutil
    for entry in os.listdir(argument.out_dir):
        path = os.path.join(argument.out_dir, entry)
        if os.path.isdir(path) and entry != 'CAMI':
            shutil.rmtree(path)

def main(argv: list[str]):
    parser_cli(argv)
    with open(argument.config_path, 'r') as f:
        config = Config.model_validate_json(f.read())
    import time
    start = time.time()
    result = Engine(config).run(argument.eval_root, argument.test_suite_dir, argument.out_dir)
    end = time.time()
    print(f'processing cost time: {end - start:.5f}')
    Printer(argument.test_suite_dir).print(result)
    if argument.auto_clean:
        cleanup_binary()


if __name__ == "__main__":
    main(sys.argv)
    exit(0)
