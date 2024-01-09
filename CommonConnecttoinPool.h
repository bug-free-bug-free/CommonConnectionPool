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
//ʵ�������ӳع���ģ��

class ConnectionPool {
public:
	//��ȡ���ӳض���ʵ��
	static ConnectionPool* getConnectionPool();
	//���ⲿ�ṩ�ӿڣ������ӳ��л�ȡһ�����õĿ�������
	shared_ptr<Connection> getConnection();
private:
	ConnectionPool();	//����#1 ���캯��˽�л�
	
	//bool loadConfigFile();	//�������ļ��м���������
	
	void initCondition();	//��ʼ����ز���

	void produceConnectionTask();	//�����ڶ������߳��У�ר�Ÿ�������������
	
	void scannerConnectionTask();	//ɨ�賬��maxIdleTimeʱ��Ŀ������ӣ�����ʣ������ӻ���
	
	string _ip;	//mysql��ip��ַ
	unsigned short _port;	//mysql�Ķ˿ں� 3306
	string _username;	//mysql�ĵ�¼�û���
	string _password;	//mysql�ĵ�¼����
	string _dbname;	//���ӵ����ݿ�����
	int _initSize;	//���ӳصĳ�ʼ������
	int _maxSize;	//���ӳص����������
	int _maxIdleTime;	//���ӳص�������ʱ��
	int _connectionTimeout;	//���ӳػ�ȡ���ӵĳ�ʱʱ��

	queue<Connection*> _connectionQue;	//�洢mysql���ӵĶ���
	mutex _queueMutex;	//ά�����Ӷ��е��̰߳�ȫ������
	atomic_int _connectionCnt;	//��¼������������connnection���ӵ�������
	condition_variable cv;	//���������������������������̺߳����������̵߳�ͨ��
};