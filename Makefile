obj_server = obj/menu_server.o obj/online.o obj/user.o obj/my_protocol.o
obj_client = obj/menu_client.o obj/my_protocol.o

all:bin/server bin/client

bin/server:$(obj_server)
	gcc $(obj_server) -o bin/server -Wall -pthread
bin/client:$(obj_client)
	gcc $(obj_client) -o bin/client -Wall -pthread
obj/menu_server.o:src/menu_server.c include/menu_server.h
	gcc -c src/menu_server.c -Iinclude -o obj/menu_server.o -Wall -pthread
obj/online.o:src/online.c include/online.h
	gcc -c src/online.c -Iinclude -o obj/online.o -Wall -pthread
obj/user.o:src/user.c include/user.h
	gcc -c src/user.c -Iinclude -o obj/user.o -Wall
obj/menu_client.o:src/menu_client.c include/menu_client.h
	gcc -c src/menu_client.c -Iinclude -o obj/menu_client.o -Wall -pthread
obj/my_protocol.o:src/my_protocol.c include/my_protocol.h include/my_type.h
	gcc -c src/my_protocol.c -Iinclude -o obj/my_protocol.o -Wall

clean:
	rm -f bin/server bin/client $(obj_server) $(obj_client)