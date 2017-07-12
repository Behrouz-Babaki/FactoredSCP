CC=g++
CPP_FILES := $(wildcard src/*.cpp)
H_FILES := $(wildcard src/*.h)
DEP_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.dep)))

OBJ_FILES = cm_options.o \
	    policy_tree_state.o \
	    constraint_andall.o \
	    brancher_exputil.o
	    
OBJ_LIST = $(addprefix obj/, $(OBJ_FILES))

CFLAGS+= -O3 -g -Wall -Wno-deprecated-declarations -std=c++0x -I./include

ifdef GECODE_HOME
    CFLAGS+= -I $(GECODE_HOME)
	LDFLAGS+= -L $(GECODE_HOME)
endif

LDFLAGS+= -lgecodesearch -lgecodekernel -lgecodesupport -lgecodeint -lgecodefloat -lgecodeminimodel -lgecodegist -lpthread

.PHONY: all clean main 

all: run_knapsack run_book run_inv2
	
run_knapsack: obj/run_knapsack.o obj/knapsack_model.o ${OBJ_LIST}
	g++ ${CFLAGS} -o bin/run_knapsack $^ ${LDFLAGS}
	
run_knapsack_gist: obj/run_knapsack.o obj/knapsack_model_gist.o ${OBJ_LIST}
	g++ ${CFLAGS} -o bin/run_knapsack $^ -lgecodegist ${LDFLAGS}

run_book: obj/run_book.o obj/book_model.o ${OBJ_LIST}
	g++ ${CFLAGS} -o bin/run_book $^ $(LDFLAGS)	

run_book_gist: ${OBJ_LIST} obj/run_book.o obj/book_model_gist.o
	g++ ${CFLAGS} -o bin/run_book_gist $^ -lgecodegist $(LDFLAGS)	
	
run_inv2: obj/run_inv2.o obj/inv2_model.o ${OBJ_LIST}
	g++ ${CFLAGS} -o bin/run_inv2 $^ ${LDFLAGS}

obj/%.o:src/%.cpp
	g++ ${CFLAGS} -o $@ -c $<

obj/%_gist.o:src/%.cpp
	g++ ${CFLAGS} -DUSE_GIST -o $@ -c $<

clean:
	@rm -f obj/* bin/* src/*~ *~ core

include ${DEP_FILES}

obj/%.dep:src/%.cpp
	@set -e; rm -f $@; \
	gcc -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

knapsack: run_knapsack
	@./bin/run_knapsack -a ./data/knapsack/toy_knapsack.net.ac -l ./data/knapsack/toy_knapsack.net.lmap -t 50
	
book: run_book
	@./bin/run_book  -a ./data/book/toy_book.net.ac -l ./data/book/toy_book.net.lmap
	
inv2: run_inv2
	@./bin/run_inv2 -a ./data/inv2/instance_5/inv2.net.ac -l ./data/inv2/instance_5/inv2.net.lmap --depth_and 5 --depth_or 5

fiveish: run_knapsack
	./bin/run_knapsack --acfile ../instance_generator/data/knapsack/instance_74/knapsack.net.ac --lmfile ../instance_generator/data/knapsack/instance_74/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5
	./bin/run_knapsack --acfile ../instance_generator/data/knapsack/instance_102/knapsack.net.ac --lmfile ../instance_generator/data/knapsack/instance_102/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5
	./bin/run_knapsack --acfile ../instance_generator/data/knapsack/instance_60/knapsack.net.ac --lmfile ../instance_generator/data/knapsack/instance_60/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5
	./bin/run_knapsack --acfile ../instance_generator/data/knapsack/instance_88/knapsack.net.ac --lmfile ../instance_generator/data/knapsack/instance_88/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5


