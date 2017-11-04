#-----------------------------------------#
#       Makefile for graphmanager         #
#             2014-10-14 mine             #
#-----------------------------------------#
CC = g++ 

#FLAG = -g -Wall -std=gnu++11
FLAG = -g -Wall

DFLAG = -D_DEBUG

CFLAG = $(FLAG) $(DFLAG)

INCFLAG = -I./GraphManagerLib/include/ -I./GraphManagerLib/include/graph_manager/ -I./sparkseecpp-5.1.0/includes/sparksee -I./include/ 
LDFLAG = -lgraphmanager -lm -lsparksee -lmysqlclient -L./GraphManagerLib/lib -L./sparkseecpp-5.1.0/lib/linux64/ -L/usr/lib64/mysql/

TARGET = condis
SRCS = main.cpp condis.cpp UpdateDistanceAgent.cpp TcpClient.cpp
OBJS = $(SRCS:%.cpp=%.o)

SRC_DIR = src
OBJ_DIR = obj

RM = rm -f

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAG) -o $@ $(INCFLAG) $(OBJS:%.o=$(OBJ_DIR)/%.o) $(LDFLAG) 

%.o : $(SRC_DIR)/%.cpp
	$(CC) $(CFLAG) -c $< $(INCFLAG) -o $(OBJ_DIR)/$@ $(LDFLAG)

$(OBJS): GraphManagerLib/lib/libgraphmanager.a


clean:
	cd $(OBJ_DIR); $(RM) $(OBJS); cd ../;
	$(RM) $(TARGET)

