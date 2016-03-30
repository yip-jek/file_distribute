#ifndef _READER_H_
#define _READER_H_

#include <list>
#include <set>
#include <dirent.h>
#include "exception.hpp"
#include "helper.hpp"
#include "log.h"
#include "file_distribute.h"

class DirNode
{
public:
	DirNode(): m_pDir(NULL), m_pSetChannelID(NULL)
	{}
	~DirNode()
	{
		Close();
	}

public:
	bool Open(const std::string& path)
	{
		Close();

		m_pDir = opendir(path.c_str());
		if ( NULL == m_pDir )
		{
			return false;
		}

		m_strPath = path;
		return true;
	}

	bool Reopen()
	{
		Close();

		m_pDir = opendir(m_strPath.c_str());
		return (m_pDir != NULL);
	}

	bool IsOpen() const
	{
		return (m_pDir != NULL);
	}

	void Close()
	{
		if ( m_pDir != NULL )
		{
			closedir(m_pDir);
			m_pDir = NULL;
		}
	}

	bool ReadFile(std::string& full_file_name)
	{
		dirent* pDirent = NULL;

		while ( (pDirent = readdir(m_pDir)) != NULL )
		{
			if ( '.' == pDirent->d_name[0] )
			{
				continue;
			}

			if ( m_pSetChannelID != NULL )
			{
				full_file_name = pDirent->d_name;

				size_t pos_1 = full_file_name.find('.');
				if ( std::string::npos == pos_1 )
				{
					continue;
				}

				size_t pos_2 = full_file_name.find('.', pos_1+1);
				if ( std::string::npos == pos_2 || ((pos_2 - 1) == pos_1) )
				{
					continue;
				}

				//if ( Helper::Str2Int(full_file_name.substr(pos_1+1, pos_2-pos_1-1)) != m_channelID )
				int channel_id = Helper::Str2Int(full_file_name.substr(pos_1+1, pos_2-pos_1-1));
				if ( m_pSetChannelID->find(channel_id) == m_pSetChannelID->end() )
				{
					continue;
				}
			}

			full_file_name = m_strPath + pDirent->d_name;
			return true;
		}

		return false;
	}

	void GetChannelIDSet(std::set<int>& id_set)
	{
		if ( id_set.empty() )
		{
			return;
		}

		if ( id_set.size() == 1 && (*(id_set.begin()) < 0) )
		{
			m_pSetChannelID = NULL;
		}
		else
		{
			m_pSetChannelID = &id_set;
		}
	}

private:
	std::string 	m_strPath;
	DIR*        	m_pDir;
	std::set<int>*	m_pSetChannelID;
};

// Base reader
class Reader
{
public:
	Reader(): m_nMaxFiles(0)
	{}
	virtual ~Reader() {}

public:
	virtual void SetSourceList(std::list<std::string>& list_src) throw(Exception)
	{
		if ( list_src.empty() )
		{
			throw Exception(RW_SRCLIST_EMPTY, "[READER] Source list is empty!");
		}

		m_pListSrc = &list_src;
	}

	virtual void SetMaxFiles(int max_files)
	{
		m_nMaxFiles = max_files;
	}

	virtual void SetChannelID(std::string id_str)
	{
		Helper::Trim(id_str);
		if ( id_str.empty() )
		{
			throw Exception(RW_CHANNEL_ID_EMPTY, "[READER] Channel_id is empty!");
		}

		std::list<std::string> list_str;
		Helper::SplitStr(id_str, ",", list_str, true);

		m_sChannelID.clear();
		for ( std::list<std::string>::iterator it = list_str.begin(); it != list_str.end(); ++it )
		{
			if ( it->empty() )
			{
				throw Exception(RW_CHANNEL_ID_INVALID, "[READER] Channel_id is invalid: "+id_str);
			}

			m_sChannelID.insert(Helper::Str2Int(*it));
		}

		std::string channel_ids;
		for ( std::set<int>::iterator sit = m_sChannelID.begin(); sit != m_sChannelID.end(); ++sit )
		{
			if ( *sit < 0 && (m_sChannelID.size() > 1) )
			{
				throw Exception(RW_CHANNEL_ID_INVALID, "[READER] Channel_id is invalid: "+id_str);
			}

			if ( sit != m_sChannelID.begin() )
			{
				channel_ids += ", ";
			}
			channel_ids += Helper::Num2Str(*sit);
		}

		if ( m_sChannelID.size() != list_str.size() )
		{
			throw Exception(RW_CHANNEL_ID_TRANS_FAIL, "[READER] Channel_id tranform fail: "+id_str);
		}

		if ( m_sChannelID.empty() || (m_sChannelID.size() == 1 && *(m_sChannelID.begin()) < 0) )
		{
			Log::Instance()->Output("NO SPECIFIC CHANNEL_ID !!!");
		}
		else
		{
			Log::Instance()->Output("SPECIFIC CHANNEL_ID: %s", channel_ids.c_str());
		}
	}

	virtual int GetFiles(std::list<std::string>& list_get) throw(Exception) = 0;

protected:
	std::list<std::string>* m_pListSrc;
	int                     m_nMaxFiles;
	std::set<int>			m_sChannelID;
};

// One_after_one reader 
class OneAfterReader : public Reader
{
public:
	virtual int GetFiles(std::list<std::string>& list_get) throw(Exception)
	{
		int file_count = 0;
		int cur_file_count = 0;
		DirNode dn;
		std::string full_file_name;

		dn.GetChannelIDSet(m_sChannelID);

		for ( std::list<std::string>::const_iterator c_it = m_pListSrc->begin(); c_it != m_pListSrc->end(); ++c_it )
		{
			if ( !dn.Open(c_it->c_str()) )
			{
				throw Exception(RW_OPENDIR_FAIL, "[READER] Open dir fail: "+(*c_it));
			}

			cur_file_count = file_count;
			while ( dn.ReadFile(full_file_name) )
			{
				list_get.push_back(full_file_name);
				++file_count;

				if ( file_count >= m_nMaxFiles )
				{
					if ( FileDistribute::MoreLog() )
					{
						Log::Instance()->Output("[READER] Get dir [%s] file(s): %d", c_it->c_str(), file_count-cur_file_count);
					}
					return file_count;
				}
			}

			if ( FileDistribute::MoreLog() )
			{
				Log::Instance()->Output("[READER] Get dir [%s] file(s): %d", c_it->c_str(), file_count-cur_file_count);
			}
		}

		return file_count;
	}
};

// One_in_one reader
class OneInReader : public Reader
{
public:
	OneInReader(): m_nArrSize(0), m_pDNArr(NULL)
	{}
	~OneInReader()
	{
		ReleaseDN();
	}

public:
	virtual void SetSourceList(std::list<std::string>& list_src) throw(Exception)
	{
		Reader::SetSourceList(list_src);

		ReleaseDN();

		m_nArrSize = list_src.size();
		m_pDNArr = new DirNode[m_nArrSize];

		int index = 0;
		for ( std::list<std::string>::iterator it = list_src.begin(); it != list_src.end(); ++it, ++index )
		{
			if ( !m_pDNArr[index].Open(it->c_str()) )
			{
				throw Exception(RW_OPENDIR_FAIL, "[READER] Open dir fail: "+(*it));
			}

			m_pDNArr[index].GetChannelIDSet(m_sChannelID);
		}
	}

	virtual int GetFiles(std::list<std::string>& list_get) throw(Exception)
	{
		int file_count = 0;
		bool bIsAllDirEmpty = false;
		std::string full_file_name;

		ReopenDN();

		while ( !bIsAllDirEmpty )
		{
			bIsAllDirEmpty = true;

			for ( int dn_index = 0; dn_index != m_nArrSize; ++dn_index )
			{
				if ( m_pDNArr[dn_index].ReadFile(full_file_name) )
				{
					bIsAllDirEmpty = false;

					list_get.push_back(full_file_name);
					++file_count;

					if ( file_count >= m_nMaxFiles )
					{
						return file_count;
					}
				}
			}
		}

		return file_count;
	}

private:
	void ReleaseDN()
	{
		if ( m_pDNArr != NULL )
		{
			delete[] m_pDNArr;
			m_pDNArr = NULL;
		}
	}

	void ReopenDN()
	{
		for ( int index = 0; index < m_nArrSize; ++index )
		{
			m_pDNArr[index].Reopen();
		}
	}

private:
	int       m_nArrSize;
	DirNode*  m_pDNArr;
};

#endif


