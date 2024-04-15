from result import Tool, Case
import math
import os
from dataclasses import dataclass
import argument
from typing import TextIO
import html

html_header = '''<!DOCTYPE html>
<html>
<head> 
<meta charset="utf-8"> 
<title>evaluation outputs</title>
</head>
<body>
'''

html_end = '''</body>
<style>
    table {
        border: 1px solid #000000;
        border-collapse: collapse;
    }
    
    th, td {
        border: 1px solid #000000;
    }
    .output {
        overflow: auto;
    }
    .testcase {
        color: green;
    }
    .cmd {
        color: blue;
        white-space: nowrap;
        overflow-x: auto;
    }
    .tool {
        color: red;
        text-align: center;
        overflow: auto;
    }
</style>
</html>
'''

@dataclass
class Printer:
    prefix: str

    def print(self, eval_result: dict[str, Tool]):
        self._print_detail_info(eval_result, 'positive')
        self._print_detail_info(eval_result, 'negative')
        self._print_overall_info(eval_result)
        if argument.verbose:
            with open(argument.verbose_output_path, 'w') as f:
                self._print_verbose_output(eval_result, f)

    def _print_detail_info(self, eval_result: dict[str, Tool], suite: str):
        is_positive_suite = suite == 'positive'
        cases = self._collect_test_cases(eval_result, suite)
        if len(cases) == 0:
            print(f'\033[34m{suite} test suite\033[0m:\n\n\tno test case executed\n')
            return
        syms = ['✗', '✓'] if is_positive_suite else ['○', '‼']
        max_case_len = max(len(s) for s in cases)
        print(f'\033[34m{suite} test suite\033[0m:\n')
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
                if (case_result := Printer._find_case(os.path.join(os.path.join(self.prefix, suite), case), getattr(tool_result, suite))) is not None:
                    as_expected = is_positive_suite == case_result.alarmed
                    print('\033[32m' if as_expected else '\033[31m', end='')
                    print(' {:^{width}}'.format(syms[int(case_result.alarmed)], width=tool_name_lens[cnt]), end='')
                    print('\033[0m', end='')
                else:
                    print(' {:^{width}}'.format('-', width=tool_name_lens[cnt]), end='')
                cnt = cnt + 1
            print('')
        print('')

    def _print_overall_info(self, eval_result: dict[str, Tool]):
        def print_overall_info(tool_results: list[Tool], tool_name_lens: list[int], info: str):
            print('\t{:<19}'.format(info), end='')
            for i, tool_result in enumerate(tool_results):
                rate = getattr(self, '_calculate_'+info.replace(' ', '_'))(tool_result)
                if rate is not None:
                    print('  {:>{width}.2f}%'.format(rate, width=tool_name_lens[i] - 1), end='')
                else:
                    print('  {:>{width}}'.format('-', width=tool_name_lens[i]), end='')
            print('')

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
        print_overall_info(tool_results, tool_name_lens, 'detection rate')
        print_overall_info(tool_results, tool_name_lens, 'false positive rate')
        print_overall_info(tool_results, tool_name_lens, 'productivity metric')

    def _collect_test_cases(self, eval_result: dict[str, Tool], suite: str) -> list[str]:
        names: set[str] = set()
        for tool_result in eval_result.values():
            cases =  getattr(tool_result, suite)
            for case_result in cases:
                names.add(os.path.relpath(case_result.name, os.path.join(self.prefix, suite)))
        name_list = list(names)
        name_list.sort()
        return name_list

    def _calculate_detection_rate(self, tool_result: Tool) -> float|None:
        return sum(map(lambda r: r.alarmed, tool_result.positive)) / len(tool_result.positive) * 100 if len(tool_result.positive) != 0 else None

    def _calculate_false_positive_rate(self, tool_result: Tool) -> float|None:
        return sum(map(lambda r: r.alarmed, tool_result.negative)) / len(tool_result.negative) * 100 if len(tool_result.negative) != 0 else None

    def _calculate_productivity_metric(self, tool_result: Tool) -> float|None:
        detect_rate = self._calculate_detection_rate(tool_result)
        false_positive_rate = self._calculate_false_positive_rate(tool_result)
        return math.sqrt(detect_rate * (100 - false_positive_rate)) if detect_rate is not None and false_positive_rate is not None else None

    @staticmethod
    def _find_case(case: str, case_results: list[Case]) -> Case|None:
        for case_result in case_results:
            if case_result.name == case:
                return case_result
        return None

    def _print_verbose_output(self, eval_result: dict[str, Tool], f: TextIO):
        def print_table_head(suite: str):
            f.write('<tr>\n\t<td></td>\n')
            for name, tool_result in eval_result.items():
                case_results = getattr(tool_result, suite)
                if len(case_results) == 0:
                    continue
                f.write(f'\t<td colspan = "{len(case_results[0].output)}"class="tool">{html.escape(name)}</td>\n')
            f.write('</tr>\n')
            f.write('<tr>\n\t<td></td>\n')
            for _, tool_result in eval_result.items():
                case_results = getattr(tool_result, suite)
                if len(case_results) == 0:
                    continue
                for output in case_results[0].output:
                    f.write(f'\t<td class="cmd">{html.escape(output[0])}</td>\n')
            f.write('</tr>\n')

        def print_table_body(suite: str):
            def print_case_output(case: str, stderr: bool):
                translation_table = str.maketrans({'\n': '<br>', '\t': '&#9;', ' ': '&nbsp;'})
                if (case_result := Printer._find_case(os.path.join(os.path.join(self.prefix, suite), case), case_results)) is not None:
                    for output in case_result.output:
                        out = html.escape(output[1 + int(stderr)]).translate(translation_table)
                        f.write(f'\t<td class="output">{out}</td>\n')
                else:
                    for _ in range(len(case_results[0].output)):
                        f.write('\t<td sytle="text-align: center;">-</td>\n')

            for case in self._collect_test_cases(eval_result, suite):
                f.write(f'<tr>\n\t<td rowspan="2" class="testcase">{html.escape(case)}</td>\n')
                for _, tool_result in eval_result.items():
                    case_results = getattr(tool_result, suite)
                    if len(case_results) == 0:
                        continue
                    print_case_output(case, False)
                f.write('</tr>\n')
                f.write('<tr>\n')
                for _, tool_result in eval_result.items():
                    case_results = getattr(tool_result, suite)
                    if len(case_results) == 0:
                        continue
                    print_case_output(case, True)
                f.write('</tr>\n')

        f.write(html_header)
        f.write('<h1>positive test suite</h1>\n')
        f.write('<table>\n')
        print_table_head('positive')
        print_table_body('positive')
        f.write('</table>\n')
        f.write('<h1>negative test suite</h1>\n')
        f.write('<table>\n')
        print_table_head('negative')
        print_table_body('negative')
        f.write('</table>\n')
        f.write(html_end)
