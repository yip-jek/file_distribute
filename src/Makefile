CPP = xlC
CPP_FLAGS = -g -brtl -O2 -q$(shell getconf LONG_BIT)
INC = -I../include

des = ../bin/file_distribute
srcs = $(wildcard *.cpp)
objs = $(srcs:.cpp=.o)

.PHONY: all

all: $(des)
	@echo ALL MADE!

$(des): $(objs)
	$(CPP) $(CPP_FLAGS) -o $@ $^

.cpp.o:
	$(CPP) $(CPP_FLAGS) $(INC) -c $<

clean:
	@echo Deleting $(des)
	@rm -f $(des)
	@echo Deleting $(objs)
	@rm -f $(objs)

