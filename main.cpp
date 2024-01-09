#include <iostream>
#include "Connection.h"
#include <ctime>
#include "CommonConnecttoinPool.h"
#include <thread>
using namespace std;

int main() {

	/*Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "male");
	conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	conn.update(sql);*/
	//基本使用方法


	clock_t begin = clock();
	ConnectionPool* cp = ConnectionPool::getConnectionPool();
	
	thread t1([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0;i < 250;i++)
		{
			shared_ptr<Connection> ptr = cp->getConnection();
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			ptr->update(sql);
		}
		});
	
	thread t2([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0;i < 250;i++)
		{
			shared_ptr<Connection> ptr = cp->getConnection();
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			ptr->update(sql);
		}
		});


	thread t3([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0;i < 250;i++)
		{
			shared_ptr<Connection> ptr = cp->getConnection();
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			ptr->update(sql);
		}
		});

	thread t4([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0;i < 250;i++)
		{
			shared_ptr<Connection> ptr = cp->getConnection();
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			ptr->update(sql);
		}
		});

	clock_t end = clock();
	cout << end - begin << "ms" << endl;
	return 0;
}