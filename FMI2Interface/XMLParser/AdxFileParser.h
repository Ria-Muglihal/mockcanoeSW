#pragma once

#include <cstring>
#include <string>
#include <pugixml.hpp>
#include <queue>
#include <regex>
#include <map>
#include <stdint.h>

struct adxAddressDataType
{
public:

	adxAddressDataType()
	{
		name.clear();
		offset 	= 0;
		size 	= 0;
		address = 0;
		memset(&value, 0, sizeof(value));
		noelmnts = 0;
		elmsize  = 0;
        fileName = "";
	}
	std::string 	name;
	uint32_t  		offset;
	size_t  		size;
	long 			address;
	unsigned char 	value[16];
	uint32_t 		noelmnts;
	uint32_t 		elmsize;
    std::string 	fileName;
};


struct adxLabelArray{
	adxAddressDataType* adxLabel;
	int indexValue;
};

class xmlData
{
public:
	adxAddressDataType* m_adxdatalist;
};

class AdxFileParser
{
	static const int SIZEOF_ELM = 11;
	static const char *elmNames[SIZEOF_ELM];
	enum Elm {
		elm_BAD_DEFINED = -1,
		elm_ADDRESS_CALCULATOR,
		elm_MEMORY_ELEMENT,
		elm_LABEL_NAME,
		elm_NAME,
		elm_ABSOLUTE_ADDRESS,
		elm_ROOT_OFFSET,
		elm_SIZE,
		elm_ARRAY_NBR_ELEMENTS,
		elm_ARRAY_ELEMENT_SIZE,
		elm_CATEGORY,elm_EXTERNAL
	};

private:
    void parseChildElements(const std::string& xmlPath, pugi::xml_node xmlParentNode);
	std::string get_error_msg_description(int errorID) const;
	bool isArrayElement( const std::string& keyname , std::vector<std::pair<int, int>>& outSquareBracketPos );
	bool getArrayDetails(const std::string& keyname ,std::queue<adxLabelArray>& outAdxLabelArrayQueue,
							std::string& outAdxLabelBaseArrayName,
							const std::vector<std::pair<int, int>>& SquareBracketPos);
	bool createAdxArrayElement(const std::string& keyname, const std::string& adxLabelBaseArrayName, std::queue<adxLabelArray>& adxLabelArrayQueue );

public:
    bool m_isMultiple; // If multiple adx files present
	std::map<std::string, adxAddressDataType*> m_adxlabellist;
	AdxFileParser();
	~AdxFileParser();
    void setIsMulitple( bool isMultiple);
    int parse(const std::string& path, std::string& strError);
	adxAddressDataType* getAdxdata(const std::string& keyname);
};