from cfg import Config
from engine import Engine
from printer import Printer
import argument
import getopt
import os
import sys


def get_help_info(self_name:str):
    return f'''Usage: {self_name} [-h|--help]
       {self_name} [-d|--show-default]
       {self_name} [-c <config_path>|--config=<config_path>]
                   [-t <testsuit_dir>|--testsuite=<testsuit_dir>]
                   [-o <out_dir>|--out=<out_dir>]
                   [--verbose=[<verbose_output_path>]]
                   [-j <jobs_cnt>|--parallel=<jobs_cnt>]
                   [--process-only]
                   [-v <view_path>|--view-only=<view_path>]
                   [-r|--retain-output]
                   [-s <path>|--save=<path>]'''


def parser_cli(argv: list[str]):
    try:
        opts, _ = getopt.getopt(argv[1:], 'hdrc:t:o:j:s:v:', ['help', 'show-default', 'retain-output', 'verbose=', 
        'config=', 'testsuit=', 'out=', 'parallel=', 'save=', 'process-only', 'view-only='])
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
                print(f'result_save_path:\t{argument.result_save_path}')
                print(f'process_only:\t{bool(argument.task & argument.Task.process)}')
                print(f'view_only:\t{bool(argument.task & argument.Task.view)}')
                print(f'view_path:\t{argument.view_path}')
                exit(0)
            case '-r' | '--retain-output':
                argument.auto_clean = False
            case '-c' | '--config':
                argument.config_path = os.path.abspath(arg)
            case '-t' | '--testsuit':
                argument.test_suite_dir = os.path.abspath(arg)
            case '-o' | '--out':
                argument.out_dir = os.path.abspath(arg)
            case '-j' | '--parallel':
                argument.parallel = int(arg)
            case '--verbose':
                argument.verbose = True
                if arg is not None and arg != '':
                    argument.verbose_output_path = os.path.abspath(arg)
            case '-s' | '--save':
                argument.result_save_path = os.path.abspath(arg)
                argument.view_path = os.path.abspath(arg)
            case '--process-only':
                argument.task = argument.Task.process
            case '-v' | '--view-only':
                argument.task = argument.Task.view
                argument.view_path = os.path.abspath(arg)
            case _:
                raise ValueError(f'Unknown option: {opt}')


def cleanup_binary():
    import shutil
    for entry in os.listdir(argument.out_dir):
        path = os.path.join(argument.out_dir, entry)
        if os.path.isdir(path) and entry != 'CAMI':
            shutil.rmtree(path)


def process():
    with open(argument.config_path, 'r') as f:
        config = Config.model_validate_json(f.read())
    import time
    start = time.time()
    result = Engine(config).run(argument.eval_root, argument.test_suite_dir, argument.out_dir)
    end = time.time()
    print(f'processing cost time: {end - start:.5f}s')
    with open(argument.result_save_path, 'w') as f:
        import json
        result_dict = {k: v.dict() for k, v in result.items()}
        json.dump({'test_suite_dir': argument.test_suite_dir, 'result': result_dict}, f)
    if argument.auto_clean:
        cleanup_binary()


def view(path: str):
    import json
    from result import Tool
    with open(path, 'r') as f:
        result = json.load(f)
    Printer(result['test_suite_dir']).print({k: Tool(**v) for k, v in result['result'].items()})


def main(argv: list[str]):
    parser_cli(argv)
    if argument.task & argument.Task.process:
        process()
    if argument.task & argument.Task.view:
        view(argument.view_path)


if __name__ == "__main__":
    main(sys.argv)
    exit(0)
