Evaluation
-------------

## Introduction
To demonstrate the ability of UB detection of CAMI, we did that:
+ constructed 178 test cases(including positive&negative suite)
+ built an automatic evaluation framework using python
+ configured an environment for evaluation using docker

After evaluation, you can get:
+ detection result for each test case
+ overall statistic including detection rate(in percent) & false-positive rate & productivity metric(geometric average of detection rate and 100% minus false-positive)
+ comparison among various detection tools(see next section)

### Detection tools
+ [CAMI](https://github.com/Liu-Xiangzhi/CAMI)
+ [GCC](https://gcc.gnu.org/) with UBSan, ASan
+ [Clang](https://clang.llvm.org/) with UBSan
+ [cppcheck](https://cppcheck.sourceforge.io/) (static analysis)
+ [scan-build](https://clang-analyzer.llvm.org/scan-build.html) (static analysis)
+ [Frama-C](https://www.frama-c.com/) (Value Analysis) (static analysis)
+ [Valgrind](https://valgrind.org/)
+ [kcc](https://github.com/runtimeverification/match/releases)
+ [Compcert C Interpreter](https://compcert.org/)

## Build docker image
```shell
cd ${PROJECT_ROOT}
docker build -f ./evaluation/Dockerfile -t cami_eval .
```

## Run evaluation
Users can run the commands in the "Quick Run" section to obtain results corresponding to the pre-configured parameters, or they can customize evaluation parameters according to the instructions in the "Detection Software Configuration" and "Parameters of Automated Evalution Script" sections.

### Quick Run
```shell
docker run -it --ulimit nofile=65536:65536 -p 8080:8000 cami_eval
# below are commands executed in docker container
cd cami
python evaluation/main.py
```
#### Get json format Evaluation Result
```shell
# in docker container
cd evaluation/out && python -m http.server >/dev/null 2>&1 &
```
Access `localhost:8080` in your browser and download `result.json`.
#### Get verbose output of detection softwares
Run the following command and download `evaluation_output.html` via `localhost:8080`
```shell
# in docker container
python evaluation/script/main.py -v evaluation/out/result.json --verbose=
```

### Detection Software Configuration

Users can configure `evaluation/config/config.json` to specify the detection software and corresponding parameters for evaluation, as well as ignore rules for test cases. 
Structure of this file is as following:
``` json
{
    "tools" : {
        "<tool name>" : {
            "description" : "some description for this configuration",
            "commands" : [
                {
                    "command": "cami run ${input}",
                    "interrupt_on_error": false
                }
                // ...
            ],
            "detect_word" : "Undefined Behavior detected"
        }
        // ...
    },
    "filter": [
        {
            "tool" : "<tool name regex>",
            "rule" : [
                "regex1",
                "regex2"
                // ...
                ]
        }
        // ...
    ]
}
```
where, the `tools` object configures different detection softwares and the `filter` object configures filter rules.

Each key-value pair in the `tools` object represents a configuration for a detection software. The key is the name of the configuration(usually the name of the software), and the value has the following meanings:

| Key | Value |
|-----|-------|
| `description` | Description of the configuration, which is currently not important|
| `commands` | A sequence of commands to be executed for running a particular test case|
| `detect_word` | This field is an eregex (as explained later) used to match the output of the detection software. If a match is found, it indicates that the current test case is recognized by the detection software as having undefined behavior|

The `command` field under the `commands` object specifies the command to be executed and may include parameters. The evaluation script will perform parameter substitution before executing the substituted command. Parameters to be replaced are enclosed in `${}`. If you want to express the literal meaning of `${`, you need to use `$` to escape it. For example, `$${xxx}` will be substituted to `${xxx}`, not means the result of concatenation of `$` with the substitution of `xxx`. Parameter substitution cannot be nested.

Avaliable paramters and its meaning are as following:
|parameter|value after substitution|
|----|-----|
|`eval_root`|evaluation directory of this project, i.e. `${PROJECT_ROOT}/evaluation`|
|`test_suite_dir`|directory of test cases|
|`out_dir`|output directory of intermediate and final products|
|`input`|the currently running test case.|
|`local_out_dir`| The output directory exclusively used by the current detection software under `${out_dir}`, i.e., `${out_dir}/<tool_name>/`|
|`unique`| A randomly generated unique string. The current generation logic is `str(time.time()).replace('.', '_')`|
|`output`| A unique string generated based on the current test case name. The current generation logic is `input_name.replace(os.sep, '-') + '.out'`|
|`shell_ext`|extension name of shell file of current OS(`bat` in Windows and `sh` in Unix-like)|

The `interrupt_on_error` field under the `commands` object indicates whether the command can fail. When `interrupt_on_error` is false, the next command will be run regardless of whether current command exited successfully. Conversely, if the current instruction fails when `interrupt_on_error` is true, the running of the current test case will be interrupted.

The `filter` object allows different filtering rules for different detection software configurations. Both the `tool` and `rule` fields' values are eregex.

The evaluation execution logic is as follows:
``` python
def fileter_test_case(tool_name, test_case_name):
    for flt in filters:
        if eregex.match(flt.tool, tool_name):
            return any(lambda r:eregex.match(r, test_case_name), flt.rule)
    return True

def process(tool, test_case_name):
    for command, interrupt_on_error in tool.commands:
        if run(substitute_param(command)).returncode != 0 and interrupt_on_error:
            # report error
            return
        # do some other things

for tool, tool_name in tools.items():
    for test_case_name in testcases:
        if not filter_test_case(tool_name, test_case_name):
            continue
        process(tool, test_case_name)
```

#### eregex (Enhenced regex)
Enhanced regular expressions have reverse matching ability by adding the `!` prefix to regular expressions. Specifically:
- `!` is an invalid eregex.
- `!xxxx` indicates reverse matching for `xxxx`.
- `!!xxxx` indicates matching for `!xxxx`.
- `!!!xxxx` indicates reverse matching for `!xxxx`.
where `x` is a non `!` character.

### Parameters of Automated Evalution Script
The parameters and their meanings for the automated evaluation script are as follows:

|parameter| meaning|
|----------------|-------------------------------------------------------------|
| -h --help     | Print help information. |
| -d --show-default | Print default values for all parameters.|
| -c --config   | Specify the path to the configuration file.|
| -t --testsuite| Specify the directory of testcases|
| -o --out      | Specify the output directory.|
| --verbose      | Integrate the output of all detection software for each test case into an HTML tables when viewing the evaluation results. An additional output path can be specified. |
| -j, --parallel | Specify the number of test cases to run in parallel.|
| --process-only | Perform evaluation only, without printing evaluation results.|
| -v, --view-only| Specify the path to the evaluation result JSON file. View the evaluation results without executing the evaluation process.|
| -r, --retain-output | Retain intermediate products (i.e., output files from each detection software for each test case). |
| -s, --save     | Specify the path to save the evaluation results.|

Example:
```shell
cd evaluation/ && python script/main.py -c config/config.json -t test_suite -o out -s out/result.json
```
The evaluation script reads the `config/config.json` configuration file, recursively runs all test cases under `test_suite`, outputs intermediate products to the `out` directory, and outputs the results to `out/result.json`. After all test cases have been run, the evaluation script will automatically clean up all directories under `out`, read and parse the `out/result.json` file, and print the results in tabular form to the terminal.

```shell
cd evaluation/ && python script/main.py --view-only=out/result.json --verbose=
```
The evaluation script will skip the evaluation process and directly read and parse the `out/result.json` file, printing the results in tabular form to the terminal. Additionally, it will save HTML tables of the output from all detection software for each test case to the default path (`out/evaluation_output.html`).
