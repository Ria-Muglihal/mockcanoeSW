#include "fmi2FunctionTypes.h"
#ifdef _WIN32
#include <string>
#include <stdint.h>
#elif __linux__
#include  <string.h>
#include<iostream>
#include<algorithm>
#endif
#include <map>
#include <vector>

typedef union
{
	fmi2Real realValue;
	fmi2Integer intValue;
    fmi2Char stringValue[255];
	fmi2Boolean boolValue;
} fmi2LabelValue;

typedef enum
{
	NONE = 0,
	SIGNED_CHAR ,
	SIGNED_SHORT,
	SIGNED_INT,
	UNSIGNED_CHAR,
	UNSIGNED_SHORT,
	UNSIGNED_INT,
	FLOAT32,
	FLOAT64,
	BOOLEAN_,
	STRING,
	ENUMERATION,
	BINARY,
	SIGNED_LONG_LONG_INT,
	UNSIGNED_LONG_LONG_INT
}CustomDataType;

typedef enum
{
	FMI2_INTEGER = 0,
	FMI2_REAL,
	FMI2_STRING,
	FMI2_BOOLEAN,
    FMI2_BINARY,
	FMI2_NONE
} fmi2LabelDataType;

typedef enum 
{
	INPUT_CAUSALITY= 0, 
	OUTPUT_CAUSALITY, 
	PARAMETER_CAUSALITY
} CausalityType;

class fmi2LabelData
{
private:
	bool m_bIsGet;
	bool m_bPrviouslyUpdated;    // Is this variable updated in the server? 
	bool m_bIsDataFetched;       // Flag to check whether the data is fetched fro the server


    std::string m_mimeType;
	size_t m_labelSize;
	long m_labelAddress;
	fmi2LabelValue m_labelValue;
    fmi2Binary m_binaryValue;
	unsigned int m_labelIndex;
	std::string m_fileName;
	CustomDataType m_elementType;
	fmi2LabelValue m_startValue;

public:
	std::string m_labelName;
	std::string m_causality;
	CausalityType m_causalityType;

	std::string m_variability;
	bool apply_quantization;
	fmi2Real    fmiQuantValues[4];
	std::map<std::string, CustomDataType> m_mappedelementType;
	std::map<std::string, CausalityType> m_mappedCausalityType;
	fmi2LabelData();
	~fmi2LabelData();

	bool isLabelFetching() const;
	void setLabelFetch(bool bIsGet);

	bool isLabelPreviouslyUpdated() const;
	void setLabelPreviouslyUpdated(bool bPrviouslyUpdated);

	bool isDataFetched() const;
	void setDataFetched(bool bdataFetched);

	unsigned int getLabelIndex() const;
	void setLabelIndex(unsigned int labelIndex);

	std::string getLabelName() const;
	void setLabelName(const std::string &labelName);

	std::string getFileName() const;
    void setFileName(const std::string &fileName);

	std::string getCausality() const;
	CausalityType getCausalityType() const;
	void setCausality(const std::string &causality);

	std::string getVariability() const;
	void setVariability(const std::string &variability);
	

	size_t getLabelSize() const;
	void setLabelSize(size_t labelSize);

	long getLabelAddress() const;
	void setLabelAddress(long labelAddress);

    std::string getMimeType() const;
    void setMimeType(std::string mimeType);

	CustomDataType getElementType() const;
	void setElementType(const std::string& elementType);

	const fmi2LabelValue* getLabelValue() const;
	const fmi2LabelValue* getStartValue() const;
    const fmi2Binary getBinaryValue() const;
	void setStringLabelValue(const fmi2String& stringValue);

	void setStringLabelValue(const char* stringValue, size_t labelSize);

	void setIntegerLabelValue(const fmi2Integer intValue);
	void setIntegerLabelValue(const char* intValue, size_t labelSize);
	void setRealLabelValue(const fmi2Real realValue);
	void setRealLabelValue(const char* realValue, size_t labelSize);
	void setBoolLabelValue(const fmi2Boolean& boolValue);
	void setBoolLabelValue(const char* boolValue, size_t labelSize);
    void setBinaryValue(const fmi2Binary binaryValue);
    void setBinaryValue(const char* binary_value, size_t binarysize);

	//start value from modelDescription.xml file
	void setIntegerStartValue(const char* value);
	void setRealStartValue(const char* value);
	void setBoolStartValue(const char* value);
	void setStringStartValue(const char* value);

private:
	fmi2LabelData(const fmi2LabelData&);
	fmi2LabelData& operator=(const fmi2LabelData&);
};

class LabelDataMap : public std::map <int, std::vector<fmi2LabelData*>>
{

public:
	bool updateLabel(int key, fmi2LabelData *labelData);
    const fmi2LabelData* getLabel(int key, unsigned int index)const;
	~LabelDataMap();
};


