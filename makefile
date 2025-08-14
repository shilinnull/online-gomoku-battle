.PHONY:gobang
gobang:gobang.cc util.hpp Test.hpp session.hpp server.hpp room.hpp online.hpp matcher.hpp Logger.hpp db.hpp
	rm -rf gobang
	g++ -g -std=c++11 $^ -o  $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp -lpthread
.PHONY:clean
clean:
	rm -rf gobang