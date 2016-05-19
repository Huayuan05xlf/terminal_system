DIR_BIN = bin
DIR_OBJ = obj
DIR_SRC = src
DIR_INC = include
CC = gcc
FLAGS := -I$(DIR_INC) -pthread -Wall
targets = CheckObjDir CheckBinDir $(DIR_BIN)/server $(DIR_BIN)/client
obj_server = $(DIR_OBJ)/menu_server.o $(DIR_OBJ)/online.o \
	$(DIR_OBJ)/user.o $(DIR_OBJ)/my_protocol.o
obj_client = $(DIR_OBJ)/menu_client.o $(DIR_OBJ)/my_protocol.o
.PHONY:all
all:$(targets)
$(DIR_BIN)/server:$(obj_server)
	$(CC) $^ -o $@ $(FLAGS)
$(DIR_BIN)/client:$(obj_client)
	$(CC) $^ -o $@ $(FLAGS)
$(DIR_OBJ)/%.o:$(DIR_SRC)/%.c $(DIR_INC)/%.h
	$(CC) -c $< -o $@ $(FLAGS)
CheckObjDir:
	@if test ! -d $(DIR_OBJ); then mkdir $(DIR_OBJ); fi
CheckBinDir:
	@if test ! -d $(DIR_BIN); then mkdir $(DIR_BIN); fi
.PHONY:clean
clean:
	$(RM) $(DIR_BIN)/* $(DIR_OBJ)/*.o
