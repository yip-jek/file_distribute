#include "file_distribute.h"
#include <dirent.h>
#include <unistd.h>
#include "def.h"
#include "log.h"
#include "helper.hpp"
#include "gsignal.h"
#include "reader.hpp"
#include "writer.hpp"

bool FileDistribute::_sbMoreLog = false;

FileDistribute::FileDistribute()
:m_bInit(false)
,m_nWaitSec(0)
,m_nOnceDistrib(0)
,m_nInMode(0)
,m_nOutMode(0)
,m_pReader(NULL)
,m_pWriter(NULL)
{
}

FileDistribute::~FileDistribute()
{
	if ( m_pReader != NULL )
	{
		delete m_pReader;
		m_pReader = NULL;
	}

	if ( m_pWriter != NULL )
	{
		delete m_pWriter;
		m_pWriter = NULL;
	}
}

bool FileDistribute::MoreLog()
{
	return _sbMoreLog;
}

void FileDistribute::Init(const std::string& ccm_id, const std::string& str_cfg) throw(Exception)
{
	if ( m_bInit )
	{
		Log::Instance()->Output("Already Initialized!");
		return;
	}

	if ( !Log::SetCCMID(Helper::Str2Int(ccm_id)) )
	{
		throw Exception(LE_CCMID_INVALID, "[INIT] CCM_ID is invalid!");
	}

	if ( !m_cfg.SetCfgFile(str_cfg) )
	{
		throw Exception(FD_INVALID_CFG_FILE, "[INIT] Set configuration file fail!");
	}

	m_cfg.RegisterItem("LOG_PATH", "SYS");
	m_cfg.RegisterItem("MORE_LOG", "SYS");
	m_cfg.RegisterItem("WAIT_SEC", "SYS");
	m_cfg.RegisterItem("ONCE_DISTRIB", "SYS");

	m_cfg.RegisterItem("CHANNEL_ID", "COMMON");
	m_cfg.RegisterItem("IN_MODE", "COMMON");
	m_cfg.RegisterItem("OUT_MODE", "COMMON");
	m_cfg.RegisterItem("PROPORTION", "COMMON");
	m_cfg.RegisterItem("SRC_PATH", "COMMON");
	m_cfg.RegisterItem("DES_PATH", "COMMON");

	if ( !m_cfg.ReadConfig() )
	{
		throw Exception(FD_READ_CFG_FAIL, "[INIT] Read configuration file fail!");
	}

	Log::Instance()->SetPath(m_cfg.GetCfgValue("LOG_PATH", "SYS"));
	Log::Instance()->Init();

	if ( !GSignal::Init() )
	{
		throw Exception(FD_GSIGNAL_INIT_FAIL, "[INIT] [ERROR] GSignal init fail!");
	}

	_sbMoreLog = m_cfg.GetCfgBoolVal("MORE_LOG", "SYS");

	m_nWaitSec = m_cfg.GetCfgIntVal("WAIT_SEC", "SYS");
	if ( m_nWaitSec <= 0 )
	{
		throw Exception(FD_WAIT_SEC_INVALID, "[INIT] The wait_sec invalid!");
	}

	m_nOnceDistrib = m_cfg.GetCfgIntVal("ONCE_DISTRIB", "SYS");
	if ( m_nOnceDistrib <= 0 )
	{
		throw Exception(FD_ONCE_DISTRIB_INVALID, "[INIT] The once_distrib invalid!");
	}

	Helper::SplitStr(m_cfg.GetCfgValue("SRC_PATH", "COMMON"), ",", m_listSrc, true);
	CheckPath("SRC_PATH", m_listSrc, false);
	DirWithSlash(m_listSrc);

	m_nInMode = m_cfg.GetCfgIntVal("IN_MODE", "COMMON");
	if ( 1 == m_nInMode )			// One_after_one in mode
	{
		Log::Instance()->Output("IN MODE: 1 - ONE_AFTER_ONE");
		m_pReader = new OneAfterReader();
	}
	else if ( 2 == m_nInMode )		// One_in_one in mode
	{
		Log::Instance()->Output("IN MODE: 2 - ONE_IN_ONE");
		m_pReader = new OneInReader();
	}
	else		// Error
	{
		throw Exception(FD_IN_MODE_INVALID, "[INIT] Invalid mode: "+Helper::Num2Str(m_nInMode));
	}
	m_pReader->SetChannelID(m_cfg.GetCfgValue("CHANNEL_ID", "COMMON"));
	m_pReader->SetSourceList(m_listSrc);
	m_pReader->SetMaxFiles(m_nOnceDistrib);

	Helper::SplitStr(m_cfg.GetCfgValue("DES_PATH", "COMMON"), ",", m_listDes, true);
	CheckPath("DES_PATH", m_listDes, true);
	DirWithSlash(m_listDes);

	m_nOutMode = m_cfg.GetCfgIntVal("OUT_MODE", "COMMON");
	if ( 1 == m_nOutMode )			// Normal out mode
	{
		Log::Instance()->Output("OUT MODE: 1 - NORMAL");
		m_pWriter = new NormalWriter();
	}
	else if ( 2 == m_nOutMode )		// Proportion out mode
	{
		Log::Instance()->Output("OUT MODE: 2 - PROPORTION");
		InitProport();
		m_pWriter = new ProportionWriter();
	}
	else if ( 3 == m_nOutMode )
	{
		Log::Instance()->Output("OUT MODE: 3 - LIMITED");
		InitProport();
		m_pWriter = new LimitedWriter();
	}
	else		// Error
	{
		throw Exception(FD_OUT_MODE_INVALID, "[INIT] Invalid mode: "+Helper::Num2Str(m_nOutMode));
	}
	m_pWriter->SetProportion(&m_listProport);
	m_pWriter->SetDestination(m_listDes);

	Log::Instance()->Output("Initialize successfully!");
	m_bInit = true;
}

void FileDistribute::Distribute() throw(Exception)
{
	if ( !m_bInit )
	{
		throw Exception(FD_NOT_INITIALIZED, "[DISTRIB] Not initialized!");
	}

	int get_files = 0;
	std::list<std::string> list_files;
	while ( GSignal::IsRunning() )
	{
		if ( (get_files = m_pReader->GetFiles(list_files)) > 0 )
		{
			Log::Instance()->Output("[DISTRIB] Total get files: %d", get_files);

			m_pWriter->MoveFiles(list_files);
		}

		sleep(m_nWaitSec);
	}
}

void FileDistribute::InitProport() throw(Exception)
{
	std::string prop_val = m_cfg.GetCfgValue("PROPORTION", "COMMON");

	std::list<std::string> l_prop;
	if ( Helper::SplitStr(prop_val, ":", l_prop, true) != m_listDes.size() )
	{
		throw Exception(FD_PROP_SIZE_NOMATCH, "[INIT] The proportion size is not match with des_dir size!");
	}

	int counter = 0;
	for ( std::list<std::string>::iterator it = l_prop.begin(); it != l_prop.end(); ++it )
	{
		++counter;
		if ( Helper::IsAllNumber(*it) )
		{
			m_listProport.push_back(Helper::Str2Int(*it));
		}
		else
		{
			throw Exception(FD_PROP_UNKNOWN_VALUE, "[INIT] The proportion "+Helper::Num2Str(counter)+" unknown value: "+*it);
		}
	}

	for ( std::list<int>::iterator iti = m_listProport.begin(); iti != m_listProport.end(); ++iti )
	{
		if ( m_listProport.begin() == iti )
		{
			prop_val = Helper::Num2Str(*iti);
		}
		else
		{
			prop_val += " : " + Helper::Num2Str(*iti);
		}
	}
	Log::Instance()->Output("PROPORTION = %s", prop_val.c_str());
}

bool FileDistribute::CheckDir(const std::string& dir, bool need2write)
{
	DIR* p_dir = opendir(dir.c_str());
	if ( NULL == p_dir )
	{
		if ( !need2write )
		{
			return false;
		}
		else
		{
			if ( mkdir(dir.c_str(), 777) != 0 )
			{
				return false;
			}
		}
	}
	closedir(p_dir);

	return (access(dir.c_str(), (need2write?W_OK:R_OK)) == 0);
}

void FileDistribute::CheckPath(const std::string& path_name, const std::list<std::string>& list_path, bool need2write) throw(Exception)
{
	if ( list_path.empty() )
	{
		throw Exception(FD_NO_PATH, "[INIT] Check: NO "+path_name+" (size: 0) !");
	}

	Log::Instance()->Output("%s size: %d", path_name.c_str(), list_path.size());

	int counter = 0;
	for ( std::list<std::string>::const_iterator c_it = list_path.begin(); c_it != list_path.end(); ++c_it )
	{
		++counter;
		if ( c_it->empty() )
		{
			throw Exception(FD_PATH_EMPTY, "[INIT] Check: ["+path_name+"\x20"+Helper::Num2Str(counter)+"] is empty!");
		}

		if ( CheckDir(*c_it, need2write) )
		{
			Log::Instance()->Output("CHECK [(%c)%s %d> %s] OK!", (need2write?'W':'R'), path_name.c_str(), counter, c_it->c_str());
		}
		else
		{
			throw Exception(FD_PATH_INVALID, "[INIT] Check: ["+path_name+"\x20"+Helper::Num2Str(counter)+"> "+*c_it+"] is invalid: not existed or permission denied!");
		}
	}
}

void FileDistribute::DirWithSlash(std::list<std::string>& list_path)
{
	for ( std::list<std::string>::iterator it = list_path.begin(); it != list_path.end(); ++it )
	{
		const char LAST_CH = (*it)[it->size()-1];
		if ( LAST_CH != '/' )
		{
			it->append("/");
		}
	}
}

