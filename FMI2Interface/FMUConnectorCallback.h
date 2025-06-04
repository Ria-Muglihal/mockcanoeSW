#ifndef _FMU_CONNECTOR_CALLBACK_H_
#define _FMU_CONNECTOR_CALLBACK_H_

enum E_SIGNAL_TYPE_T
{
	E_INT = 0,
	E_DOUBLE,
	E_CHAR,
	E_BOOL
};


// function pointers to callback function
typedef void(*fptr_doStep)(const int&, std::string&);
typedef bool(*fptr_initialize)(const char *,const void*);
typedef void(*fptr_terminate)();
typedef void(*fptr_setSignalValue)(const unsigned int[], size_t, const void*, void**, size_t, E_SIGNAL_TYPE_T);
typedef void(*fptr_getSignalValue)(const unsigned int[], size_t, void*, void**, size_t, E_SIGNAL_TYPE_T);

#pragma pack(1)
typedef struct
{
	fptr_setSignalValue setSignalValue;
	fptr_getSignalValue getSignalValue;
	fptr_doStep doStep;
	fptr_initialize initialize;
	fptr_terminate terminate;
}dllcallbackFunctions;

typedef void(*start_FMU_Connector_fptr)(dllcallbackFunctions*);
////////////

//Reserved for furture use
struct labelinfo_extended
{
	void* pointer;//could be null if not found in adx
	int size;//Could be set to default based on adx or modeldescription
	bool apply_quantization;
	int factor;
	int offset;
	const char * name;
	int labelindex;
	const char * causality;
	labelinfo_extended()
	{
		pointer = nullptr;
		size = 0;
		apply_quantization = false;
		name = nullptr;
		factor = 1;
		offset = 0;
		labelindex = -1;
		causality = nullptr;
	}
};

struct labelinfo
{
	void* pointer;//Pointer to scalar variable. Could be null if not found in adx
	int size;//Size of Scalar variable based on adx or modeldescription
	bool apply_quantization;//convertion for float to int ,viceversa
	const char * name;// scalar variable name from the model_description
	
	//start value from the modelDescription.xml (attribute "start")
	union
	{
		int start_value_integer;
		bool start_value_boolean;
		const char* start_value_string;
		double start_value_real;
	};

	labelinfo()
	{
		pointer = nullptr;
		size = 0;
		apply_quantization = false;
		name = nullptr;
		start_value_integer = 0;
		start_value_boolean = false;
		start_value_string = nullptr;
		start_value_real = 0.0;
	}
};



struct MDScalarVariableMap
{
	labelinfo** integerType;
	int integerTypeSize;
	labelinfo** stringType;
	int stringTypeSize;
	labelinfo** boolType;
	int boolTypeSize;
	labelinfo** realType;
	int realTypeSize;

	MDScalarVariableMap()
	{
		integerType = nullptr;
		stringType = nullptr;
		boolType = nullptr;
		realType = nullptr;
		integerTypeSize = 0;
		stringTypeSize = 0;
		boolTypeSize = 0;
		realTypeSize = 0;
	}
};
#pragma pack()
#endif
