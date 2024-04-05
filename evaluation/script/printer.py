from result import Tool
import math
import os
from dataclasses import dataclass
import argument

@dataclass
class Printer:
    prefix: str

    def print(self, eval_result: dict[str, Tool]):
        self._print_detail_info(eval_result, False)
        self._print_detail_info(eval_result, True)
        self._print_overall_info(eval_result)
        if argument.verbose:
            self._print_output(eval_result)

    def _print_detail_info(self, eval_result: dict[str, Tool], negative: bool):
        cases = self._collect_test_cases(eval_result, negative)
        if len(cases) == 0:
            print(f'\033[34m{"negative" if negative else "positive"} test suite\033[0m:\n\n\tno test case executed\n')
            return
        syms = ['‼', '○'] if negative else ['✓', '✗']
        max_case_len = max(len(s) for s in cases)
        print(f'\033[34m{"negative" if negative else "positive"} test suite\033[0m:\n')
        print('\t{:<{width}}'.format('', width=max_case_len), end='')
        tool_name_lens: list[int] = []
        # guarentee the sequence of traversal
        tool_results: list[Tool] = []
        for tool_name, tool_result in eval_result.items():
            print(' {:^7}'.format(tool_name), end='')
            tool_name_lens.append(max(len(tool_name), 7))
            tool_results.append(tool_result)
        print('')
        for case in cases:
            print('\t{:<{width}}'.format(case, width=max_case_len), end='')
            cnt = 0
            for tool_result in tool_results:
                executed = False
                for case_result in tool_result.negative if negative else tool_result.positive:
                    if case_result.name == os.path.join(self.prefix, case):
                        as_expected = (negative and not case_result.alarmed) or not negative and case_result.alarmed
                        print('\033[32m' if as_expected else '\033[31m', end='')
                        print(' {:^{width}}'.format(syms[0] if case_result.alarmed else syms[1], width=tool_name_lens[cnt]), end='')
                        print('\033[0m', end='')
                        executed = True
                        break
                if not executed:
                    print(' {:^{width}}'.format('-', width=tool_name_lens[cnt]), end='')
                cnt = cnt + 1
            print('')
        print('')

    def _print_overall_info(self, eval_result: dict[str, Tool]):
        print('\033[34mstatistic\033[0m:\n')
        print('\t{:<19}'.format(''), end='')
        tool_name_lens: list[int] = []
        # guarentee the sequence of traversal
        tool_results: list[Tool] = []
        for tool_name, tool_result in eval_result.items():
            print('  {:>7}'.format(tool_name), end='')
            tool_name_lens.append(max(len(tool_name), 7))
            tool_results.append(tool_result)
        print('')
        print('\t{:<19}'.format('detection rate'), end='')
        cnt = 0
        for tool_result in tool_results:
            detect_rate = self._calculate_detection_rate(tool_result)
            if detect_rate is not None:
                print('  {:>{width}.2f}%'.format(detect_rate, width=tool_name_lens[cnt] - 1), end='')
            else:
                print('  {:>{width}}'.format('-', width=tool_name_lens[cnt]), end='')
            cnt = cnt + 1
        print('')
        print('\t{:<19}'.format('false positive rate'), end='')
        cnt = 0
        for tool_result in tool_results:
            false_positive_rate = self._calculate_false_positive_rate(tool_result)
            if false_positive_rate is not None:
                print('  {:>{width}.2f}%'.format(false_positive_rate, width=tool_name_lens[cnt] - 1), end='')
            else:
                print('  {:>{width}}'.format('-', width=tool_name_lens[cnt]), end='')
            cnt = cnt + 1
        print('')
        print('\t{:<19}'.format('productivity metric'), end='')
        cnt = 0
        for tool_result in tool_results:
            productivity_metric = self._calculate_productivity_metric(tool_result)
            if productivity_metric is not None:
                print('  {:>{width}.2f}%'.format(productivity_metric, width=tool_name_lens[cnt] - 1), end='')
            else:
                print('  {:>{width}}'.format('-', width=tool_name_lens[cnt]), end='')
            cnt = cnt + 1
        print('')

    def _collect_test_cases(self, eval_result: dict[str, Tool], negative: bool) -> list[str]:
        names: set[str] = set()
        for tool_result in eval_result.values():
            cases = tool_result.negative if negative else tool_result.positive
            for case_result in cases:
                names.add(os.path.relpath(case_result.name, self.prefix))
        return list(names)

    def _calculate_detection_rate(self, tool_result: Tool) -> float|None:
        return sum(map(lambda r: r.alarmed, tool_result.positive)) / len(tool_result.positive) * 100 if len(tool_result.positive) != 0 else None

    def _calculate_false_positive_rate(self, tool_result: Tool) -> float|None:
        return sum(map(lambda r: r.alarmed, tool_result.negative)) / len(tool_result.negative) * 100 if len(tool_result.negative) != 0 else None

    def _calculate_productivity_metric(self, tool_result: Tool) -> float|None:
        detect_rate = self._calculate_detection_rate(tool_result)
        false_positive_rate = self._calculate_false_positive_rate(tool_result)
        return math.sqrt(detect_rate * (100 - false_positive_rate)) if detect_rate is not None and false_positive_rate is not None else None

    def _print_output(self, eval_result: dict[str, Tool]):
        promote = ['command', 'stdout', 'stderr']
        for name, tool_result in eval_result.items():
            for case_result in tool_result.positive:
                print(f'{name} {case_result.name}')
                for out in case_result.output:
                    for i in range(3):
                        print(f'\t{promote[i]}: {out[i]}')
                    print('\t----------------------------------')
                print('*************************************')
                