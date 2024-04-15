import re
import cfg
from cfg import Filter
import result
import os
import sys
from dataclasses import dataclass
import asyncio
from eregex import ERegex

def filte(filters: list[Filter], tool: str, test_case:str):
    for flt in filters:
        tool_matcher = ERegex(flt.tool)
        if not tool_matcher.match(tool):
            continue
        return any(map(lambda r: ERegex(r).match(test_case), flt.rule))
    return True

@dataclass
class Executor:
    tool_name: str
    tool_info: cfg.Tool
    eval_root: str
    test_suite_dir: str
    out_dir: str

    async def execute(self, test_case: str) -> tuple[str, result.Case]|None:
        self.current_test_case = test_case
        output: list[tuple[str, str, str]] = []
        is_alarmed = False
        for cmd in self.tool_info.commands:
            cmd_list = self._parse_cmd(cmd.command)
            p = await asyncio.create_subprocess_exec(*cmd_list, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
            stdout, stderr = map(lambda x: x.decode('utf-8'), await p.communicate())
            if p.returncode != 0 and cmd.interrupt_on_error:
                print(f'\n\033[31mtool "{self.tool_name}" execute aborted\033[0m, due to failed command: {" ".join(cmd_list)}\noutput: {stdout}\n {stderr}' ,file=sys.stderr)
                return None
            output.append((' '.join(cmd_list), stdout, stderr))
            word_matcher = ERegex(self.tool_info.detect_word)
            is_alarmed = is_alarmed or word_matcher.match(stdout) or word_matcher.match(stderr)
        
        return (self.tool_name, result.Case(name= test_case, output=output, alarmed=is_alarmed))

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
            case 'unique':
                import time
                return str(time.time()).replace('.', '_')
            case 'output':
                input_name, _ = os.path.splitext(self.current_test_case)
                return input_name.replace(os.sep, '-') + '.out'
            case _:
                raise ValueError(f'Unknown command variable "{var}"')

@dataclass
class Engine:
    config: cfg.Config

    def run(self, eval_root: str, test_suite_dir: str, out_dir: str) -> dict[str, result.Tool]:
        import os
        import glob
        self.tool_test_cases_num = len(glob.glob(os.path.join(test_suite_dir, '**/*.c'), recursive=True )) * len(self.config.tools)
        self.processed_case_cnt = 0
        self.eval_root = eval_root
        self.test_suite_dir = test_suite_dir
        self.out_dir = out_dir
        self.eval_result: dict[str, result.Tool] = {}
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)
        for tool_name in self.config.tools.keys():
            out_path = os.path.join(out_dir, re.sub(r'\s', '_', tool_name))
            if not os.path.exists(out_path):
                os.makedirs(out_path)
        asyncio.run(self._run_suites())
        print('')
        return self.eval_result

    async def _run_suites(self):
        await self._run_suite('positive')
        await self._run_suite('negative')

    async def _run_suite(self, suite: str):
        from pathlib import Path
        clear_line = '\r' + ' ' * os.get_terminal_size().columns + '\r'
        tasks: list[asyncio.Task[tuple[str, result.Case]|None]] = []
        for test_case in Path(os.path.join(self.test_suite_dir, suite)).glob('**/*.c'):
            for name, tool in self.config.tools.items():
                self.processed_case_cnt = self.processed_case_cnt + 1
                print(f'{clear_line}processing {name} {test_case} [{self.processed_case_cnt}/{self.tool_test_cases_num}]', end='', flush=True)
                if not filte(self.config.filter, name, str(test_case.resolve())):
                    continue
                if self.eval_result.get(name) is None:
                    self.eval_result[name] = result.Tool(positive=[],negative=[])
                executor = Executor(name ,tool, self.eval_root, self.test_suite_dir, self.out_dir)
                import argument
                if len(tasks) >= argument.parallel:
                    done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
                    tasks = list(pending)
                    self._add_result(suite, done)
                tasks.append(asyncio.create_task(executor.execute(str(test_case.resolve()))))
        if len(tasks) != 0:
            self._add_result(suite, (await asyncio.wait(tasks, return_when=asyncio.ALL_COMPLETED))[0])
        
    def _add_result(self, suite: str, tasks: set[asyncio.Task[tuple[str, result.Case]|None]]):
        for task in tasks:
            if (result := task.result()) is not None:
                tool_name, case_result = result
                getattr(self.eval_result[tool_name], suite).append(case_result)
                
