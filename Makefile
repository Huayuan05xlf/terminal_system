CC = gcc
FLAGS := -Iinclude -pthread -Wall
targets = bin/server bin/client
obj_server = obj/menu_server.o obj/online.o obj/user.o obj/my_protocol.o
obj_client = obj/menu_client.o obj/my_protocol.o
.PHONY:all
all:$(targets)
bin/server:$(obj_server)
	$(CC) $^ -o $@ $(FLAGS)
bin/client:$(obj_client)
	$(CC) $^ -o $@ $(FLAGS)
obj/%.o:src/%.c include/%.h
	$(CC) -c $< -o $@ $(FLAGS)
.PHONY:clean
clean:
	$(RM) $(targets) obj/*.o
