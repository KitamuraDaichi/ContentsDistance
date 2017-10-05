#-----------------------------------------#
#       Makefile for graphmanager         #
#             2014-10-14 mine             #
#-----------------------------------------#
CC = g++ 

FLAG = -g -Wall

DFLAG = -D_DEBUG

CFLAG = $(FLAG) $(DFLAG)

INCFLAG = -I./GraphManagerLib/include/ -I./GraphManagerLib/include/graph_manager/ -I./sparkseecpp-5.1.0/includes/sparksee -I./include/ 
#INCFLAG = -I./GraphManagerLib/include/
LDFLAG = -lgraphmanager -lm -lsparksee -lmysqlclient -L./GraphManagerLib/lib -L./sparkseecpp-5.1.0/lib/linux64 -L/usr/lib64/mysql/
#LDFLAG = -L./GraphManagerLib/lib -lgraphmanager -lm 

TARGET = condis
#OBJ = ./obj/main.o
#SRC = ./src/main.cpp
SRCS = main.cpp condis.cpp
OBJS = $(SRCS:%.cpp=%.o)

SRC_DIR = src
OBJ_DIR = obj

RM = rm -f

all: $(TARGET)

#$(TARGET): $(OBJ)
#	$(CC) $(CFLAG) -o $@ $< $(INCFLAG) $(LDFLAG)
$(TARGET): $(OBJS)
	$(CC) $(CFLAG) -o $@ $(INCFLAG) $(LDFLAG) $(OBJS:%.o=$(OBJ_DIR)/%.o)

#$(OBJ): $(SRC)
#	$(CC) $(CFLAG) -c -o $@ $< $(INCFLAG) $(LDFLAG)

$(OBJS): GraphManagerLib/lib/libgraphmanager.a

#%.o : $(SRC_DIR)/%.cpp
#	$(CC) $(CFLAG) -c $< $(INCFLAG) $(LDFLAG) -o $(OBJ_DIR)/$@
%.o : $(SRC_DIR)/%.cpp
	$(CC) $(CFLAG) -c $< $(INCFLAG) $(LDFLAG) -o $(OBJ_DIR)/$@


#clean:
#	$(RM) $(OBJ) $(TARGET)
clean:
	cd $(OBJ_DIR); $(RM) $(OBJS); cd ../;
	$(RM) $(TARGET)

