
libhdclog_device.a : hdclog_device_init.o \
                     hdclog_device_ide_cmd.o \
                     hdclog_device_multi_thread.o \
                     hdclog_device_process.o \
                     hdclog_device_tool.o
	ar crv libhdclog_device.a libhdclog_device.a hdclog_device_init.o \
                                     hdclog_device_ide_cmd.o \
                                     hdclog_device_multi_thread.o \
                                     hdclog_device_process.o \
                                     hdclog_device_tool.o

hdclog_device_init.o : hdclog_device_init.c \
                       hdclog_device_init.h \
                       hdclog_device_com.h \
                       hdc-api.h \
                       hdclog_device_multi_thread.h
	gcc -c hdclog_device_init.c


hdclog_device_ide_cmd.o : hdclog_device_ide_cmd.c \
                          hdclog_device_ide_cmd.h \
                          hdclog_device_com.h \
                          hdc-api.h
	gcc -c hdclog_device_ide_cmd.c 


hdclog_device_multi_thread.o : hdclog_device_multi_thread.c \
                               hdclog_device_multi_thread.h \
                               hdclog_device_com.h \
                               hdc-api.h \
                               securec.h
	gcc -c hdclog_device_multi_thread.c -L. -lsecurec


hdclog_device_process.o : hdclog_device_process.c \
                          hdclog_device_process.h \
                          hdclog_device_com.h
	gcc -c hdclog_device_process.c

hdclog_device_tool.o : hdclog_device_tool.c \
                       hdclog_device_tool.h \
                       hdclog_device_com.h \
                       hdclog_device_ide_cmd.h \
                       securec.h
	gcc -c hdclog_device_tool.c -L. -lsecurec
	

.PHONY: clean
clean:
	rm -rf *.o


