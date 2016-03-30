#ifndef _HELPER_H_
#define _HELPER_H_

#include <string>
#include <ctype.h>
#include <stdlib.h>
#include <list>

class Helper
{
public:
	static void Trim(std::string& str)
	{
		if ( !str.empty() )
		{
			std::string white_spaces(" \t\f\v\n\r");
			std::size_t first_pos = str.find_first_not_of(white_spaces);
			if ( std::string::npos == first_pos )
			{
				str.clear();
			}
			else
			{
				std::size_t last_pos = str.find_last_not_of(white_spaces);
				str = str.substr(first_pos, last_pos-first_pos+1);
			}
		}
	}

	static void Upper(std::string& str)
	{
		if ( !str.empty() )
		{
			const int STR_SIZE = str.size();
			for ( int i = 0; i < STR_SIZE; ++i )
			{
				str[i] = toupper(str[i]);
			}
		}
	}

	static void DelComment(std::string& str)
	{
		if ( !str.empty() )
		{
			std::size_t pos = str.find('#');
			if ( pos != std::string::npos )
			{
				str.erase(pos);
			}
		}
	}

	static std::string Num2Str(long num)
	{
		char buf[32] = "";
		snprintf(buf, sizeof(buf), "%ld", num);
		return buf;
	}

	static std::string CompleteStrWithSpace(const std::string& str, int width)
	{
		std::string compl_str = str;
		width -= compl_str.size();
		if ( width > 0 )
		{
			for ( int i = 0; i < width; ++i )
			{
				compl_str += ' ';
			}
		}

		return compl_str;
	}

	static bool IsAllNumber(const std::string& str)
	{
		if ( str.empty() )
		{
			return false;
		}

		const int STR_SIZE = str.size();
		for ( int i = 0; i < STR_SIZE; ++i )
		{
			if ( str[i] < '0' || str[i] > '9' )
			{
				return false;
			}
		}
		return true;
	}

	static int Str2Int(const std::string& str)
	{
		return atoi(str.c_str());
	}

	static float Str2Float(const std::string& str)
	{
		return atof(str.c_str());
	}

	static int SplitStr(const std::string& str, const std::string& delim, std::list<std::string>& list, bool b_trim)
	{
		if ( str.empty() || delim.empty() )
		{
			return 0;
		}

		const size_t SEP_SIZE = delim.size();
		size_t pos = 0;
		size_t f_pos = 0;

		std::string sub_str;
		while ( (f_pos = str.find(delim, pos)) != std::string::npos )
		{
			sub_str = str.substr(pos, f_pos-pos);
			if ( b_trim )
			{
				Trim(sub_str);
			}

			list.push_back(sub_str);

			pos = f_pos + SEP_SIZE;
		}

		sub_str = str.substr(pos);
		if ( b_trim )
		{
			Trim(sub_str);
		}

		list.push_back(sub_str);

		return list.size();
	}
};

#endif

