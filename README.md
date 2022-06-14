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