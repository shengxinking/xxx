TOPDIR=..

EXTRA_INCS += -I /usr/local/apr/include/apr-1 -I /usr/include/mysql 

OBJS1 = ip_addr.o sock_util.o mempool.o connection.o packet.o thread_fun.o

OBJS2 = http_basic.o  http_table.o http_header.o  http_api.o http.o\
                http_body.o http_dump.o base64.o http_poll.o http_arg.o http_decode.o http_modify.o http_form.o

OBJS3 = pat.o acsmx.o sqlite_db.o load_rule.o load_key.o pcre_func.o log_db.o
#OBJS4 = load_config.o para_limit.o url_limit.o  custom_limit.o
OBJS5 = accept.o work.o proxy.o main.o

OBJS6 = load_cfg.o clock.o cfg_db.o detect_module.o accept.o work2.o proxy2.o main2.o 

EXTRA_LIBS = -L /usr/local/apr/lib  -L  /usr/lib64/mysql  
EXTRA_LDFLAGS = -L /usr/local/apr/lib 
#TARGET_PROG = proxyd
#OBJECTS_PROG = $(OBJS1) $(OBJS2) $(OBJS3)  $(OBJS5)
#TARGET_PROG_LIBS = -lpcre -lsqlite3  -lpthread -lapr-1 

TARGET_TEST = proxyd_re
OBJECTS_TEST = $(OBJS1) $(OBJS2) $(OBJS3)  $(OBJS6)

TARGET_TEST_LIBS = -lpcre -lsqlite3  -lpthread -lapr-1  -lmysqlclient 
include $(TOPDIR)/rules.Make
