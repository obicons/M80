OUT_DIR=bin/
CXXFLAGS=-Wall -Wextra -Wpedantic -Idependencies/ELFIO/ -g # -fsanitize=address
objs=$(shell find . -name *.cc | sed 's/\.cc/\.o/g')

main: $(objs) trampoline.o
	$(CXX) $(CXXFLAGS) $(objs) trampoline.o -o main

.PHONY: build-in-container
build-in-container:
	./build_in_container.sh

trampoline.o: trampoline.S
	gcc -c trampoline.S -o trampoline.o

# .PHONY: elfio
# elfio:
# 	./dependencies/make_elfio.sh

.PHONY: clean
clean:
	rm -f *.o main trampoline.o
