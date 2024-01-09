#pragma once
#include <string>
#include <queue>
#include "Connection.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <condition_variable>
using namespace std;
//实现了连接池公共模块

class ConnectionPool {
public:
	//获取连接池对象实例
	static ConnectionPool* getConnectionPool();
	//给外部提供接口，从连接池中获取一个可用的空闲连接
	shared_ptr<Connection> getConnection();
private:
	ConnectionPool();	//单例#1 构造函数私有化
	
	//bool loadConfigFile();	//从配置文件中加载配置项
	
	void initCondition();	//初始化相关参数

	void produceConnectionTask();	//运行在独立的线程中，专门负责生产新连接
	
	void scannerConnectionTask();	//扫描超过maxIdleTime时间的空闲连接，进行剩余的连接回收
	
	string _ip;	//mysql的ip地址
	unsigned short _port;	//mysql的端口号 3306
	string _username;	//mysql的登录用户名
	string _password;	//mysql的登录密码
	string _dbname;	//连接的数据库名称
	int _initSize;	//连接池的初始连接量
	int _maxSize;	//连接池的最大连接量
	int _maxIdleTime;	//连接池的最大空闲时间
	int _connectionTimeout;	//连接池获取连接的超时时间

	queue<Connection*> _connectionQue;	//存储mysql连接的队列
	mutex _queueMutex;	//维护连接队列的线程安全互斥锁
	atomic_int _connectionCnt;	//记录连接所创建的connnection连接的总数量
	condition_variable cv;	//设置条件变量，用于连接生产线程和连接消费线程的通信
};