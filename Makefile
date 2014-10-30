###########################################
#Makefile for simple programs
###########################################
INC=
LIB= -lpthread

CC=gcc
CC_FLAG=-Wall -g -DRTSP_DEBUG -DSAVE_FILE_DEBUG

PRG=rtspClient
OBJ=./*.o

$(PRG):$(OBJ)
	$(CC) $(CC_FLAG) $(INC) -o $@ $(OBJ) $(LIB)
	
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CC_FLAG) $(INC) -c $*.c

.PRONY:clean
clean:
	@echo "Removing linked and compiled files......"
	rm -f $(OBJ) $(PRG)
