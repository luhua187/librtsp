
CC := gcc
CFLAGS    := -g -Wall -O2 -Wno-deprecated -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
LD_PATH   :=  
LDFLAGS   := -L../lib -lrtsp -lpthread -lm  -lstdc++
INC_PATH  := -I../lib 



all:demo


demo:demo.cpp
	$(CC)  $(CFLAGS) $(INC_PATH)  demo.cpp -o  demo $(LDFLAGS)


.PHONY:clean

clean:
	@rm -rf *.o demo