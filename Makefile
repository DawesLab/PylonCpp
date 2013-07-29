INC = /opt/PrincetonInstruments/picam/includes

configure: configure.o
	@g++ -lpicam configure.o -o configure

configure.o: configure.cpp
	@g++ -I$(INC) -lpicam -c configure.cpp

save_data: save_data.o
	@g++ -lpicam save_data.o -o save_data

save_data.o: save_data.cpp
	@g++ -I$(INC) -lpicam -c save_data.cpp
