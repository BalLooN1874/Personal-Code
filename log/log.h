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
		google::SetStderrLogging( google::GLOG_INFO ); //设置级别高于 google::INFO 的日志同时输出到屏幕
#else
		google::SetStderrLogging( google::GLOG_WARNING );//设置级别高于 google::FATAL 的日志同时输出到屏幕
#endif
		FLAGS_colorlogtostderr = true; //设置输出到屏幕的日志显示相应颜色
		std::string str = LOG_PATH + "\\log_fatal_";
		google::SetLogDestination( google::GLOG_FATAL , str.c_str() ); // 设置 google::FATAL 级别的日志存储路径和文件名前缀
		str = LOG_PATH + "\\log_error_";
		google::SetLogDestination( google::GLOG_ERROR , str.c_str() ); //设置 google::ERROR 级别的日志存储路径和文件名前缀
		str = LOG_PATH + "\\log_warning_";
		google::SetLogDestination( google::GLOG_WARNING , str.c_str() ); //设置 google::WARNING 级别的日志存储路径和文件名前缀
		str = LOG_PATH + "\\log_info_";
		google::SetLogDestination( google::GLOG_INFO , str.c_str() ); //设置 google::INFO 级别的日志存储路径和文件名前缀
		FLAGS_logbufsecs = 0; //缓冲日志输出，默认为30秒，此处改为立即输出
		FLAGS_max_log_size = 512; //最大日志大小为 512MB
		FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出
		//google::SetLogFilenameExtension("91_"); //设置文件名扩展，如平台？或其它需要区分的信息
		//google::InstallFailureSignalHandler(); //捕捉 core dumped (linux)
		//google::InstallFailureWriter(&Log); //默认捕捉 SIGSEGV 信号信息输出会输出到 stderr (linux)
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