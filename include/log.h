#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <fstream>
#include "exception.hpp"

class Log
{
private:
	Log();
	~Log();

public:
	static Log* Instance();
	static void Release();
	static bool SetCCMID(int ccm_id);
	static bool ResetFileSize(unsigned long fsize);

public:
	void Init() throw(Exception);
	void SetPath(const std::string& path) throw(Exception);
	bool Output(const char* format, ...);

private:
	void TryCloseFileLogger();
	void OpenNewLogger() throw(Exception);

private:
	static Log*          _spLogger;
	static int           _sLogCcmID;
	static unsigned long _sMaxLogFileSize;

private:
	std::string    m_sLogPath;
	std::fstream   m_fsLogger;
	unsigned long  m_sCurrentFileSize;
};

#endif 

