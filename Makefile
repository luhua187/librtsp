
all:librtp librtsp  demo
.PHONY:all librtp librtsp  demo
	
librtp:
	@cd librtp && make
	@cd ./librtp && cp librtp.a ../librtsp

librtsp:
	@cd ./librtsp && make
	@cd ./librtsp/ && cp ./include/rtsp.h ../lib
	@cd ./librtsp && cp librtsp.so ../lib

demo:
	@cd ./demo && make


.PHONY:clean
clean:
	@cd librtp && make clean
	@cd librtsp && make clean
	@cd demo    && make clean