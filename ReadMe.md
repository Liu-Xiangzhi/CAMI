CAMI
-----------
C Abstract Machine Interpreter, also an examiner of your C programs.

See [here](./doc/index.md) for other languages.

## Introduction
TBD

## Build
Note:
+ let `${PROJECT_ROOT}` denote the root directory of this project.
+ it can be built only under Unix-like platform now.
### Configure (optional)
Modify the configuration items in `${RROJECT_ROOT}/config/config.toml` to configure different parameters.
See [here](./config/ReadMe.md) for detail.
### Compile
```shell
cd ${PROJECT_ROOT}
cmake -S . -B build
cmake --build buils
```
### Install (optional)
```shell
cd ${PROJECT_ROOT}
cmake --build build --target install # may need 'sudo'
```

## Usage
```shell
cami run <file_name>
```

## Possible Improvement
+ support more format types(e.g. json, yaml) for AM
+ support serialization & deserialization of AM
+ provide more precise debug info & support more debug manipulations(e.g. step execution, breakpoint)
+ fulfil the atomicity of execution(an instruction either executes successfully or fails without changing any state of AM)

**Note**: restrict to the time of development(as this project is my bachelor thesis project), 
please do not impose too much requirement or expectation on the code quality of this program,
and this program is **NOT** undergone exhausted test

DONE IS BETTER THAN PERFECT
