CXXFLAGS=-Wall -std=c++11 -g -O3 -I /opt/intel/vtune_amplifier_2018.1.0.535340/include /opt/intel/vtune_amplifier_2018.1.0.535340/lib64/libittnotify.a
#CXXFLAGS=-Wall -std=c++11 -g -pg
#CXXFLAGS=-Wall -std=c++11 -g -pg -DDEBUG
CC=g++

test: test.cpp BEpsilon.h

clean:
	$(RM) *.o test

g++ -Wall -std=c++11 -g -O3 -o test -I /opt/intel/vtune_amplifier_2018.1.0.535340/include /opt/intel/vtune_amplifier_2018.1.0.535340/lib64/libittnotify.a    test.cpp BEpsilon.h

export INTEL_LIBITTNOTIFY64=/opt/intel/vtune_amplifier_2018.1.0.535340/lib64/runtime/libittnotify_collector.so

g++ -Wall -c test.cpp -g -I /opt/intel/vtune_amplifier_2018.1.0.535340/include -o test.o
g++ -g test.o -I /opt/intel/vtune_amplifier_2018.1.0.535340/include -L /opt/intel/vtune_amplifier_2018.1.0.535340/lib64 -l ittnotify -ldl  -o test