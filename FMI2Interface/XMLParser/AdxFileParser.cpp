#include "AdxFileParser.h"
#include <iostream>

#ifdef _WIN32
	#include "windows.h"
	#include <cctype>
#elif __linux__
	#include <sys/types.h>
	#include <unistd.h>
	#include <algorithm>
#endif

const char *AdxFileParser::elmNames[SIZEOF_ELM] = {
	"ADDRESS-CALCULATOR",
	"MEMORY-ELEMENT",
	"LABEL-NAME",
	"NAME",
	"ABSOLUTE-ADDRESS",
	"ROOT-OFFSET",
	"SIZE",
	"ARRAY-NBR-ELEMENTS",
	"ARRAY-ELEMENT-SIZE",
	"CATEGORY",
	"EXTERNAL"
};

AdxFileParser::AdxFileParser() : m_isMultiple(false)
{
}

AdxFileParser::~AdxFileParser()
{
	std::map<std::string, adxAddressDataType*>::iterator it = m_adxlabellist.begin();
	while (it != m_adxlabellist.end())
	{
		delete it->second;
		++it;
	}
	m_adxlabellist.clear();
}

void AdxFileParser::setIsMulitple(bool isMultiple)
{
    m_isMultiple = isMultiple;
}

std::string AdxFileParser::get_error_msg_description(int errorID) const
{
	std::string strMessage("");
#ifdef _WIN32
    DWORD errorMessageID = GetLastError();
    LPSTR messageBuffer = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    strMessage = messageBuffer;
    //Free the buffer.
    LocalFree(messageBuffer);

#elif __linux__
    if(errorID != 0)
    	strMessage = strMessage.append("ADX File Parser Error").append(strerror(errorID));
#endif
    return strMessage;
}

int AdxFileParser::parse(const std::string &path, std::string &strError)
{
	int retVal = 1;
	std::string adxPath = path;
	pugi::xml_document xmlDoc;
	pugi::xml_parse_result xmlResult = xmlDoc.load_file(adxPath.c_str());

	if (xmlResult)
	{
		pugi::xml_node xmlNode = xmlDoc.child(elmNames[elm_ADDRESS_CALCULATOR]);

		if (xmlNode)
		{
			parseChildElements(adxPath, xmlNode);
			retVal = 0;
		}
		else
		{
			strError = "The root element of the ADX document is invalid";
			retVal = 1;
		}
	}
	else
	{
		strError = "ADX configuration file is not parsed successfully.";
		retVal = 1;
	}
	return retVal;
}

void AdxFileParser::parseChildElements(const std::string &xmlPath, pugi::xml_node xmlParentNode)
{

	for (pugi::xml_node xmlChildNode : xmlParentNode.children(elmNames[elm_MEMORY_ELEMENT]))
	{
		bool externalFlag = false;
		std::string name;
		std::string labelname;

		adxAddressDataType *adxlabel = new adxAddressDataType;
		if (adxlabel == nullptr)
			return;

		//----------------------------------------------------------------------------
		// To have only file name without ".adx" extension in the fileName variable

		std::size_t pos = xmlPath.find("resources\\");
		std::string tempFileName = xmlPath.substr(pos + 10);
		tempFileName.erase(0, 11);
		std::string substring = ".adx";
		std::string::size_type n = substring.length();
		for (std::string::size_type i = tempFileName.find(substring); i != std::string::npos; i = tempFileName.find(substring))
			tempFileName.erase(i, n);
		substring.clear();
		adxlabel->fileName = tempFileName;
		//-----------------------------------------------------------------------------
		bool isarray = false;

		name = xmlChildNode.child_value(elmNames[elm_NAME]);
		labelname = xmlChildNode.child_value(elmNames[elm_LABEL_NAME]);
		adxlabel->address = strtol((char *)(xmlChildNode.child_value(elmNames[elm_ABSOLUTE_ADDRESS])), NULL, 0);
		adxlabel->offset = strtol((char *)(xmlChildNode.child_value(elmNames[elm_ROOT_OFFSET])), NULL, 0);
		adxlabel->size = atoi((char *)xmlChildNode.child_value(elmNames[elm_SIZE]));
		adxlabel->noelmnts = atoi((char *)xmlChildNode.child_value(elmNames[elm_ARRAY_NBR_ELEMENTS]));
		adxlabel->elmsize = atoi((char *)xmlChildNode.child_value(elmNames[elm_ARRAY_ELEMENT_SIZE]));
		std::string tempStr;
		tempStr = xmlChildNode.child_value(elmNames[elm_EXTERNAL]);
		if (tempStr.size() > 0)
			externalFlag = true;
		tempStr = xmlChildNode.child_value(elmNames[elm_CATEGORY]);
		if (tempStr.size() > 0)
			isarray = true;

		if (externalFlag)
			adxlabel->name.assign(name);
		else
			adxlabel->name.assign(labelname);

		if (m_isMultiple)
		{
			std::string labelName = "";
			labelName = labelName.append(adxlabel->fileName.c_str()).append("_").append(adxlabel->name);
			adxlabel->name.clear();
			adxlabel->name.assign(labelName);
		}
		m_adxlabellist.insert(std::pair<std::string, adxAddressDataType *>(adxlabel->name, adxlabel));
	}
}

bool AdxFileParser::getArrayDetails( const std::string& keyname ,std::queue<adxLabelArray>& outAdxLabelArrayQueue,
									std::string& outAdxLabelBaseArrayName,
                                    const std::vector<std::pair<int, int>>& SquareBracketPos )
{
	int dotOperatorPos = 0;
	int iArrayIndexValue;
	std::map<std::string, adxAddressDataType*>::iterator m_adxlabelListItr;

	try{
		for ( const auto &SquareBracket : SquareBracketPos ){

			outAdxLabelBaseArrayName +=  keyname.substr( dotOperatorPos, SquareBracket.first-dotOperatorPos );
			iArrayIndexValue = std::stoi( keyname.substr( SquareBracket.first+1, (SquareBracket.second - SquareBracket.first -1 ) ));

			//Check adxLabel base array name exist in the adxlabellist map.
			m_adxlabelListItr = m_adxlabellist.find(outAdxLabelBaseArrayName);
			if ( m_adxlabelListItr != m_adxlabellist.end() ){

				// Passed array Index is less than no of array element check .
				if ( (unsigned)iArrayIndexValue < m_adxlabelListItr->second->noelmnts ){
					outAdxLabelArrayQueue.emplace( adxLabelArray{m_adxlabelListItr->second,iArrayIndexValue} ) ;
				}else{
					std::cout <<" [ ERROR ] getArrayDetails --> Index Out of Range " << std::endl;
					std::cout <<" Array Index Received is " << iArrayIndexValue <<std::endl;
					std::cout <<" Array Element Number is " << m_adxlabelListItr->second->noelmnts <<std::endl;
					return false;
				}
			}else{
				std::cout <<" [ ERROR ] getArrayDetails --> Invalid Label Array  :: " << outAdxLabelBaseArrayName <<std::endl;
				return false ;
			}

			outAdxLabelBaseArrayName += "[0]" ;
			dotOperatorPos=SquareBracket.second + 1;
		}
		outAdxLabelBaseArrayName += keyname.substr(dotOperatorPos);
		return true;

	} catch(const std::exception &exc){
		std::cout << " [ FATAL ] getArrayDetails --> " << exc.what();
		return false;
	}
}

bool AdxFileParser::createAdxArrayElement(const std::string& keyname, const std::string& adxLabelBaseArrayName,std::queue<adxLabelArray>& adxLabelArrayQueue ){

	long offSetOfAdxElement = 0 ; 
	while( !adxLabelArrayQueue.empty() ){
		offSetOfAdxElement  =  ( offSetOfAdxElement + ( adxLabelArrayQueue.front().indexValue * adxLabelArrayQueue.front().adxLabel->elmsize ) ) ;
		adxLabelArrayQueue.pop();
	}

	adxAddressDataType *adxLabel = new adxAddressDataType ;
	if ( adxLabel == nullptr ) return false;

	std::map<std::string, adxAddressDataType*>::iterator m_adxlabelListItr = m_adxlabellist.find(adxLabelBaseArrayName) ;
	if ( m_adxlabelListItr != m_adxlabellist.end() ){

		*adxLabel= *( m_adxlabelListItr->second );
		adxLabel->name = keyname;
		// Address is calculated by adding Offset to the Base address
		adxLabel->offset += offSetOfAdxElement ;
		adxLabel->address += offSetOfAdxElement ;
		m_adxlabellist.insert( std::pair<std::string, adxAddressDataType*>( adxLabel->name , adxLabel ) );

		return true ;
	}else{
		std::cout<<" [ ERROR ] Invalid Label Name "<< adxLabelBaseArrayName <<std::endl;
		return false ;
	}
}

bool AdxFileParser::isArrayElement( const std::string& keyname , std::vector<std::pair<int, int>>& outSquareBracketPos ){

	//To find the array index position []; and store it in SquareBracketPos vector;
	for  (int openBracketPos =0 ; (size_t)openBracketPos <keyname.size(); ++openBracketPos ){
		if ( keyname[openBracketPos]=='[' ){
			for ( int closeBracketPos = openBracketPos+1 ; (size_t)closeBracketPos < keyname.size(); ++closeBracketPos ){
				if ( keyname[closeBracketPos] == ']' ){
					outSquareBracketPos.push_back({openBracketPos,closeBracketPos});
					openBracketPos = closeBracketPos;
					break;
				}
			}
		}
	}

	return outSquareBracketPos.size() == 0 ? false : true  ;

}

adxAddressDataType* AdxFileParser::getAdxdata(const std::string& keyname)
{
	if ( m_adxlabellist.size() == 0 ) return nullptr;
	std::map<std::string, adxAddressDataType*>::iterator m_adxlabelListItr = m_adxlabellist.find(keyname);
	if ( m_adxlabelListItr != m_adxlabellist.end() )
		return m_adxlabellist[keyname];

	std::vector<std::pair<int, int>> SquareBracketPos ; SquareBracketPos.reserve(5);
	//Check the keyname consist of array [] pattern
	bool status = isArrayElement( keyname , SquareBracketPos );
	if ( status == false ){
		return nullptr;
	}

	std::queue<adxLabelArray> adxLabelArrayQueue;
	std::string adxLabelBaseArrayName;adxLabelBaseArrayName.reserve( keyname.size() );
	// takes SquareBracketPos and populates adxLabelBaseArrayName , fetch the index value of array.
	status = getArrayDetails( keyname , adxLabelArrayQueue , adxLabelBaseArrayName , SquareBracketPos );
	if ( status == false ) {
		return nullptr;
	}

	status = createAdxArrayElement( keyname , adxLabelBaseArrayName , adxLabelArrayQueue );
	if( status == false ){
		std::cout <<" [ ERROR ] createAdxArrayElements " << std::endl;
		return nullptr;
	}

	return m_adxlabellist[keyname];
}