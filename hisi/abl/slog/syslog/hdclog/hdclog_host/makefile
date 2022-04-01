
libhdclog_host.a : hdclog_host_init.o \
                     hdclog_host_multi_thread.o \
                     hdclog_host_process.o \
                     hdclog_host_tool.o
	ar crv libhdclog_host.a libhdclog_host.a hdclog_host_init.o \
                                     hdclog_host_multi_thread.o \
                                     hdclog_host_process.o \
                                     hdclog_host_tool.o

hdclog_host_init.o : hdclog_host_init.c \
                       hdclog_host_init.h \
                       hdclog_host_com.h \
                       hdc-api.h \
                       hdclog_host_multi_thread.h
	gcc -c hdclog_host_init.c


hdclog_host_multi_thread.o : hdclog_host_multi_thread.c \
                               hdclog_host_multi_thread.h \
                               hdclog_host_com.h \
                               hdc-api.h \
                               securec.h
	gcc -c hdclog_host_multi_thread.c -L. -lsecurec


hdclog_host_process.o : hdclog_host_process.c \
                          hdclog_host_process.h \
                          hdclog_host_com.h
	gcc -c hdclog_host_process.c

hdclog_host_tool.o : hdclog_host_tool.c \
                       hdclog_host_tool.h \
                       hdclog_host_com.h \
                       hdclog_host_ide_cmd.h \
                       securec.h
	gcc -c hdclog_host_tool.c -L. -lsecurec
	

.PHONY: clean
clean:
	rm -rf *.o


