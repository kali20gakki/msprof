## download
dependency download
```bash
bash scripts/download_thirdparty.sh
```

## build
build msprof、libmsprofiler.so、stub/libmsprofiler.so, if you want to build local, please comment `bep_env_init`
```bash
# default
bash build/build.sh

# Debug
bash build/build.sh Debug
```


## DT
```bash
# cpp
bash scripts/execute_test_case.sh
bash scripts/generate_coverage_cpp.sh

# python
bash scripts/generate_coverage_py.sh
```

## fuzz
```bash
# default run all module fuzz testcase for 1000000 times
bash scripts/fuzz_cpp_testcase.sh

# run specific module(collector/analysis/mspti/all) fuzz testcase
bash scripts/fuzz_cpp_testcase.sh -m collector

# run fuzz testcase for specific times
bash scripts/fuzz_cpp_testcase.sh -t 10

# run specific module fuzz testcase for specific times
bash scripts/fuzz_cpp_testcase.sh -t 10 -m collector

# fuzz test result path
test/fuzz_test/result
```