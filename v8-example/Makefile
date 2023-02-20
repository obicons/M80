CXXFLAGS=-std=c++2b -fno-rtti								\
         -lv8_monolith										\
		 -lv8_libbase -lv8_libplatform -ldl 				\
		 -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX			\
		 -I$(HOME)/src/v8/v8/ -I$(HOME)/src/v8/v8/include	\
		 -L$(HOME)/src/v8/v8//out.gn/x64.release.sample/obj/
objs=$(shell find . -maxdepth 1 -name '*.cc' -exec sh -c 'echo build/`basename {}`' \; | sed 's/\.cc/\.o/g')
dyobjs=$(shell find lib -maxdepth 1 -name '*.cc' -exec sh -c 'echo build/{}' \; | sed 's/\.cc/\.so/g')

all: create-build-directory ./build/main $(dyobjs)

./build/main: $(objs)
	$(CXX) $(objs) $(CXXFLAGS) -o ./build/main

.PHONY: create-build-directory
create-build-directory:
	mkdir -p build
	mkdir -p build/lib

build/lib/%.o : lib/%.cc
	$(CXX) -c -std=c++2b -fPIC $< -o $@

build/%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

build/lib/%.so : build/lib/%.o
	$(CXX) -shared $< -o $@

clean:
	rm -fr $(objs) main build/
