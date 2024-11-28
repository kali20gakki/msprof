set(MSPROF_CODE ${TOP_DIR}/abl/msprof/analysis)
set(MSPROF_DB ${TOP_DIR}/llt/abl/msprof/msprof_python/ut/testcase)
set(MSPTI_CODE ${TOP_DIR}/abl/mspti/whl)

run_python_llt_test(
    TARGET msprof_python_utest
    SRC_FILES_DIR ${TOP_DIR}/abl/msprof/analysis/
    TEST_FILES_DIR ${TOP_DIR}/llt/abl/msprof/msprof_python/ut/testcase
    EXPORT_PYTHONPATH "${MSPROF_CODE}:${MSPROF_DB}:${MSPTI_CODE}"
)
