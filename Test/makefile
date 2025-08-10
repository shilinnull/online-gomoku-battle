mysql:mysql.c
	gcc $^ -o $@ -L/usr/lib64/mysql/ -lmysqlclient
json:json.cc
	g++ -std=c++11 $^ -o $@ -lpthread -lboost_system -ljsoncpp
wsserver:wsserver.cc
	g++ -std=c++11 $^ -o $@ -lpthread -lboost_system

.PHONY:clean
clean:
	rm -rf mysql json wsserver