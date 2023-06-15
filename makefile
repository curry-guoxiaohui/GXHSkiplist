CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

# FLAG 为 1 ，功能测试显示日志，为0 不显示日志
FLAG ?= 0
ifeq ($(FLAG),1)
	ADDMICRO += -DDEBUG
else
	ADDMICRO += -Daa

endif

# 主文件的功能测试文件编译
main: main.cpp
	$(CXX) -o main  $^ $(CXXFLAGS) -DDEBUG

# 压力测试文件的编译 主要是对于添加功能和查询功能的测试
press: press_test.cpp
	$(CXX) -o press_test  $^ $(CXXFLAGS) $(ADDMICRO) -lpthread 

clean:
	rm  -r main

cleanp:
	rm  -r press_test