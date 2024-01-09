#include "CommonConnecttoinPool.h"
#include <iostream>
#include "public.h"
#include <thread>
#include <functional>

//线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

//从配置文件中加载配置项
//bool ConnectionPool::loadConfigFile()
//{
//	FILE* pf = fopen("mysql.ini", "r");
//	if (pf == nullptr)
//	{
//		LOG("mysql.ini file is not exist!");
//		return false;
//	}
//
//	while (!feof(pf))
//	{
//		char line[1024] = { 0 };
//		fgets(line, 1024, pf);
//		string str = line;
//		int idx = str.find('=', 0);
//		if (idx == -1)	//无效的配置项
//		{
//			continue;
//		}
//		//password=123456\n
//		int endidx = str.find('\n', idx);
//		string key = str.substr(0, idx);
//		string value = str.substr(idx + 1, endidx - idx - 1);
//
//		if (key == "ip")
//		{
//			_ip = value;
//		}
//		else if (key =="port")
//		{
//			_port = atoi(value.c_str());
//		}
//		else if (key == "username")
//		{
//			_username = value;
//		}
//		else if (key == "password")
//		{
//			_password = value;
//		}
//		else if (key == "dbname")
//		{
//			_dbname = value;
//		}
//		else if (key == "initSize")
//		{
//			_initSize = atoi(value.c_str());
//		}
//		else if (key == "maxSize")
//		{
//			_maxSize= atoi(value.c_str());
//		}
//		else if (key == "maxIdleTime")
//		{
//			_maxIdleTime= atoi(value.c_str());
//		}
//		else if (key == "connectionTimeOut")
//		{
//			_connectionTimeout = atoi(value.c_str());
//		}
//	}
//	return true;
//}

void ConnectionPool::initCondition()
{
	_ip = "127.0.0.1";
	_port = 3306;
	_username = "root";
	_password = "123456";
	_dbname = "chat";
	_initSize = 10;
	_maxSize = 1024;
	_maxIdleTime = 60;
	_connectionTimeout = 100;
}

//连接池的构造
ConnectionPool::ConnectionPool()
{

	//加载配置项
	/*if (!loadConfigFile())
	{
		return;
	}*/
	//加载配置项
	initCondition();


	//创建初始数量的连接
	for (int i = 0;i < _initSize;i++)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();	//刷新一下开始空闲的起始时间 
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//启动一个新的线程，作为连接的生产者
	/*thread produce(std::bind( & ConnectionPool::produceConnectionTask, this));*/
	//这两个线程进行了修改，删除了bind绑定器

	thread produce(&ConnectionPool::produceConnectionTask,this);	
	produce.detach();

	//启动一个新的定时线程，扫面超过maxIdleTime时间的空闲连接，进行多余的连接回收
	//thread scanner(std::bind( & ConnectionPool::scannerConnectionTask, this));
	//删除了bind绑定器

	thread scanner(&ConnectionPool::scannerConnectionTask,this);
	scanner.detach();
}

void ConnectionPool::produceConnectionTask() 
{
	while(true)
	{
		unique_lock<mutex> lock(_queueMutex);
		/*while (!_connectionQue.empty())
		{
			cv.wait(lock);
		}*/


		//更新版本，使用lambda表达式
		cv.wait(lock, [&]()->bool {
			if (_connectionQue.empty())
				return true;
			return false;
		});

		//连接数量没有达到上限，继续创建新的连接
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();	//刷新一下开始空闲的起始时间 
			_connectionQue.push(p);
			_connectionCnt++; 
		}

		//通知消费者线程，可以消费连接了
		cv.notify_all();
	}
};

//给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	if (_connectionQue.empty())
	{
		//sleep
		
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取空闲时间超时了...获取连接失败了！");
				return nullptr;
			}
		}
	}

	/*
	shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
	调用connection的析构函数，connection就被close掉了。
	这里需要定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
	*/
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
		//这里是在服务区应用线程中调用的，所以一定要考虑队列的线程安全操作
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAliveTime();	//刷新一下开始空闲的起始时间
			_connectionQue.push(pcon);
	});	

	_connectionQue.pop();
	//消费完连接之后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
	cv.notify_all();	
	return sp;
}

void ConnectionPool::scannerConnectionTask()
{
	while(true)
	{
		//通过sleep模拟定时效果
		/*this_thread::sleep_for(chrono::seconds(_maxIdleTime));*/

		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;	//调用~Connection()释放连接
			}
			else
			{
				break;	//队头的连接没有超过_maxIdleTime，其他连接肯定没有
			}
		}
	}
}
