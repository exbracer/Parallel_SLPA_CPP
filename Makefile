CC=g++
MPICXX=mpicxx

#CFLAGS=-c -Wall
CFLAGS=-c -O3 -g
#CFLAGS= -rdynamic -g -pg -c 
#CFLAGS= -static -g -pg -c 
#CFLAGS= -g -pg -c 


all: main_SLPA MPI_SLPA

#multi-thread
main_SLPA: main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o
	$(CC) main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o -o SLPA -lpthread -g

MPI_SLPA: mpi_main_SLPA.o mpi_classSLPA.o MPI_Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o
	$(MPICXX) mpi_main_SLPA.o mpi_classSLPA.o MPI_Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o -o MPI_SLPA -lpthread -g

#Normal
#main_SLPA: main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o
#	$(CC) main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o -o SLPA -lpthread

#debug
#main_SLPA: main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o
#	$(CC) -g -pg main_SLPA.o classSLPA.o Net.o NODE.o CommonFuns.o fileOpts.o rndNumbers.o -o SLPA


main_SLPA.o: main_SLPA.cpp
	$(CC) $(CFLAGS) main_SLPA.cpp

mpi_main_SLPA.o: mpi_main_SLPA.cpp
	$(MPICXX) $(CFLAGS) mpi_main_SLPA.cpp

classSLPA.o: SLPA.cpp
	$(CC) $(CFLAGS) SLPA.cpp -o classSLPA.o

mpi_classSLPA.o: MPI_SLPA.cpp
	$(MPICXX) $(CFLAGS) MPI_SLPA.cpp -o mpi_classSLPA.o

Net.o: Net.cpp
	$(CC) $(CFLAGS) Net.cpp

MPI_Net.o: MPI_Net.cpp
	$(MPICXX) $(CFLAGS) MPI_Net.cpp

NODE.o: NODE.cpp
	$(CC) $(CFLAGS) NODE.cpp

CommonFuns.o: CommonFuns.cpp
	$(CC) $(CFLAGS) CommonFuns.cpp

fileOpts.o: fileOpts.cpp
	$(CC) $(CFLAGS) fileOpts.cpp

rndNumbers.o: rndNumbers.cpp
	$(CC) $(CFLAGS) rndNumbers.cpp


clean:
	rm -rf *o SLPA
