
LIB = $(shell pwd)
INCLUDE = $(shell pwd)/include

ROOT_LIBS = $(shell root-config --glibs)
ROOT_GCC_FLAGS = $(shell root-config --cflags)
# ROOT_LIBSEXTRA =  -lTreePlayer -lMathMore -lSpectrum -lMinuit 
# ROOT_LIBSEXTRA =  -lTreePlayer -lMathMore -lSpectrum -lMinuit -lPyROOT

CC = g++
CFLAGS = -std=c++11 -g -fPIC $(ROOT_GCC_FLAGS) -I$(INCLUDE) 
LIBRS = -L$(INCLUDE) $(ROOT_LIBS) -L$(LIB) -L$(LIB)/bin

OBJECTS = $(patsubst src/%.cpp,bin/%.o,$(wildcard src/*.cpp))
DEPS = $(OBJECTS:.o=.d)
# SYSHEAD = $(wildcard include/*.h)
# HEAD = $(patsubst %.h,$(shell pwd)/%.h,$(SYSHEAD))
PROG = $(patsubst programs/%.C,bin/%,$(wildcard programs/*.C))

SHAREDLIB = bin/SS.so
TARG = bin/JAEASort

DUMMY: $(OBJECTS) $(PROG)
	ls

bin/%.o: src/%.cpp
	$(CC) $(CFLAGS) -MMD -MP -o $@ -c $< $(LIBRS)
	
bin/%: programs/%.C $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< -I. $(OBJECTS)  $(LIBRS) 
	chmod +x $@

# bin/Bin2RootSingleThread: programs/Bin2RootSingleThread.C 
# 	$(CC) $(CFLAGS) -o $@ $< $(LIBRS) 
# 	chmod +x $@
# 	
# bin/SinglesBuild: programs/SinglesBuild.C
# 	$(CC) $(CFLAGS) -o $@ $< -I. bin/Bin2RootClassy.o  bin/MakeEventTreeSingle.o  bin/Digitisers.o	  $(LIBRS)	 
# 	chmod +x $@
# bin/SinglesBuildChunkless: programs/SinglesBuildChunkless.C 
# 	$(CC) $(CFLAGS) -o $@ $< -I. bin/Bin2RootClassy.o  bin/MakeEventTreeSingle.o  bin/Digitisers.o  $(LIBRS)   
# 	chmod +x $@

# $(TARG): Sort.cpp $(SHAREDLIB)
# 	$(CC) $(CFLAGS) -o $@ $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS)
# 	chmod +x $@

# $(TARG):  Sort.cpp $(SHAREDLIB) sortcode/CdTeHistList.h sortcode/CdTeSortLoop.h
# 	$(CC) -DCDTE $(CFLAGS) -o  $(TARG) $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS)
# 	chmod +x $(TARG)
# # 	touch Sort.cpp

# debug:  Sort.cpp $(SHAREDLIB)
# 	$(CC) -DDEBUG $(CFLAGS) -o  $(TARG) $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS)
# 	chmod +x $(TARG)
# 	touch Sort.cpp
# 	
# cal:  Sort.cpp $(SHAREDLIB)
# 	$(CC) -DCALIBRATE $(CFLAGS) -o  $(TARG) $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS) -ljroot_phys
# 	chmod +x $(TARG)
# 	touch Sort.cpp
# 	
# cdte:  Sort.cpp $(SHAREDLIB)
# 	$(CC) -DCDTE $(CFLAGS) -o  $(TARG) $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS)
# 	chmod +x $(TARG)
# 	touch Sort.cpp
# 
# $(SHAREDLIB): $(OBJECTS) $(NONHEAD) bin/DictOutput.cxx
# 	$(CC) $(CFLAGS) -o $@ -shared bin/DictOutput.cxx $(OBJECTS) -I. $(ROOT_LIBS) $(ROOT_LIBSEXTRA)
# 	bash bin/MakeExport.sh
# 	
# QuickCal : bin/JAEACal	
# 	
# bin/JAEACal: scripts/QuickCal.cpp $(OBJECTS) $(NONHEAD) bin/DictOutput.cxx
# 	$(CC) $(CFLAGS) -o $@ $< bin/DictOutput.cxx -I. $(OBJECTS) $(LIBRS)
# 	chmod +x $@
# 
# bin/%.o: src/%.cpp include/%.h 
# 	$(CC) $(CFLAGS) -o $@ -c $< $(LIBRS)
# 	
# bin/DictOutput.cxx: $(HEAD)
# 	bash bin/link.sh $(HEAD)
# 	rootcint -f $@ -c -I$(INCLUDE) $(HEAD) bin/Linkdef.h
	
clean:
	rm -f $(LIB)/bin/*.o
	rm -f $(LIB)/bin/*.d
	rm -f $(LIB)/$(PROG)

-include $(DEPS)
