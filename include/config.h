// 配置文件
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <list>
#include "exception.hpp"

class Config;

// 配置项
class CfgItem
{
	friend class Config;

public:
	CfgItem() {}
	CfgItem(const std::string& sName, const std::string& sValue = std::string(), const std::string& sSegment = std::string());
	virtual ~CfgItem() {}

public:
	// 是否为同一配置项
	friend bool operator == (const CfgItem& item1, const CfgItem& item2)
	{ return (item1.m_segment == item2.m_segment) && (item1.m_name == item2.m_name); }

	//// 设置配置值
	//bool SetValue(double dVal);
	//bool SetValue(long long llVal);
	//// 获取配置值
	//double DoubleValue() const;
	//long long LLongValue() const;
	bool BoolValue() const;

private:
	std::string		m_segment;		// 配置段
	std::string		m_name;			// 配置名
	std::string		m_value;		// 配置值
};

////////////////////////////////////////////////////////////////////////
// 配置
class Config
{
public:
	Config(const std::string& cfgFile = std::string());
	virtual ~Config() {}

public:
	// 指定配置文件
	bool SetCfgFile(const std::string& cfgFile);
	// 注册配置项
	bool RegisterItem(const std::string& sName, const std::string& sSegment = std::string());
	// 取消注册配置项
	bool UnregisterItem(const std::string& sName, const std::string& sSegment = std::string());
	// 初始化所有配置项
	void InitItems();
	// 删除所有配置项
	void DeleteItems();
	// 读取配置
	bool ReadConfig();
	// 获取配置值
	std::string GetCfgValue(const std::string& sName, const std::string& sSegment = std::string()) throw(Exception);
	float GetCfgFloatVal(const std::string& sName, const std::string& sSegment = std::string());
	int GetCfgIntVal(const std::string& sName, const std::string& sSegment = std::string());
	bool GetCfgBoolVal(const std::string& sName, const std::string& sSegment = std::string());

private:
	// 查找指定配置项
	bool FindItem(const std::string& sName, const std::string& sSegment, std::list<CfgItem>::iterator* pItr = NULL);
	// 尝试获取配置段
	bool TryGetSegment(const std::string& str, std::string& segment) const;
	// 尝试获取配置名与配置值
	bool TryGetNameValue(const std::string& str, std::string& name, std::string& value) const;
	// 删除注释
	void CleanComment(std::string& str) const;

private:
	std::string			m_cfgFile;		// 配置文件（含路径和名称）
	std::list<CfgItem>	m_listItems;	// 配置项列表
};

#endif 

