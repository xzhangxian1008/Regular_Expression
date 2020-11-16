target = c
objects = DFA.o parse_regular.o data_struct.o main.o
CC = g++
FLAGS = -std=c++11 -g

#显然这样的源文件与头文件依赖关系设置过于简单粗暴
#但是因为头文件规模尚小所以未使用自动生成依赖
cpp_headers = DFA.hpp data_struct.hpp parse_regular.hpp utility.hpp

.PHONY:all
all: $(target) 

$(target): $(objects) $(cpp_headers)
	$(CC) $(objects) $(FLAGS)  -o $(target)

$(objects):%.o:%.cpp $(cpp_headers)
	$(CC) -c $(FLAGS) $< -o $@

.PHONY:clean
clean:
	rm $(target) $(objects)