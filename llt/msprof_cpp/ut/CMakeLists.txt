# add_subdirectory(app) # ok
# add_subdirectory(common) # core dump
# add_subdirectory(device) # OK
# add_subdirectory(driver) # ok
# add_subdirectory(job_wrapper) # ok
# add_subdirectory(message)  # No rule to make target 'install'
# add_subdirectory(msprof) # ok
# add_subdirectory(transport) # SendFiles死循环
# add_subdirectory(streamio) # dumped core client（OK）
# add_subdirectory(msprofbin)
add_subdirectory(stub_utest)