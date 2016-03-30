#ifndef _FILE_DISTRIBUTE_H_
#define _FILE_DISTRIBUTE_H_

#include <list>
#include "config.h"
#include "exception.hpp"

class Reader;
class Writer;

class FileDistribute
{
public:
	FileDistribute();
	virtual ~FileDistribute();

	static bool MoreLog();

public:
	void Init(const std::string& ccm_id, const std::string& str_cfg) throw(Exception);
	void Distribute() throw(Exception);

private:
	void InitProport() throw(Exception);
	bool CheckDir(const std::string& dir, bool need2write);
	void CheckPath(const std::string& path_name, const std::list<std::string>& list_path, bool need2write) throw(Exception);
	void DirWithSlash(std::list<std::string>& list_path);

private:
	static bool            _sbMoreLog;

private:
	bool                   m_bInit;
	Config                 m_cfg;
	int                    m_nWaitSec;
	int                    m_nOnceDistrib;
	int                    m_nInMode;
	int                    m_nOutMode;
	std::list<int>         m_listProport;
	std::list<std::string> m_listSrc;
	std::list<std::string> m_listDes;
	Reader*                m_pReader;
	Writer*                m_pWriter;
};

#endif

