/*
* FMI_H header defining an FMU structure containing all function definitions that will serve as links
* to the associated dll as well as a model description object containing all the data contained in the 
* associated model description.
*
* author: PES6SI
* -------------------------------------------------------------------------*/

#ifndef FMI_H
#define FMI_H

#include <Ws2tcpip.h>
#include <windows.h>
#include "fmi2Functions.h"
#include "XmlParserCApi.h"
#include "XmlParser.h"
#include "XmlElement.h"
typedef fmi2String* (fmi2getStringValue)(char* address, char*varTypeName);

typedef fmi2String* (fmi2setStringValue)(char* address, char*varTypeName, fmi2String stringToSet);

//clock related functions
typedef void*		 (fmi2InitClock)();
typedef void*		 (fmi2StartClock)();
typedef fmi2Integer* (fmi2getClockVal)();
typedef void*		 (fmi2AdvanceClock)();


typedef struct 
{
    ModelDescription* modelDescription;

	fmi2getStringValue				 *getStringDllValue;

	fmi2setStringValue				 *setStringDllValue;

	fmi2getClockVal					 *getClockValue;
	fmi2AdvanceClock				 *AdvanceClockByTick;
	fmi2StartClock					 *StartClock;
	fmi2InitClock					 *initClock;
} FMU;


#endif // FMI_H

