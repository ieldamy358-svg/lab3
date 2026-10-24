CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra

.PHONY: all clean

all: myshell

myshell: command.cc command.h tokenizer.cc tokenizer.h
	$(CXX) $(CXXFLAGS) command.cc tokenizer.cc -o myshell

clean:
	rm -f myshell shell_log.txt