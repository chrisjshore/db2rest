DB2PATH=/opt/ibm/db2/V11.5
EXTRA_LFLAG=-Wl,-rpath,$(DB2PATH)/lib64

default:
	gcc -g $(EXTRA_LFLAG) -I$(DB2PATH)/include -L$(DB2PATH)/lib64 -o db2rest db2rest.c -lpthread -ljansson -lulfius -ldb2
