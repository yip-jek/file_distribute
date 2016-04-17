#ifndef _WRITER_H_
#define _WRITER_H_

#include <list>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "exception.hpp"
#include "log.h"
#include "file_distribute.h"

class WriterRecord
{
public:
	WriterRecord(): m_nCurr(0), m_nMaxSize(0), m_pDes(NULL), m_pMove(NULL), m_pMoveFail(NULL)
	{}
	~WriterRecord()
	{
		Clear();
	}

public:
	void Set(const std::list<std::string>& list_des)
	{
		Clear();

		m_nMaxSize = list_des.size();
		m_pDes = new std::string[m_nMaxSize];
		m_pMove = new int[m_nMaxSize];
		m_pMoveFail = new int[m_nMaxSize];

		int index = 0;
		for ( std::list<std::string>::const_iterator c_it = list_des.begin(); c_it != list_des.end(); ++c_it )
		{
			m_pDes[index] = *c_it;

			m_pMove[index] = 0;
			m_pMoveFail[index] = 0;

			++index;
		}
	}

	void Reset()
	{
		m_nCurr = 0;

		for ( int index = 0; index < m_nMaxSize; ++index )
		{
			m_pMove[index] = 0;
			m_pMoveFail[index] = 0;
		}
	}

	std::string& GetCurrDes() const
	{
		return m_pDes[m_nCurr];
	}

	std::string& GetDes(int index) const
	{
		return m_pDes[index];
	}

	int GetCurrIndex() const
	{
		return m_nCurr;
	}

	int GetMaxSize() const
	{
		return m_nMaxSize;
	}

	int GetMove(int index) const
	{
		return m_pMove[index];
	}

	int GetMoveFail(int index) const
	{
		return m_pMoveFail[index];
	}

	void MoveUp()
	{
		++m_pMove[m_nCurr];
	}

	void MoveFailUp()
	{
		++m_pMoveFail[m_nCurr];
	}

	void Next()
	{
		++m_nCurr;

		if ( m_nCurr >= m_nMaxSize )
		{
			m_nCurr = 0;
		}
	}

private:
	void Clear()
	{
		m_nCurr = 0;
		m_nMaxSize = 0;

		if ( m_pDes != NULL )
		{
			delete[] m_pDes;
			m_pDes = NULL;
		}

		if ( m_pMove != NULL )
		{
			delete[] m_pMove;
			m_pMove = NULL;
		}

		if ( m_pMoveFail != NULL )
		{
			delete[] m_pMoveFail;
			m_pMoveFail = NULL;
		}
	}

private:
	int          m_nCurr;
	int          m_nMaxSize;
	std::string* m_pDes;
	int*         m_pMove;
	int*         m_pMoveFail;
};

// Base writer
class Writer
{
public:
	Writer()
	{
		memset(m_strBuf, 0, sizeof(m_strBuf));
	}
	virtual ~Writer() {}

public:
	static int CountDirFiles(const std::string& dir)
	{
		const std::string WORK_DIR = dir + "work/";
		DIR* pDir = opendir(WORK_DIR.c_str());
		if ( NULL == pDir )
		{
			return -1;
		}

		int file_count = 0;
		dirent* pDirent = NULL;
		std::string path_name;
		struct stat fst;
		while ( (pDirent = readdir(pDir)) != NULL )
		{
			// Skip prefix with '.'
			if ( '.' == pDirent->d_name[0] )
			{
				continue;
			}

			// Skip sub-dir
			path_name = WORK_DIR + pDirent->d_name;
			if ( stat(path_name.c_str(), &fst) != 0 )
			{
				return -2;
			}
			if ( S_ISDIR(fst.st_mode) )
			{
				continue;
			}

			++file_count;
		}
		closedir(pDir);

		return file_count;
	}

public:
	virtual void SetDestination(const std::list<std::string>& list_des) throw(Exception)
	{
		if ( list_des.empty() )
		{
			throw Exception(RW_NO_DESTINATION, "[WRITER] NO Destination!");
		}

		// Create sub-dir: ./work/ and ./commit/
		for ( std::list<std::string>::const_iterator c_it = list_des.begin(); c_it != list_des.end(); ++c_it )
		{
			TryCreateDir(*c_it+"work/");
			TryCreateDir(*c_it+"commit/");
		}

		m_rec.Set(list_des);
	}

	virtual void SetProportion(std::list<int>* pListProp) throw(Exception) {}

	virtual void MoveFiles(std::list<std::string>& list_get) throw(Exception) = 0;

protected:
	virtual bool Move(const std::string& full_file_name, const std::string& des_path)
	{
		// Read link file
		std::string old_full_name;
		memset(m_strBuf, 0, sizeof(m_strBuf));
		if ( readlink(full_file_name.c_str(), m_strBuf, sizeof(m_strBuf)) > 0 )
		{
			unlink(full_file_name.c_str());

			old_full_name = m_strBuf;
		}
		else
		{
			old_full_name = full_file_name;
		}

		std::size_t pos = old_full_name.rfind('/');
		std::string file_name = (pos != std::string::npos) ? old_full_name.substr(pos+1) : old_full_name;
		std::string work_file = des_path + "work/" + file_name;

		if ( rename(old_full_name.c_str(), work_file.c_str()) < 0 )
		{
			Log::Instance()->Output("[WRITER] <ERROR> Move file [%s] to [%s] fail! ERROR:%s", old_full_name.c_str(), work_file.c_str(), strerror(errno));
			return false;
		}

		std::string link_file = des_path + "commit/" + file_name;

		unlink(link_file.c_str());
		if ( symlink(work_file.c_str(), link_file.c_str()) != 0 )
		{
			Log::Instance()->Output("[WRITER] <ERROR> Link file [%s] fail! ERROR:%s", link_file.c_str(), strerror(errno));
			return false;
		}

		if ( FileDistribute::MoreLog() )
		{
			Log::Instance()->Output("[WRITER] Move file: [%s] -> [%s]", old_full_name.c_str(), work_file.c_str());
		}
		return true;
	}

	void TryCreateDir(const std::string& dir_path) throw(Exception)
	{
		// Not existed ?
		if ( access(dir_path.c_str(), F_OK) != 0 )
		{
			if ( mkdir(dir_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 )
			{
				throw Exception(RW_CREATE_DIR_FAIL, "Try to create dir "+dir_path+" fail! "+std::string(strerror(errno)));
			}
		}
	}

protected:
	WriterRecord  m_rec;

private:
	char m_strBuf[1024];
};

// Normal writer
class NormalWriter : public Writer
{
public:
	virtual void MoveFiles(std::list<std::string>& list_get) throw(Exception)
	{
		m_rec.Reset();

		while ( !list_get.empty() )
		{
			if ( Move(list_get.front(), m_rec.GetCurrDes()) )
			{
				m_rec.MoveUp();
			}
			else
			{
				m_rec.MoveFailUp();
			}

			list_get.pop_front();

			m_rec.Next();
		}

		const int MAX_REC_SIZE = m_rec.GetMaxSize();
		for ( int i = 0; i < MAX_REC_SIZE; ++i )
		{
			Log::Instance()->Output("[WRITER] Move to [%s] file(s): %d, fail: %d", m_rec.GetDes(i).c_str(), m_rec.GetMove(i), m_rec.GetMoveFail(i));
		}
	}
};

// Proportion writer
class ProportionWriter : public Writer
{
public:
	virtual void SetProportion(std::list<int>* pListProp) throw(Exception)
	{
		m_pListProport = pListProp;

		if ( NULL == m_pListProport )
		{
			throw Exception(RW_PROPORTION_UNSET, "[WRITER] Proportion list unset!");
		}
	}

	virtual void MoveFiles(std::list<std::string>& list_get) throw(Exception)
	{
		m_rec.Reset();

		int file_count = 0;
		std::list<int>::iterator it = m_pListProport->begin();

		while ( !list_get.empty() )
		{
			if ( Move(list_get.front(), m_rec.GetCurrDes()) )
			{
				m_rec.MoveUp();
			}
			else
			{
				m_rec.MoveFailUp();
			}

			list_get.pop_front();

			if ( ++file_count >= *it )
			{
				file_count = 0;
				++it;

				if ( m_pListProport->end() == it )
				{
					it = m_pListProport->begin();
				}

				m_rec.Next();
			}
		}

		const int MAX_REC_SIZE = m_rec.GetMaxSize();
		for ( int i = 0; i < MAX_REC_SIZE; ++i )
		{
			Log::Instance()->Output("[WRITER] Move to [%s] file(s): %d, fail: %d", m_rec.GetDes(i).c_str(), m_rec.GetMove(i), m_rec.GetMoveFail(i));
		}
	}

private:
	std::list<int>* m_pListProport;
};

// Limited writer : only move file, when num of files less than the [VALUE]
class LimitedWriter : public Writer
{
public:
	virtual void SetProportion(std::list<int>* pListProp) throw(Exception)
	{
		m_pListLimited = pListProp;

		if ( NULL == m_pListLimited )
		{
			throw Exception(RW_LIMITED_UNSET, "[WRITER] Limited list unset!");
		}
	}

	virtual void MoveFiles(std::list<std::string>& list_get) throw(Exception)
	{
		m_rec.Reset();

		int desFileCount = 0;
		int file_left = 0;
		int file_count = 0;
		int move_up = 0;
		int move_fail = 0;

		std::list<int>::iterator it = m_pListLimited->begin();
		const int MAX_REC_SIZE = m_rec.GetMaxSize();
		for ( int index = 0; index < MAX_REC_SIZE; ++index, ++it )
		{
			desFileCount = CountDirFiles(m_rec.GetDes(index));

			if ( desFileCount >= *it )
			{
				Log::Instance()->Output("[WRITER] DES_PATH: [%s], file_count=[%d] >= its limit: [%d]", m_rec.GetDes(index).c_str(), desFileCount, *it);
			}
			else
			{
				file_left = *it - desFileCount;
				file_count = 0;
				move_up = 0;
				move_fail = 0;

				while ( !list_get.empty() )
				{
					if ( Move(list_get.front(), m_rec.GetDes(index)) )
					{
						++move_up;
					}
					else
					{
						++move_fail;
					}

					list_get.pop_front();

					if ( ++file_count >= file_left )
					{
						break;
					}
				}

				Log::Instance()->Output("[WRITER] Move to [%s] file(s): %d, fail: %d", m_rec.GetDes(index).c_str(), move_up, move_fail);

				// The getter list is empty! -- END
				if ( list_get.empty() )
				{
					return;
				}
			}
		}

		// Clear the getter list
		if ( !list_get.empty() )
		{
			list_get.clear();
		}
	}

private:
	std::list<int>* m_pListLimited;
};

#endif

