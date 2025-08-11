.PHONY:gobang
gobang:gobang.cc
	g++ -g -std=c++11 $^ -o  $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp -lpthread
.PHONY:clean
clean:
	rm -rf gobang