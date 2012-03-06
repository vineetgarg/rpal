#makefile - compile the project 
# @Garg, Vineet
p1: token.h token.cc parser.h parser.cc p1.cc 
	g++ -g token.h token.cc parser.h parser.cc p1.cc -o p1

cl: 
	rm -rf p1

local_test: p1
	perl difftest.pl -1 "../../rpal/rpal -ast -noout FILE" -2 "./p1 -ast FILE" -t tests/

test:
	./difftest.pl -1 "./rpal -ast -noout FILE" -2 "./p1 -ast FILE" -t ~/rpal/tests/
