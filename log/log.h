#pragma once
#include <string.h>
#include "glog\logging.h"
const std::string  LOG_PATH = "..\\LOG_FILE";
class Log
{
public:
	Log(const char* pExeName)
	{
		google::InitGoogleLogging( pExeName );
#ifdef _DEBUG
		google::SetStderrLogging( google::GLOG_INFO ); //���ü������ google::INFO ����־ͬʱ�������Ļ
#else
		google::SetStderrLogging( google::GLOG_WARNING );//���ü������ google::FATAL ����־ͬʱ�������Ļ
#endif
		FLAGS_colorlogtostderr = true; //�����������Ļ����־��ʾ��Ӧ��ɫ
		std::string str = LOG_PATH + "\\log_fatal_";
		google::SetLogDestination( google::GLOG_FATAL , str.c_str() ); // ���� google::FATAL �������־�洢·�����ļ���ǰ׺
		str = LOG_PATH + "\\log_error_";
		google::SetLogDestination( google::GLOG_ERROR , str.c_str() ); //���� google::ERROR �������־�洢·�����ļ���ǰ׺
		str = LOG_PATH + "\\log_warning_";
		google::SetLogDestination( google::GLOG_WARNING , str.c_str() ); //���� google::WARNING �������־�洢·�����ļ���ǰ׺
		str = LOG_PATH + "\\log_info_";
		google::SetLogDestination( google::GLOG_INFO , str.c_str() ); //���� google::INFO �������־�洢·�����ļ���ǰ׺
		FLAGS_logbufsecs = 0; //������־�����Ĭ��Ϊ30�룬�˴���Ϊ�������
		FLAGS_max_log_size = 512; //�����־��СΪ 512MB
		FLAGS_stop_logging_if_full_disk = true; //�����̱�д��ʱ��ֹͣ��־���
		//google::SetLogFilenameExtension("91_"); //�����ļ�����չ����ƽ̨����������Ҫ���ֵ���Ϣ
		//google::InstallFailureSignalHandler(); //��׽ core dumped (linux)
		//google::InstallFailureWriter(&Log); //Ĭ�ϲ�׽ SIGSEGV �ź���Ϣ���������� stderr (linux)
	}
	~Log()
	{
		google::ShutdownGoogleLogging();
	}
private:
	Log();
	Log( const Log& ) = delete;
	Log& operator=( const Log& ) = delete;
	Log& operator=( Log&& ) = delete;
};