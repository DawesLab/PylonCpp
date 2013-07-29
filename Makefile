INC = /opt/PrincetonInstruments/picam/includes

configure: configure.o
	@g++ -lpicam configure.o -o configure

configure.o: configure.cpp
	@g++ -I$(INC) -lpicam -c configure.cpp

#TODO make these all generic:
