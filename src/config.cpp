#include "config.h"
#include <iostream>
#include <fstream>
#include "def.h"
#include "helper.hpp"

// class CfgItem
CfgItem::CfgItem(const std::string& sName, const std::string& sValue /*= std::string()*/, const std::string& sSegment /*= std::string()*/)
:m_segment(sSegment),m_name(sName),m_value(sValue)
{
}

//bool CfgItem::SetValue(double dVal)
//{
//	return HELP::TTrans2T<double,std::string>(dVal,m_value);
//}
//
//bool CfgItem::SetValue(long long llVal)
//{
//	return HELP::TTrans2T<long long,std::string>(llVal,m_value);
//}

//double CfgItem::DoubleValue() const
//{
//	double dVal = 0.0;
//	HELP::TTrans2T<std::string,double>(m_value,dVal);
//	return dVal;
//}
//
//long long CfgItem::LLongValue() const
//{
//	long long llVal = 0;
//	HELP::TTrans2T<std::string,long long>(m_value,llVal);
//	return llVal;
//}
//
//bool CfgItem::BoolValue() const
//{
//	return HELP::IsTrue(m_value);
//}

////////////////////////////////////////////////////////////////////////

// class Config
Config::Config(const std::string& cfgFile /*= std::string()*/)
{
	SetCfgFile(cfgFile);
}

bool Config::SetCfgFile(const std::string& cfgFile)
{
	if ( cfgFile.empty() )
	{
		return false;
	}

	m_cfgFile = cfgFile;
	return true;
}

bool Config::RegisterItem(const std::string& sName, const std::string& sSegment /*= std::string()*/)
{
	if ( FindItem(sName,sSegment) )
	{
		return false;
	}
	
	m_listItems.push_back(CfgItem(sName,"",sSegment));
	return true;
}

bool Config::UnregisterItem(const std::string& sName, const std::string& sSegment /*= std::string()*/)
{
	std::list<CfgItem>::iterator it;
	if ( FindItem(sName,sSegment,&it) )
	{
		m_listItems.erase(it);
		return true;
	}

	return false;
}

void Config::InitItems()
{
	for ( std::list<CfgItem>::iterator it = m_listItems.begin(); it != m_listItems.end(); ++it )
	{
		it->m_value.clear();
	}
}

void Config::DeleteItems()
{
	m_listItems.clear();
}

bool Config::ReadConfig()
{
	if ( m_cfgFile.empty() )
	{
		std::cerr << __FILE__ << "(" << __LINE__ << "): The configuration file path-name is empty!" << std::endl;
		return false;
	}

	// 打开文件（读取模式）
	std::fstream m_fsCfg;
	m_fsCfg.open(m_cfgFile.c_str(),std::ios_base::in);
	if ( !m_fsCfg.is_open() || !m_fsCfg.good() )
	{
		std::cerr << __FILE__ << "(" << __LINE__ << "): Error opening configuration file \"" << m_cfgFile << "\"" << std::endl;
		m_fsCfg.close();
		return false;
	}

	std::string strLine;
	std::string strSegment;			// 当前配置段	
	std::string strName;			// 配置名
	std::string strValue;			// 配置值
	while ( !m_fsCfg.eof() )
	{
		strLine.clear();
		std::getline(m_fsCfg,strLine);

		// 删除注释
		CleanComment(strLine);
		Helper::Trim(strLine);
		if ( strLine.empty() )
		{
			continue;
		}

		// 是否为配置段
		if ( TryGetSegment(strLine,strSegment) )
		{
			continue;
		}

		// 是否为注册的配置名与配置值
		if ( TryGetNameValue(strLine,strName,strValue) )
		{
			std::list<CfgItem>::iterator it;
			if ( FindItem(strName,strSegment,&it) )
			{
				it->m_value = strValue;
			}
		}
	}

	m_fsCfg.close();
	return true;
}

std::string Config::GetCfgValue(const std::string& sName, const std::string& sSegment /*= std::string()*/) throw(Exception)
{
	std::list<CfgItem>::iterator it;
	if ( FindItem(sName,sSegment,&it) )
	{
		if ( it->m_value.empty() )
		{
			std::string str_err = "[CFG] [" + sSegment + "->" + sName + "] configure value is invalid!";
			throw Exception(CFG_GET_VALUE_FAIL, str_err);
		}

		return it->m_value;
	}

	std::string str_err = "[CFG] Can not get [" + sSegment + "->" + sName + "] configure value!";
	throw Exception(CFG_GET_VALUE_FAIL, str_err);
	return std::string();
}

float Config::GetCfgFloatVal(const std::string& sName, const std::string& sSegment /*= std::string()*/)
{
	return Helper::Str2Float(GetCfgValue(sName,sSegment));
}

int Config::GetCfgIntVal(const std::string& sName, const std::string& sSegment /*= std::string()*/)
{
	return Helper::Str2Int(GetCfgValue(sName,sSegment));
}

bool Config::GetCfgBoolVal(const std::string& sName, const std::string& sSegment /*= std::string()*/)
{
	std::string strBool = GetCfgValue(sName,sSegment);
	Helper::Upper(strBool);
	return (std::string("TRUE") == strBool) || (std::string("YES") == strBool);
}

bool Config::FindItem(const std::string& sName, const std::string& sSegment, std::list<CfgItem>::iterator* pItr /*= NULL*/)
{
	const CfgItem ITEM(sName,"",sSegment);
	for ( std::list<CfgItem>::iterator it = m_listItems.begin(); it != m_listItems.end(); ++it )
	{
		if ( ITEM == *it )
		{
			if ( pItr != NULL )
			{
				*pItr = it;
			}
			return true;
		}
	}
	
	return false;
}

bool Config::TryGetSegment(const std::string& str, std::string& segment) const
{
	const std::string::size_type SIZE = str.size();
	if ( SIZE <= 2 )
	{
		return false;
	}

	if ( str[0] == '[' && str[SIZE-1] == ']' )
	{
		segment = str.substr(1,SIZE-2);
		Helper::Trim(segment);
		return true;
	}
	return false;
}

bool Config::TryGetNameValue(const std::string& str, std::string& name, std::string& value) const
{
	const std::string::size_type SIZE = str.size();
	if ( SIZE < 3 )
	{
		return false;
	}

	std::string::size_type equal_pos = str.find('=');
	if ( equal_pos != std::string::npos && equal_pos > 0 )
	{
		name = str.substr(0,equal_pos);
		Helper::Trim(name);

		value = str.substr(equal_pos+1);
		Helper::Trim(value);
		return true;
	}
	return false;
}

void Config::CleanComment(std::string& str) const
{
	if ( str.empty() )
	{
		return;
	}

	// 第一个注释符位置
	std::string::size_type first_comm_pos = str.find('#');
	std::string::size_type sec_comm_pos = str.find("//");
	first_comm_pos = first_comm_pos < sec_comm_pos ? first_comm_pos : sec_comm_pos;

	if ( first_comm_pos != std::string::npos )
	{
		str.erase(first_comm_pos);
	}
}

