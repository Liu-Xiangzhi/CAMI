import re
import cfg
from cfg import Filter
import result
import os
import sys
from dataclasses import dataclass

def filte(filters: list[Filter], tool: str, test_case:str):
    for flt in filters:
        if not re.search(flt.tool, tool):
            continue
        return any(map(lambda r: re.search(r, test_case), flt.rule))
    return True

@dataclass
class Executor:
    tool_name: str
    tool_info: cfg.Tool
    eval_root: str
    test_suite_dir: str
    out_dir: str

    def execute(self, test_case: str) -> result.Case|None:
        import subprocess
        self.current_test_case = test_case
        output: list[tuple[str, str, str]] = []
        is_alarmed = False
        for cmd in self.tool_info.commands:
            cmd_list = self._parse_cmd(cmd.command)
            p = subprocess.run(cmd_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
            if p.returncode != 0 and cmd.interrupt_on_error:
                print(f'\n\033[31mtool "{self.tool_name}" execute aborted\033[0m, due to failed command: {" ".join(cmd_list)}\noutput: {p.stdout}\n {p.stderr}' ,file=sys.stderr)
                return None
            output.append((' '.join(cmd_list), str(p.stdout), str(p.stderr)))
            is_alarmed = is_alarmed or \
                     re.search(self.tool_info.detect_word, str(p.stdout)) is not None or \
                     re.search(self.tool_info.detect_word, str(p.stderr)) is not None
        
        return result.Case(name= test_case, output=output, alarmed=is_alarmed)

    def _parse_cmd(self, cmd:str) -> list[str]:
        import shlex
        return shlex.split(re.sub(r'(?<!\$)\$\{(\w+)\}', lambda s: self._substitue_variable(s.group(1)), cmd))
    
    def _substitue_variable(self, var:str) -> str:
        match var:
            case 'eval_root':
                return self.eval_root
            case 'test_suite_dir':
                return self.test_suite_dir
            case 'out_dir':
                return self.out_dir
            case 'input':
                return self.current_test_case
            case 'local_out_dir':
                return os.path.join(self.out_dir, re.sub(r'\s', '_',self.tool_name))
            case _:
                raise ValueError(f'Unknown command variable "{var}"')

@dataclass
class Engine:
    config: cfg.Config

    def run(self, eval_root: str, test_suite_dir: str, out_dir: str) -> dict[str, result.Tool]:
        import os
        import glob
        from pathlib import Path
        eval_result: dict[str, result.Tool] = {}
        tool_test_cases_num = len(glob.glob(os.path.join(test_suite_dir, '**/*.c'))) * len(self.config.tools)
        processed_case_cnt = 0
        clear_line = '\r' + ' ' * os.get_terminal_size().columns + '\r'
        for tool_name in self.config.tools.keys():
            out_path = os.path.join(out_dir, re.sub(r'\s', '_', tool_name))
            if not os.path.exists(out_path):
                os.makedirs(out_path)
        for test_case in Path(os.path.join(test_suite_dir, 'positive')).glob('**/*.c'):
            for name, tool in self.config.tools.items():
                processed_case_cnt = processed_case_cnt + 1
                print(f'{clear_line}processing {name} {test_case} [{processed_case_cnt}/{tool_test_cases_num}]', end='', flush=True)
                if not filte(self.config.filter, name, str(test_case.resolve())):
                    continue
                if eval_result.get(name) is None:
                    eval_result[name] = result.Tool(positive=[],negative=[])
                executor = Executor(name ,tool, eval_root, test_suite_dir, out_dir)
                case_result = executor.execute(str(test_case.resolve()))
                if case_result is not None:
                    eval_result[name].positive.append(case_result)
        for test_case in Path(os.path.join(test_suite_dir, 'negative')).glob('**/*.c'):
            for name, tool in self.config.tools.items():
                processed_case_cnt = processed_case_cnt + 1
                print(f'{clear_line}processing {name} {test_case} [{processed_case_cnt}/{tool_test_cases_num}]', end='', flush=True)
                if not filte(self.config.filter, name, str(test_case.resolve())):
                    continue
                if eval_result.get(name) is None:
                    eval_result[name] = result.Tool(positive=[],negative=[])
                executor = Executor(name, tool, eval_root, test_suite_dir, out_dir)
                case_result = executor.execute(str(test_case.resolve()))
                if case_result is not None:
                    eval_result[name].negative.append(case_result)
        print('')
        return eval_result
