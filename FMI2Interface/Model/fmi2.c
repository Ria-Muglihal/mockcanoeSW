/* -------------------------------------------------------------------------
* fmi.c
* Implementation of used functions defined in FMI 2.0 libraries
* Copyright Robert Bosch GmbH. All rights reserved.
* -------------------------------------------------------------------------*/

#include "fmi2.h"
#include <stdarg.h>
#ifdef FMI_H

#ifdef __cplusplus
extern "C"{
#endif
/* -------------------------------------------------------------------------
* Callback functions for the fmu model instance, these are default callback functions
* These functions are to be called if not defined
*
* ------------------------------------------------------------------------*/
#define MAX_MSG_SIZE 1000
void logger(void *componentEnvironment, fmi2String instanceName, fmi2Status status,
	fmi2String category, fmi2String message, ...) 
{
	char msg[MAX_MSG_SIZE];
	//char* copy;

	va_list argp;

	//replace C format strings
	va_start(argp, message);
	vsprintf(msg, message, argp);
	va_end(argp);

	// print the final message
	if (!instanceName) instanceName = "?";
	if (!category) category = "?";
	printf("%s %s (%s): %s\n", fmi2StatusToString(status), instanceName, category, msg);
}

static const char* fmi2StatusToString(fmi2Status status)
{
	switch (status)
	{
	case fmi2OK:      return "ok";
	case fmi2Warning: return "warning";
	case fmi2Discard: return "discard";
	case fmi2Error:   return "error";
	case fmi2Fatal:   return "fatal";
#ifdef FMI_COSIMULATION
	case fmi2Pending: return "fmi2Pending";
#endif
	default:         return "?";
	}
}


#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif