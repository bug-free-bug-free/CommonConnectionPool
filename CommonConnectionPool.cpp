#include "CommonConnecttoinPool.h"
#include <iostream>
#include "public.h"
#include <thread>
#include <functional>

//�̰߳�ȫ���������������ӿ�
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

//�������ļ��м���������
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
//		if (idx == -1)	//��Ч��������
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

//���ӳصĹ���
ConnectionPool::ConnectionPool()
{

	//����������
	/*if (!loadConfigFile())
	{
		return;
	}*/
	//����������
	initCondition();


	//������ʼ����������
	for (int i = 0;i < _initSize;i++)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();	//ˢ��һ�¿�ʼ���е���ʼʱ�� 
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//����һ���µ��̣߳���Ϊ���ӵ�������
	/*thread produce(std::bind( & ConnectionPool::produceConnectionTask, this));*/
	//�������߳̽������޸ģ�ɾ����bind����

	thread produce(&ConnectionPool::produceConnectionTask,this);	
	produce.detach();

	//����һ���µĶ�ʱ�̣߳�ɨ�泬��maxIdleTimeʱ��Ŀ������ӣ����ж�������ӻ���
	//thread scanner(std::bind( & ConnectionPool::scannerConnectionTask, this));
	//ɾ����bind����

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


		//���°汾��ʹ��lambda���ʽ
		cv.wait(lock, [&]()->bool {
			if (_connectionQue.empty())
				return true;
			return false;
		});

		//��������û�дﵽ���ޣ����������µ�����
		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();	//ˢ��һ�¿�ʼ���е���ʼʱ�� 
			_connectionQue.push(p);
			_connectionCnt++; 
		}

		//֪ͨ�������̣߳���������������
		cv.notify_all();
	}
};

//���ⲿ�ṩ�ӿڣ������ӳ��л�ȡһ�����õĿ�������
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
				LOG("��ȡ����ʱ�䳬ʱ��...��ȡ����ʧ���ˣ�");
				return nullptr;
			}
		}
	}

	/*
	shared_ptr����ָ������ʱ�����connection��Դֱ��delete�����൱��
	����connection������������connection�ͱ�close���ˡ�
	������Ҫ����shared_ptr���ͷ���Դ�ķ�ʽ����connectionֱ�ӹ黹��queue����
	*/
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
		//�������ڷ�����Ӧ���߳��е��õģ�����һ��Ҫ���Ƕ��е��̰߳�ȫ����
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAliveTime();	//ˢ��һ�¿�ʼ���е���ʼʱ��
			_connectionQue.push(pcon);
	});	

	_connectionQue.pop();
	//����������֮��֪ͨ�������̼߳��һ�£��������Ϊ���ˣ��Ͻ���������
	cv.notify_all();	
	return sp;
}

void ConnectionPool::scannerConnectionTask()
{
	while(true)
	{
		//ͨ��sleepģ�ⶨʱЧ��
		/*this_thread::sleep_for(chrono::seconds(_maxIdleTime));*/

		//ɨ���������У��ͷŶ��������
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;	//����~Connection()�ͷ�����
			}
			else
			{
				break;	//��ͷ������û�г���_maxIdleTime���������ӿ϶�û��
			}
		}
	}
}
