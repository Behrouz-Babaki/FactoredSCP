CC=g++
CPP_FILES := $(wildcard src/*.cpp)
DEP_FILES := $(addprefix obj/, $(notdir $(CPP_FILES:.cpp=.dep)))

CFLAGS+= -O3 -g -Wall -Wno-deprecated-declarations -std=c++0x -m64 -I./include
CFLAGS+= -I$(GUROBI_HOME)/include

LDFLAGS+= -lgurobi_c++ -lgurobi65 -lm

# for bn_engine, manually installed xmlrpc-c
CFLAGS+= -I $(HOME)/local/include
LDFLAGS+= -L $(HOME)/local/lib
LDFLAGS+= -lxmlrpc++ -lxmlrpc_client++ -lxmlrpc_util++

.PHONY: all clean facility knapsack finance run_knapsack run_book run_inv2

all: run_knapsack run_book run_inv2

obj/%.o:src/%.cpp
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

facility: obj/facility.o
	$(CC) $^ -o bin/facility $(LDFLAGS)
	
run_book: obj/run_book_ilp.o obj/book_ilp.o obj/cm_options.o
	$(CC) $^ -o bin/run_book $(LDFLAGS)

run_knapsack: obj/run_knapsack_ilp.o obj/knapsack_ilp.o obj/cm_options.o
	$(CC) $^ -o bin/run_knapsack $(LDFLAGS)
	
run_finance : obj/run_finance_ilp.o obj/finance_ilp.o obj/cm_options.o
	$(CC) $^ -o bin/run_finance $(LDFLAGS)	

run_inv2: obj/run_inv2_ilp.o obj/inv2_ilp.o obj/cm_options.o
	$(CC) $^ -o bin/run_inv2 $(LDFLAGS)

clean:
	@rm -f obj/* bin/* src/*~ *~ core

include ${DEP_FILES}

obj/%.dep:src/%.cpp
	@set -e; rm -f $@; \
	gcc -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$
	
finance: run_finance
	@./bin/run_finance -a ./data/financial/instance_1/financial.net.ac \
	-l ./data/financial/instance_1/financial.net.lmap \
	-d ./data/financial/instance_1/financial.data
	
book: run_book
	@./bin/run_book \
	-a ../ground_models/data/book/instance_3/book.net.ac \
	-l ../ground_models/data/book/instance_3/book.net.lmap \
	--timeout 10 --verbose 1

knapsack: run_knapsack
	@./bin/run_knapsack \
	-a ./data/knapsack/instance_1/knapsack.net.ac \
	-l ./data/knapsack/instance_1/knapsack.net.lmap \
	--capacity 50 --timeout 10 --verbose 0
	
inv2: run_inv2
	@./bin/run_inv2 \
	-a ./data/inv2/instance_5/inv2.net.ac \
	-l ./data/inv2/instance_5/inv2.net.lmap \
	--timeout 10 --verbose 1
