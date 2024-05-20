# 评估指南

## 介绍
为了展示 CAMI 的检测能力，我们进行了如下工作：
+ 构造个178个测试样例（包含阳性套件和阴性套件）
+ 使用 python 构建了自动化评估框架（脚本）
+ 使用 docker 配置了评估环境

评估之后，你可以得到：
+ 对于每一个测试样例的检测结果
+ 总体的统计信息，包含检测率（以百分比为单位），假阳性率和产品指数（检测率和 100% - 假阳性率的几何平均数）
+ 不同检测软件间的对比（见下节）

### 检测软件
+ [CAMI](https://github.com/Liu-Xiangzhi/CAMI)
+ [GCC](https://gcc.gnu.org/) with UBSan, ASan
+ [Clang](https://clang.llvm.org/) with UBSan
+ [cppcheck](https://cppcheck.sourceforge.io/) (static analysis)
+ [scan-build](https://clang-analyzer.llvm.org/scan-build.html) (static analysis)
+ [Frama-C](https://www.frama-c.com/) (Value Analysis) (static analysis)
+ [Valgrind](https://valgrind.org/)
+ [kcc](https://github.com/runtimeverification/match/releases)
+ [Compcert C Interpreter](https://compcert.org/)

## 构建 Docker 镜像
```shell
cd ${PROJECT_ROOT}
docker build -f ./evaluation/Dockerfile -t cami_eval .
```

## 运行评估脚本
用户可执行“快速运行”一节中的命令得到预配置参数对应的结果，也可以按照“检测软件配置”和“自动化评估脚本参数”的指导，自定义评估参数。
### 快速运行
运行如下命令后将在终端显示评估结果
```shell
docker run -it --ulimit nofile=65536:65536 -p 8080:8000 cami_eval
# below are commands executed in docker container
cd cami
python evaluation/script/main.py
```
#### 获取 json 格式的评估结果
```shell
# in docker container
cd evaluation/out && python -m http.server >/dev/null 2>&1 &
```
在浏览器中访问 `localhost:8080` 并下载 `result.json` 文件。
#### 获取检测软件的详细输出
运行如下命令后通过 `localhost:8080` 下载 `evaluation_output.html` 文件
```shell
# in docker container
python evaluation/script/main.py -v evaluation/out/result.json --verbose=
```

### 检测软件配置
用户可配置 `evaluation/config/config.json` 文件来指定参与评估的检测软件和对应参数已经针对测试用例的忽略规则。
该文件的结构如下
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
其中 `tools` 对象用于配置不同的检测软件，`filter` 对象用于配置过滤规则。

`tools` 对象中的每一个键值对代表一个检测软件配置，其中的键是对该配置的命名，值的含义如下：

|键|值|
|--|--|
|`description`| 对该配置的描述，目前该字段并不重要。|
|`commands`| 该配置下运行某一个测试样例需要执行的一串命令|
|`detect_word`| 该字段为 eregex（见后文），用来匹配检测软件的输出，若匹配成功则代表当前测试用例被测试软件认定为存在未定义行为。|

其中`commands`对象下的`command`字段为执行的指令且可包含参数，评估脚本会先进行参数替换再执行替换后的命令，需要替换的参数用`${}`包裹。若想表达`${`的字面含义需使用`$`转义，即`$${xxx}`进行参数替换后的结果为`${xxx}`，而不是`$`和`xxx`替换后的结果拼接得到的结果。参数替换不可嵌套。可用参数及含义如下：

|参数|替换值|
|----|-----|
|`eval_root`|本项目的 evaluation 目录，即`${PROJECT_ROOT}/evaluation`|
|`test_suite_dir`|测试用例集目录|
|`out_dir`|中间产物和最终结果输出目录|
|`input`|当前运行的测试用例|
|`local_out_dir`|在`${out_dir}`目录下当前检测软件独占的输出目录，即`${out_dir}/<tool_name>/`|
|`unique`|随机产生的独一无二的字符串，当前生产逻辑为 `str(time.time()).replace('.', '_')`|
|`output`|根据当前测试用例名称生成的独一无二的字符串，当前生成逻辑为 `input_name.replace(os.sep, '-') + '.out'`|
|`shell_ext`|当前操作系统中默认脚本文件的后缀名（Windows 中为`bat`，类 Unix 系统在为`sh`）|

`commands`对象下的`interrupt_on_error`字段表示该命令是否可失败。`interrupt_on_error`为假时无论当前指令是否执行成功都会执行下一条指令，反之，若当前指令执行失败则终止当前测试样例的运行。

`filter` 对象可针对不同的检测软件配置不同的过滤规则，其中`tool`和`rule`字段的值均为 eregex。

评估的执行逻辑如下：

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

#### eregex（增强的正则表达式）
增强的正则表达式在正则表达式上提供了反向匹配的功能，通过对正则表达式添加前缀`!`进行表达。即（用字符“x”表示非“!”的字符）：
+ `!` 错误的 eregex
+ `!xxxx` 反向匹配 `xxxx` 
+ `!!xxxx` 匹配 `!xxxx`
+ `!!!xxxx` 反向匹配 `!xxxx`

### 自动化评估脚本参数
自动化评估脚本的参数及其含义如下：
|参数|含义|
|---|----|
|-h --help |打印帮助信息|
|-d --show-default |打印所有参数的默认值|
|-c --config |指定配置文件路径|
|-t --testsuite |指定测试数据集目录|
|-o --out |指定输出目录|
|--verbose | 在查看评估结果时整合所有检测软件对每一个测试样例的输出生成对应的 html 表格。可额外指定输出路径|
|-j --parallel |指定并行运行的测试用例的数量|
|--process-only |仅执行评估，不打印评估结果|
|-v --view-only |指定评估结果 json 文件所在路径，仅查看该评估结果，不执行评估过程|
|-r --retain-output |保留中间产物（即每个检测软件对于每个测试样例的输出文件）|
|-s --save |指定评估结果的存放路径|


示例：
```shell
cd evaluation/ && python script/main.py -c config/config.json -t test_suite -o out -s out/result.json
```
评估脚本读取`config/config.json`配置文件，递归地运行`test_suite`下所有的测试用例，将中间产物输出到`out`目录，将结果输出到`out/result.json`。所有测试用例运行完毕后评估脚本将自动清理`out`目录下所有目录，读取并解析`out/result.json`文件并以表格形式打印到终端上。

```shell
cd evaluation/ && python script/main.py --view-only=out/result.json --verbose=
```
评估脚本将跳过评估的执行，直接读取并解析`out/result.json`文件并以表格地形式打印到终端上，同时，还会将所有检测软件对每一个测试样例的输出生成对应的 html 表格保存在默认路径（`out/evaluation_output.html`）下。
