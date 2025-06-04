#include "LabelDataMap.h"
#include <iostream>

bool LabelDataMap::updateLabel(int key, fmi2LabelData *labelData)
{
	LabelDataMap::iterator iter = find(key);
	if (iter == end()) return false;
	if (iter->second.size() <= labelData->getLabelIndex()) return false;
	if (iter->second[labelData->getLabelIndex()] != labelData) return false;
	iter->second[labelData->getLabelIndex()] = labelData;
	return true;
}

const fmi2LabelData* LabelDataMap::getLabel(int key, unsigned int index)const
{
	LabelDataMap::const_iterator iter = find(key);
	if (iter == end()) return nullptr;
	if (iter->second.size() <= index) return nullptr;
	return iter->second[index];
}

LabelDataMap::~LabelDataMap()
{
	for (auto map_iter = begin(); map_iter != end(); ++map_iter)
	{
		for(auto vec_iter = map_iter->second.begin(); vec_iter != map_iter->second.end(); ++vec_iter)
			delete *vec_iter;
		map_iter->second.clear();
	}	
}

fmi2LabelData::fmi2LabelData() :
	  m_bIsGet(false)
	, m_bPrviouslyUpdated(true)  // Initial flag is to say, the data is updated in server. Will be set to false when new variable is set
	, m_bIsDataFetched(false)    // New data is not fetched from server
	, m_labelSize(0)
	, m_labelAddress(0L)
	, m_binaryValue(nullptr)
	, m_labelIndex(0)
	, m_elementType(NONE)
	, apply_quantization(false)
	
{
	m_mappedelementType = decltype(m_mappedelementType){
	{ std::string("sint8"), CustomDataType::SIGNED_CHAR },  { std::string("sint16"), CustomDataType::SIGNED_SHORT },   { std::string("sint32"), CustomDataType::SIGNED_INT },  {std::string("sint64"),CustomDataType::SIGNED_LONG_LONG_INT}, 
	{ std::string("uint8"), CustomDataType::UNSIGNED_CHAR },{ std::string("uint16"), CustomDataType::UNSIGNED_SHORT }, { std::string("uint32"), CustomDataType::UNSIGNED_INT },{std::string("uint64"),CustomDataType::UNSIGNED_LONG_LONG_INT},
	{ std::string("float32"), CustomDataType::FLOAT32 }, { std::string("float64"), CustomDataType::FLOAT64 },
	{ std::string("Boolean"), CustomDataType::BOOLEAN_ }, { std::string("String"), CustomDataType::STRING }, 
	{ std::string("Enumeration"), CustomDataType::ENUMERATION }, { std::string("Binary"), CustomDataType::BINARY } };

	m_mappedCausalityType = decltype(m_mappedCausalityType) {{ std::string("input"), CausalityType::INPUT_CAUSALITY}, { std::string("output"), CausalityType::OUTPUT_CAUSALITY }};
}

fmi2LabelData::~fmi2LabelData()
{
    delete[] m_binaryValue;
}

bool fmi2LabelData::isLabelFetching() const 
{ 
	return m_bIsGet; 
}

void fmi2LabelData::setLabelFetch(bool bIsGet) 
{ 
	m_bIsGet = bIsGet; 
}

bool fmi2LabelData::isLabelPreviouslyUpdated() const 
{ 
	return m_bPrviouslyUpdated; 
}

void fmi2LabelData::setLabelPreviouslyUpdated(bool bPrviouslyUpdated) 
{ 
	m_bPrviouslyUpdated = bPrviouslyUpdated; 
}

bool fmi2LabelData::isDataFetched() const 
{ 
	return m_bIsDataFetched; 
}

void fmi2LabelData::setDataFetched(bool bdataFetched)
{
	m_bIsDataFetched = bdataFetched;
}

unsigned int fmi2LabelData::getLabelIndex() const 
{
	return m_labelIndex; 
}

void fmi2LabelData::setLabelIndex(unsigned int labelIndex) 
{ 
	m_labelIndex = labelIndex; 
}

std::string fmi2LabelData::getLabelName() const 
{ 
	return m_labelName; 
}

void fmi2LabelData::setLabelName(const std::string &labelName) 
{ 
	m_labelName = labelName; 
}

std::string fmi2LabelData::getFileName() const
{
	return m_fileName;
}

void fmi2LabelData::setFileName(const std::string &fileName)
{
	m_fileName = fileName;
}

std::string fmi2LabelData::getCausality() const
{
	return m_causality;
}
CausalityType fmi2LabelData::getCausalityType() const
{
	return m_causalityType;
}
void fmi2LabelData::setCausality(const std::string &causality)
{
	m_causality = causality;
	if (m_mappedCausalityType.find(m_causality) != m_mappedCausalityType.end())
		m_causalityType = m_mappedCausalityType[m_causality];
}

std::string fmi2LabelData::getVariability() const{
	return m_variability;

}
void fmi2LabelData::setVariability(const std::string &variability)
{
	m_variability = variability;

}

size_t fmi2LabelData::getLabelSize() const 
{ 
	return m_labelSize; 
}

void fmi2LabelData::setLabelSize(size_t labelSize) 
{ 
	m_labelSize = labelSize; 
}

long fmi2LabelData::getLabelAddress() const 
{ 
	return m_labelAddress; 
}

void fmi2LabelData::setLabelAddress(long labelAddress) 
{ 
	m_labelAddress = labelAddress; 
}

std::string fmi2LabelData::getMimeType() const
{
    return m_mimeType;
}

void fmi2LabelData::setMimeType(std::string mimeType)
{
    m_mimeType = mimeType;
}

CustomDataType fmi2LabelData::getElementType() const
{
	return m_elementType;
}

void fmi2LabelData::setElementType(const std::string& elementType)
{
	if (m_mappedelementType.find(elementType) != m_mappedelementType.end())
		m_elementType = m_mappedelementType[elementType];
}

const fmi2LabelValue* fmi2LabelData:: getLabelValue() const 
{ 
	return &m_labelValue; 
}

const fmi2LabelValue* fmi2LabelData::getStartValue() const
{
	return &m_startValue;
}

const fmi2Binary fmi2LabelData::getBinaryValue() const
{
    return m_binaryValue;
}

void fmi2LabelData::setStringLabelValue(const fmi2String& stringValue)
{
    memset(m_labelValue.stringValue, 0x0, sizeof(m_labelValue.stringValue));
	strcpy((char *)m_labelValue.stringValue, stringValue);
	m_bIsDataFetched = true;
}

void fmi2LabelData::setStringLabelValue(const char* stringValue, size_t labelSize)
{
    memset(m_labelValue.stringValue, 0x0, sizeof(m_labelValue.stringValue));
    memcpy(m_labelValue.stringValue, stringValue, labelSize);
	m_bIsDataFetched = true;
}

void fmi2LabelData::setIntegerLabelValue(const fmi2Integer intValue)
{
	m_labelValue.intValue = intValue;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setIntegerLabelValue(const char* intValue, size_t labelSize)
{
	fmi2Integer value = 0;
	memcpy(&value, intValue, labelSize);
	m_labelValue.intValue = value;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setRealLabelValue(const fmi2Real realValue)
{
	m_labelValue.realValue = realValue;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setRealLabelValue(const char* realValue, size_t labelSize)
{
	fmi2Real value = 0;
	memcpy(&value, realValue, labelSize);
	m_labelValue.realValue = value;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setBoolLabelValue(const fmi2Boolean& boolValue)
{
	m_labelValue.boolValue = boolValue;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setBoolLabelValue(const char* boolValue, size_t labelSize)
{
	fmi2Boolean value = 0;
	memcpy(&value, boolValue, labelSize);
	m_labelValue.boolValue = value;
	m_bIsDataFetched = true;
}

void fmi2LabelData::setBinaryValue(const fmi2Binary binaryValue)
{
    m_binaryValue = binaryValue;
    m_bIsDataFetched = true;
}

void fmi2LabelData::setBinaryValue(const char* binary_value, size_t binarysize)
{
	if (m_binaryValue != nullptr)
		delete[] m_binaryValue;

    fmi2Binary value = new fmi2Binary[binarysize];
    memcpy(value, binary_value, binarysize);
    m_binaryValue = value;
    m_bIsDataFetched = true;
}

void fmi2LabelData::setIntegerStartValue(const char* value)
{
	int temp_val = 0;
	if (value)
		sscanf(value, "%d", &temp_val);

	m_startValue.intValue = temp_val;
}

void fmi2LabelData::setRealStartValue(const char* value)
{
	double temp_val = 0;
	if (value)
		sscanf(value, "%lf", &temp_val);

	m_startValue.realValue = temp_val;
}

void fmi2LabelData::setBoolStartValue(const char* value)
{
	bool temp_val = false;
	if (value)
	{
		if ( (!strcmp(value, "true")) || (!strcmp(value, "1")) )
		{
			temp_val = true;
		}
	}

	m_startValue.boolValue = temp_val;
}

void fmi2LabelData::setStringStartValue(const char* value)
{
	memset(m_startValue.stringValue, 0x0, sizeof(m_startValue.stringValue));
	if (value)
		memcpy(m_startValue.stringValue, value, sizeof(m_labelValue.stringValue));
}