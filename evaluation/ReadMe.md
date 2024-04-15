Evaluation
-------------

## Introduction
To demonstrate the ability of UB detection of CAMI, we did that:
+ constructed 100+ test cases(for positive/negative suite each)
+ built a automatic testing framework using python
+ configured an environment for evaluation using docker

After evaluation, you can get:
+ detection result for each test case
+ overall statistic including detection rate(in percent) & false-positive rate & productivity metric(geometric average of detection rate and 100% minus false-positive)
+ comparison among various detection tools(see next section)

### Detection tools
+ CAMI
+ GCC with UBSan, ASan
+ Clang with UBSan
+ cppcheck (static analysis)
+ scan-build (static analysis)
+ Frama-C (Value Analysis) (static analysis)
+ Valgrind
+ kcc
+ Compcert C Interpreter

## Build docker image
```shell
cd ${PROJECT_ROOT}
docker build -f ./evaluation/Dockerfile -t cami_eval .
```

## Run evaluation
```shell
docker run -it --ulimit nofile=65536:65536 -p 8080:8000 cami_eval
# below are commands executed in docker container
cd cami
python evaluation/main.py
```
