all: bb_csvimport

bb_csvimport: DBAccess.o FileIO.o lib.o CrayAccess.o config.o
	g++ DBAccess.o FileIO.o lib.o CrayAccess.o config.o -o bb_csvimport  -Wno-write-strings -I./include -L./lib -std=c++11 main.cpp -lpq 

config.o: config.cpp
	g++ -c -std=c++11 config.cpp

lib.o: lib.cpp
	g++ -c -std=c++11 lib.cpp

FileIO.o: FileIO.cpp
	g++ -c -std=c++11 FileIO.cpp

CrayAccess.o: CrayAccess.cpp
	g++ -c -std=c++11 CrayAccess.cpp

DBAccess.o: DBAccess.cpp
	g++ -c -std=c++11 -I/usr/local/pgsql/include -L/usr/local/pgsql/lib DBAccess.cpp -lpq

clean:
	rm *.o bb_csvimport
