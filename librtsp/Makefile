
CC := gcc
CFLAGS    := -g -Wall -O2 -fPIC -shared -Wno-deprecated -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-pointer-sign
LD_PATH   :=  
LDFLAGS   :=  -lpthread -lm
INC_PATH  := -I./include/ -I../librtp/include 

SRC = $(wildcard src/*.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

all:lib demo


demo:demo.c
	$(CC) $(INC_PATH)  demo.c -o demo -L./ -lrtsp -lpthread -lm -lstdc++

lib:$(OBJ)
	$(CC) $(CFLAGS) $(INC_PATH)  $(OBJ)  librtp.a -o librtsp.so


$(OBJ):%.o:%.c
	$(CC) -c $(CFLAGS) $(INC_PATH) $< -o $@



.PHONY:clean

clean:
	@rm -rf src/*.o demo
	@rm -rf librtsp.so
