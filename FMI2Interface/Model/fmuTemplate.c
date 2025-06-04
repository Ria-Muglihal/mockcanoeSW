/* ---------------------------------------------------------------------------*
 * fmuTemplate.c
 * Implementation of the FMI interface based on functions and macros to
 * be defined by the includer of this file.
 * If FMI_COSIMULATION is defined, this implements "FMI for Co-Simulation 2.0",
 * otherwise "FMI for Model Exchange 2.0".
 * The "FMI for Co-Simulation 2.0", implementation assumes that exactly the
 * following capability flags are set to fmi2True:
 *    canHandleVariableCommunicationStepSize, i.e. fmi2DoStep step size can vary
 * and all other capability flags are set to default, i.e. to fmi2False or 0.
 *
 * Revision history
 *  07.03.2014 initial version released in FMU SDK 2.0.0
 *  02.04.2014 allow modules to request termination of simulation, better time
 *             event handling, initialize() moved from fmi2EnterInitialization to
 *             fmi2ExitInitialization, correct logging message format in fmi2DoStep.
 *  10.04.2014 use FMI 2.0 headers that prefix function and types names with 'fmi2'.
 *  13.06.2014 when fmi2setDebugLogging is called with 0 categories, set all
 *             categories to loggingOn value.
 *  09.07.2014 track all states of Model-exchange and Co-simulation and check
 *             the allowed calling sequences, explicit isTimeEvent parameter for
 *             eventUpdate function of the model, lazy computation of computed values.
 *
 * Author: Adrian Tirea
 * Copyright QTronic GmbH. All rights reserved.
 * ---------------------------------------------------------------------------*/

#include "../ECUConnectorFactory.h"
#include <stdlib.h>
#include "../FMI2Interface.h"
#include "../FMI2InterfaceDirectComm.h"
#include <sstream>
#ifdef VISUAL_LEAK_DETECTOR
	#include "vld.h"
#endif

char* currentFMULocation;
bool isSlaveActive = false;

#ifdef __cplusplus
extern "C" {
#endif

// macro to be used to log messages. The macro check if current
// log category is valid and, if true, call the logger provided by simulator.
#define FILTERED_LOG(instance, status, categoryIndex, message, ...) if (isCategoryLogged(instance, categoryIndex)) \
        instance->functions->logger(instance->functions->componentEnvironment, instance->instanceName, status, \
        logCategoriesNames[categoryIndex], message, ##__VA_ARGS__);

static fmi2String logCategoriesNames[] = {"logAll", "logError", "logFmiCall", "logEvent"};

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

fmi2Boolean isCategoryLogged(ModelInstance *comp, int categoryIndex);
fmi2Status fmiStartDescreteComponent(fmi2Component c);

static inline fmi2Boolean invalidNumber(ModelInstance* comp, const char* f, const char* arg, int n, int nExpected) {
    if (n != nExpected) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected)
        return fmi2True;

    }
    return fmi2False;
}

static inline fmi2Boolean invalidState(ModelInstance *comp, const char *f, int statesExpected) {
    if (!comp)
        return fmi2True;
    if (!(comp->state & statesExpected)) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal call sequence.", f)
        return fmi2True;
    }
    return fmi2False;
}

static inline fmi2Boolean nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {
    if (!p) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Invalid argument %s = NULL.", f, arg)
        return fmi2True;
    }
    return fmi2False;
}

static inline fmi2Boolean vrOutOfRange(ModelInstance *comp, const char *f, fmi2ValueReference vr, unsigned int end) {
    if (vr >= end) {
        FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Illegal value reference %u.", f, vr)
        comp->state = modelError;
        return fmi2True;
    }
    return fmi2False;
}

static fmi2Status unsupportedFunction(fmi2Component c, const char *fName, int statesExpected) {
    ModelInstance *comp = (ModelInstance *)c;
    //fmi2CallbackLogger log = comp->functions->logger;
    if (invalidState(comp, fName, statesExpected))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, fName);
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "%s: Function not implemented.", fName)
    return fmi2Error;
}

fmi2Status setString(fmi2Component comp, fmi2ValueReference vr, fmi2String value) {
    return fmi2SetString(comp, &vr, 1, &value);
}

// ---------------------------------------------------------------------------
// Private helpers logger
// ---------------------------------------------------------------------------

// return fmi2True if logging category is on. Else return fmi2False.
fmi2Boolean isCategoryLogged(ModelInstance *comp, int categoryIndex) {
    if (categoryIndex < NUMBER_OF_CATEGORIES
        && (comp->logCategories[categoryIndex] || comp->logCategories[LOG_ALL])) {
        return fmi2True;
    }
    return fmi2False;
}


fmi2Status startSlaveController(ModelInstance *comp)
{
	std::string comm_type = "";
    fmi2Status status = fmi2Error;

#ifdef WIN32
    std::string ext = ".dll";
#else
    std::string ext = ".so";
#endif

    std::string filename = FMU_CONNECTOR_LIB + ext;
    std::string abspath = std::string(currentFMULocation).append(PATH_SEPARATOR BINARIES_DIR) + filename;
    std::string curr_abspath = std::string(currentFMULocation).append(PATH_SEPARATOR) + filename;

    if (boost::filesystem::exists(abspath) || boost::filesystem::exists(curr_abspath))
    {
		//assigning DirectMode class obj to communication pointer
		CFMI2Interface::getInstance(comp->instanceName).m_com_ptr = std::make_shared<CFMI2Interface_Direct_Com>();
	}
	else
    {
        comm_type = CFMI2Interface::getInstance(comp->instanceName).getFmuConfig()->fmu.com_mode;
		if (comm_type == "TCP" || comm_type == "UDP")
        {
            CFMI2Interface::getInstance(comp->instanceName).m_com_ptr = std::make_shared<CFMI2Interface_IPC>();
        }
		else
		{
            FILTERED_LOG(comp, fmi2Error, LOG_FMI_CALL, "startSlaveController : Invalid commMode / No dll found ")
			return status;
		}
 	}

    CFMI2Interface::getInstance(comp->instanceName).SetConfigData();

    if ( CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->initializeCommInterface(comp, currentFMULocation) == fmi2OK )
    {
    	isSlaveActive = true;
        status = fmi2OK;
    }
    return status;
}

void stopSlaveController(ModelInstance *comp)
{
	CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->uninitializeCommInterface(comp);
	isSlaveActive = false;
}


// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------
fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                            fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions,
                            fmi2Boolean visible, fmi2Boolean loggingOn) {
    // ignoring arguments: fmuResourceLocation, visible
	ModelInstance *comp;
    if (!functions || !functions->logger) {
        return NULL;
    }

    if (!functions->allocateMemory || !functions->freeMemory) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Missing callback function.");

        return NULL;
    }
    if (!instanceName || strlen(instanceName) == 0) {
        functions->logger(functions->componentEnvironment, "?", fmi2Error, "error",
                "fmi2Instantiate: Missing instance name.");
        return NULL;
    }
    // instanceName is used before allocating memory for the comp (ModelInstance). 
    // Eg: initLists is called before allocation so by using CFMI2Interface::getInstanceName() instanceName will be passed to logger()
	CFMI2Interface::setInstanceName(instanceName);

	if (!((fmuResourceLocation != nullptr) && (fmuResourceLocation[0] != '\0')))
	{
		functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
			"fmi2Instantiate: Missing fmu resource location.");
		return NULL;
	}

	functions->logger(functions->componentEnvironment, instanceName, fmi2OK, "running",
			"Resource location path %s", fmuResourceLocation);

	
#ifdef WIN32
	//Convert URI Format to File Path
	std::string fmu_resource_location_uri(fmuResourceLocation);
	char fmu_path_buffer[MAX_PATH];
	memset(fmu_path_buffer, 0, MAX_PATH);
	DWORD cch = ARRAYSIZE(fmu_path_buffer);
	HRESULT hr = ::PathCreateFromUrlA(fmu_resource_location_uri.c_str(), fmu_path_buffer, &cch, 0);
	if (FAILED(hr))
	{
		return NULL;
	}
	std::string fmu_resource_path(fmu_path_buffer);


#else
	std::string fmu_resource_path(fmuResourceLocation);
	fmu_resource_path = fmu_resource_path.substr(7, fmu_resource_path.length());

#endif

	std::size_t found = fmu_resource_path.find("resources", fmu_resource_path.length() - 10);
	if (found != std::string::npos)
	{
		currentFMULocation = (char*)functions->allocateMemory(strlen(fmu_resource_path.c_str()) - 9, sizeof(char));
		strncpy(currentFMULocation, fmu_resource_path.c_str(), strlen(fmu_resource_path.c_str()) - 10);
		functions->logger(functions->componentEnvironment, instanceName, fmi2OK, "running",
			"current fmu location path %s", currentFMULocation);
		currentFMULocation[strlen(fmu_resource_path.c_str()) - 10] = '\0';
	}
	else
	{
		currentFMULocation = (char*)functions->allocateMemory(strlen(fmu_resource_path.c_str()), sizeof(char));
		strncpy(currentFMULocation, fmu_resource_path.c_str(), strlen(fmu_resource_path.c_str()));
		currentFMULocation[strlen(fmu_resource_path.c_str())] = '\0';
	}

	//XML File Exists Check
	std::string model_xml_file_path;

	std::string xml_path_at_fmu_root = std::string(currentFMULocation) + "/modelDescription.xml";
	std::string xml_path_at_fmu_resources = std::string(currentFMULocation) + "/resources/modelDescription.xml";
	if (boost::filesystem::exists(xml_path_at_fmu_root.c_str()))
	{
		//XML File available at FMU Root Folder
		model_xml_file_path = xml_path_at_fmu_root;
	}
	else if (boost::filesystem::exists(xml_path_at_fmu_resources.c_str()))
	{
		//XML File available at FMU Resource Folder
		model_xml_file_path = xml_path_at_fmu_resources;
	}
	else
	{
		functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
			"fmi2Instantiate: Unable to locate model description XML file.");
		return NULL;
	}

	////--------------- INI file parsing   ------------------------------------------
	std::string strError;
	std::string ini_path_str = std::string(currentFMULocation) + "/resources/" + INI_FILE_NAME + ".ini";
	functions->logger(functions->componentEnvironment, instanceName, fmi2OK, "running",
		"fmi2Instantiate: INI config file path %s.", ini_path_str.c_str());

    if ( false == CFMI2Interface::getInstance(instanceName).parseConfigIniFie(const_cast<char*>( ini_path_str.c_str() ), functions, strError) )
	{
		functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
			"fmiStartDescreteComponent: INI parse error: %s.", strError.c_str());
		return NULL;
	}

	std::string binary_xml_file_path( std::string(currentFMULocation ) + "resources/modelBinaryDescription.xml" ) ; 
    if ( boost::filesystem::exists(binary_xml_file_path.c_str()) )
	{
	    //Initialize list of Variables of Binaryt Datatypes
        if ( 1 == CFMI2Interface::getInstance(instanceName).initLists(binary_xml_file_path.c_str() , functions ) )
            return NULL ;
    }

	//Initialize list of Variables of Standard Datatypes
	if ( 1 == CFMI2Interface::getInstance(instanceName).initLists(model_xml_file_path.c_str()  , functions ) ){
        return NULL;
    }

    if (!fmuGUID || strlen(fmuGUID) == 0) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Missing GUID.");

        return NULL;
    }

    if (strcmp(fmuGUID, MODEL_GUID)) {
        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
                "fmi2Instantiate: Wrong GUID %s. Expected %s.", fmuGUID, MODEL_GUID);

        return NULL;
    }
    comp = (ModelInstance *)functions->allocateMemory(1, sizeof(ModelInstance));
    if (comp) {
        int i;
        comp->r = (fmi2Real *) functions->allocateMemory(CFMI2Interface::getInstance(instanceName).getNumberofReals(), sizeof(fmi2Real));
        comp->i = (fmi2Integer *) functions->allocateMemory(CFMI2Interface::getInstance(instanceName).getNumberofIntegers(), sizeof(fmi2Integer));
        comp->b = (fmi2Boolean *) functions->allocateMemory(CFMI2Interface::getInstance(instanceName).getNumberofBooleans(), sizeof(fmi2Boolean));
        comp->bi= (fmi2Binary  *)functions->allocateMemory(CFMI2Interface::getInstance(instanceName).getNumberofBinarys(),  sizeof(fmi2Binary));
        comp->s = (fmi2String *) functions->allocateMemory(CFMI2Interface::getInstance(instanceName).getNumberofStrings(), sizeof(fmi2String));
        comp->isPositive = (fmi2Boolean *)functions->allocateMemory(NUMBER_OF_EVENT_INDICATORS,
            sizeof(fmi2Boolean));
        comp->instanceName = (char *)functions->allocateMemory(1 + strlen(instanceName), sizeof(char));
        comp->GUID = (char *)functions->allocateMemory(1 + strlen(fmuGUID), sizeof(char));

        // set all categories to on or off. fmi2SetDebugLogging should be called to choose specific categories.
        for (i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            comp->logCategories[i] = loggingOn;
        }
    }

    if (!comp || !comp->r || !comp->i || !comp->b || ( !comp->bi ) || !comp->s || !comp->isPositive
        || !comp->instanceName || !comp->GUID) {

        functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error",
            "fmi2Instantiate: Out of memory.");
        return NULL;
    }
    comp->time = 0; // overwrite in fmi2SetupExperiment, fmi2SetTime
    strcpy((char *)comp->instanceName, (char *)instanceName);
    comp->type = fmuType;
    strcpy((char *)comp->GUID, (char *)fmuGUID);
    comp->functions = functions;
    comp->componentEnvironment = functions->componentEnvironment;
    comp->loggingOn = loggingOn;
    comp->state = modelInstantiated;
    CFMI2Interface::getInstance(instanceName).setStartValues(comp); // to be implemented by the includer of this file
    comp->isDirtyValues = fmi2True; // because we just called setStartValues
    comp->isNewEventIteration = fmi2False;

    comp->eventInfo.newDiscreteStatesNeeded = fmi2False;
    comp->eventInfo.terminateSimulation = fmi2False;
    comp->eventInfo.nominalsOfContinuousStatesChanged = fmi2False;
    comp->eventInfo.valuesOfContinuousStatesChanged = fmi2False;
    comp->eventInfo.nextEventTimeDefined = fmi2False;
    comp->eventInfo.nextEventTime = 0;

	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Instantiate: GUID=%s", fmuGUID)

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Instantiate: Starting vECU", fmuGUID)
	fmi2Status status = fmiStartDescreteComponent(comp);
	if (status != fmi2OK)
	{
		comp->functions->logger(comp->functions->componentEnvironment, comp->instanceName, fmi2Error, "error",
			"fmiStartDescreteComponent: Unable to start the vECU");
		return nullptr;
	}

    return comp;
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    // ignore arguments: stopTimeDefined, stopTime
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetupExperiment", MASK_fmi2SetupExperiment))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetupExperiment: toleranceDefined=%d tolerance=%g",
        toleranceDefined, tolerance)
    comp->time = startTime;

	if (stopTimeDefined)
		comp->timeStop = stopTime;
	if (tolerance)
		comp->tolerance = tolerance;

    return fmi2OK;
  }



fmi2Status fmiStartDescreteComponent(fmi2Component c)
{
	ModelInstance *comp = (ModelInstance *)c;
	//std::string strError;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmiStartDescreteComponent")
	return startSlaveController(comp);
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2EnterInitializationMode")
    if (invalidState(comp, "fmi2EnterInitializationMode", MASK_fmi2EnterInitializationMode))
        return fmi2Error;

    comp->state = modelInitializationMode;
    return fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2ExitInitializationMode", MASK_fmi2ExitInitializationMode))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2ExitInitializationMode")


    // if values were set and no fmi2GetXXX triggered update before,
    // ensure calculated values are updated now
    if (comp->isDirtyValues) {
        //calculateValues(comp);
        comp->isDirtyValues = fmi2False;
    }

    if (comp->type == fmi2ModelExchange) {
        comp->state = modelEventMode;
        comp->isNewEventIteration = fmi2True;
    }
    else comp->state = modelStepComplete;
    return fmi2OK;
}

fmi2Status fmi2Terminate(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Terminate", MASK_fmi2Terminate))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Terminate")
    comp->state = modelTerminated;
    return fmi2OK;
}

fmi2Status fmi2Reset(fmi2Component c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Reset", MASK_fmi2Reset))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2Reset")
    comp->state = modelInstantiated;
    CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->clearDataBuffer();
    CFMI2Interface::getInstance(comp->instanceName).setStartValues(comp); // to be implemented by the includer of this file
    comp->isDirtyValues = fmi2True; // because we just called setStartValues
	return startSlaveController(comp);
    //return fmi2OK;
}

void fmi2FreeInstance(fmi2Component c) {

		ModelInstance *comp = (ModelInstance *)c;
		if (!comp) return;
		if (invalidState(comp, "fmi2FreeInstance", MASK_fmi2FreeInstance))
			return;
		
		FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2FreeInstance")

		if (c)
		{
			//Added Code for VECUServer Dll Terminate Support
			CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->fmuTerminate();
			stopSlaveController(comp);
		}

		comp->functions->freeMemory(currentFMULocation);
		if (comp->r) comp->functions->freeMemory(comp->r);
		if (comp->i) comp->functions->freeMemory(comp->i);
		if (comp->b) comp->functions->freeMemory(comp->b);
		if (comp->b) comp->functions->freeMemory(comp->bi);
		if (comp->s) {
			unsigned int i;
            for ( i = 0; i < CFMI2Interface::getInstance(comp->instanceName).getNumberofStrings(); i++ ){
				if (comp->s[i]) comp->functions->freeMemory((void *)comp->s[i]);
			}
			comp->functions->freeMemory((void *)comp->s);
		}
		
        CFMI2Interface::getInstance(comp->instanceName).FreeListMemory(comp->functions);
		comp->functions->logger(comp->functions->componentEnvironment, comp->instanceName, fmi2OK, "Running",
			"fmi2FreeInstance: freed bridge");

		comp->functions->logger(comp->functions->componentEnvironment, comp->instanceName, fmi2OK, "Running",
			"fmi2FreeInstance: freeing component");
        if ( comp->isPositive ) comp->functions->freeMemory(comp->isPositive);
        if ( comp->GUID ) comp->functions->freeMemory((void *) comp->GUID);

        // Delete the interface instance.
        CFMI2Interface::deleteInstance(comp->instanceName);

        if ( comp->instanceName ) comp->functions->freeMemory((void *) comp->instanceName);
        comp->functions->freeMemory(comp);	
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi2GetVersion() {
    return fmi2Version;
}

const char* fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {

	// ignore arguments: nCategories, categories
    size_t j;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetDebugLogging", MASK_fmi2SetDebugLogging))
        return fmi2Error;
    comp->loggingOn = loggingOn;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetDebugLogging")

	// reset all categories
    for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
        comp->logCategories[j] = fmi2False;
    }

    if (nCategories == 0) {
        // no category specified, set all categories to have loggingOn value
        for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
            comp->logCategories[j] = loggingOn;
        }
    } else {
		size_t i;
        // set specific categories on
        for (i = 0; i < nCategories; i++) {
            fmi2Boolean categoryFound = fmi2False;
            for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
                if (strcmp(logCategoriesNames[j], categories[i]) == 0) {
                    comp->logCategories[j] = loggingOn;
                    categoryFound = fmi2True;
                    break;
                }
            }
            if (!categoryFound) {
                comp->functions->logger(comp->componentEnvironment, comp->instanceName, fmi2Warning,
                    logCategoriesNames[LOG_ERROR],
                    "logging category '%s' is not supported by model", categories[i]);
            }
        }
    }

    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetDebugLogging")
    return fmi2OK;
}

fmi2Status fmi2GetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetReal", MASK_fmi2GetReal))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "value[]", value))
        return fmi2Error;
    if (nvr > 0 && comp->isDirtyValues) {
        //calculateValues(comp);
        comp->isDirtyValues = fmi2True;
    }
	fmi2Status status = fmi2OK;
	unsigned int realSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofReals();
    if (realSignalCount  > 0 )
	{
		size_t i;
		for (i = 0; i < nvr; ++i) {
            if ( vrOutOfRange(comp, "fmi2GetReal", vr[i], realSignalCount) )
				return fmi2Error;
		}

        status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getReal(comp, vr, nvr, value);

		for (i = 0; i < nvr; ++i) {
			FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetReal: #r%u# = %.16g", vr[i], value[i])
		}
	}
	return status;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
	size_t i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetInteger", MASK_fmi2GetInteger))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "value[]", value))
            return fmi2Error;
    if (nvr > 0 && comp->isDirtyValues) {
        //calculateValues(comp);
        comp->isDirtyValues = fmi2False;
    }
	unsigned int intSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofIntegers();
    for (i = 0; i < nvr; ++i) {
        if ( vrOutOfRange(comp, "fmi2GetInteger", vr[i], intSignalCount) )
            return fmi2Error;
    }

    fmi2Status status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getInteger(comp, vr, nvr, value);

	for (i = 0; i < nvr; ++i) {
		FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetInteger: #i%u# = %d", vr[i], value[i])
	}

	return status;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
	size_t i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetBoolean", MASK_fmi2GetBoolean))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "vr[]", vr))
            return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "value[]", value))
            return fmi2Error;
    if (nvr > 0 && comp->isDirtyValues) {
        //calculateValues(comp);
        comp->isDirtyValues = fmi2False;
    }
    unsigned int boolSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofBooleans();
    for (i = 0; i < nvr; ++i) {
        if ( vrOutOfRange(comp, "fmi2GetBoolean", vr[i], boolSignalCount) )
            return fmi2Error;
    }

    fmi2Status status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getBoolean(comp, vr, nvr, value);

	for (i = 0; i < nvr; ++i) {
		FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetBoolean: #b%u# = %s", vr[i], value[i] ? "true" : "false")
	}

	return status;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
	size_t i;
	ModelInstance *comp = (ModelInstance *)c;
	if (invalidState(comp, "fmi2GetString", MASK_fmi2GetString))
		return fmi2Error;
	if (nvr>0 && nullPointer(comp, "fmi2GetString", "vr[]", vr))
		return fmi2Error;
	if (nvr>0 && nullPointer(comp, "fmi2GetString", "value[]", value))
		return fmi2Error;
	if (nvr > 0 && comp->isDirtyValues) {
		//calculateValues(comp);
		comp->isDirtyValues = fmi2False;
	}

    unsigned int stringSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofStrings();
	for (i = 0; i<nvr; ++i) {
		if (vrOutOfRange(comp, "fmi2GetString", vr[i], stringSignalCount))
			return fmi2Error;
	}

	fmi2Status status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getString(comp, vr, nvr, value);

	for (i = 0; i < nvr; ++i) {
		FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetString: #s%u# = '%s'", vr[i], value[i])
	}

	return status;
}

fmi2Status fmi2GetBinary( fmi2Component c, const fmi2ValueReference vr[], size_t nvr, size_t sizes[], fmi2Binary value[], size_t nvalues ) {
    size_t i;
    ModelInstance *comp = (ModelInstance *) c;
    if ( invalidState( comp, "fmi2GetBinary", MASK_fmi2GetBinary ) )
        return fmi2Error;
    if ( nvr>0 && nullPointer( comp, "fmi2GetBinary", "vr[]", vr ) )
        return fmi2Error;
    if ( nvr>0 && nullPointer( comp, "fmi2GetBinary", "value[]", value ) )
        return fmi2Error;
    if ( nvr > 0 && comp->isDirtyValues ) {
        //calculateValues(comp);
        comp->isDirtyValues = fmi2False;
    }

    fmi2Status status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getBinary(comp, vr, nvr, sizes, value);

	for (i = 0; i<nvr; ++i) {
		FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, std::string("fmi2GetBinary: ").append(std::to_string(vr[i])).append((char*)value[i]).c_str())
	}

    return status;
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
	size_t i;
	fmi2Status status = fmi2OK;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetReal", MASK_fmi2SetReal))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: nvr = %d", nvr)

	// no check whether setting the value is allowed in the current state
	unsigned int realSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofReals();
    for (i = 0; i < nvr; ++i) {
        if ( vrOutOfRange(comp, "fmi2SetReal", vr[i], realSignalCount) )
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetReal: #r%d# = %.16g", vr[i], value[i])

		//comp->r[vr[i]] = value[i];
		// status = CFMI2Interface::getInstance().setReal(comp, vr[i], value[i]);
    }
    status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->setReal(comp, vr, nvr, value);
    if (nvr > 0) comp->isDirtyValues = fmi2True;
    return status;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
	size_t i;
	fmi2Status status = fmi2OK;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetInteger", MASK_fmi2SetInteger))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "vr[]", vr))
        return fmi2Error;
    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: nvr = %d", nvr)
	unsigned int intSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofIntegers();
	for (i = 0; i < nvr; ++i) {
        if ( vrOutOfRange(comp, "fmi2SetInteger", vr[i], intSignalCount) )
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetInteger: #i%d# = %d", vr[i], value[i])

        //comp->i[vr[i]] = value[i];
		// status = CFMI2Interface::getInstance().setInteger(comp, vr[i], value[i]);
    }
    status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->setInteger(comp, vr, nvr, value);
    if (nvr > 0) comp->isDirtyValues = fmi2True;
    return status;
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
	size_t i;
    ModelInstance *comp = (ModelInstance *)c;
	fmi2Status status = fmi2OK;
    if (invalidState(comp, "fmi2SetBoolean", MASK_fmi2SetBoolean))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "vr[]", vr))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: nvr = %d", nvr)

    unsigned int boolSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofBooleans();
    for (i = 0; i < nvr; ++i) {
        if ( vrOutOfRange(comp, "fmi2SetBoolean", vr[i], boolSignalCount) )
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetBoolean: #b%d# = %s", vr[i], value[i] ? "true" : "false")

        //comp->b[vr[i]] = value[i];
		//status = CFMI2Interface::getInstance().setBoolean(comp, vr[i], value[i]);
    }
    status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->setBoolean(comp, vr, nvr, value);
    if (nvr > 0) comp->isDirtyValues = fmi2True;
    return status;
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
	size_t i;
	fmi2Status status = fmi2OK;
    ModelInstance *comp = (ModelInstance *)c;
	if (invalidState(comp, "fmi2SetString", MASK_fmi2SetString))
		return fmi2Error;
	if (nvr==0)
		return fmi2OK;
    if (nvr>0 && nullPointer(comp, "fmi2SetString", "vr[]", vr))
        return fmi2Error;
    if (nvr>0 && nullPointer(comp, "fmi2SetString", "value[]", value))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetString: nvr = %d", nvr)

	unsigned int stringSignalCount = CFMI2Interface::getInstance(comp->instanceName).getNumberofStrings();
    for (i = 0; i < nvr; ++i) {
        char *string = (char *)comp->s[vr[i]];
		if (vrOutOfRange(comp, "fmi2SetString", vr[i], stringSignalCount))
            return fmi2Error;
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetString: #s%d# = '%s'", vr[i], value[i])

        if (value[i] == NULL) {
            if (string) comp->functions->freeMemory(string);
            comp->s[vr[i]] = NULL;
            FILTERED_LOG(comp, fmi2Warning, LOG_ERROR, "fmi2SetString: string argument value[%d] = NULL.", i);

        } else {
            if (string == NULL || strlen(string) < strlen(value[i])) {
                // cppcheck-suppress nullPointerRedundantCheck
                if (string) comp->functions->freeMemory(string);
                comp->s[vr[i]] = (char *)comp->functions->allocateMemory(1 + strlen(value[i]), sizeof(char));
                if (!comp->s[vr[i]]) {
                    comp->state = modelError;
                    FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "fmi2SetString: Out of memory.")

                    return fmi2Error;
                }
            }
            //strcpy((char *)comp->s[vr[i]], (char *)value[i]);
			//status = CFMI2Interface::getInstance().setString(comp, vr[i], value[i]);
        }
    }
    status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->setString(comp, vr, nvr, value);
    if (nvr > 0) comp->isDirtyValues = fmi2True;
    return status;
}

fmi2Status fmi2SetBinary( fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const size_t sizes[], const fmi2Binary value[], size_t nvalues )
{
    size_t i;
    fmi2Status status = fmi2OK;
    ModelInstance *comp = (ModelInstance *) c;
    if ( invalidState( comp, "fmi2SetBinary", MASK_fmi2SetBinary ) )
        return fmi2Error;
    if ( nvr>0 && nullPointer( comp, "fmi2SetBinary", "vr[]", vr ) )
        return fmi2Error;
    if ( nvr>0 && nullPointer( comp, "fmi2SetBinary", "value[]", value ) )
        return fmi2Error;
    FILTERED_LOG( comp, fmi2OK, LOG_FMI_CALL, std::string( "fmi2SetBinary: nvr = " ).append( std::to_string( nvr ) ).c_str( ) )

        for ( i = 0; i < nvr; ++i ) {
            char* binary = (char*) comp->bi[vr[i]];
            //if ( vrOutOfRange( comp, "fmi2SetBinary", vr[i], CFMI2Interface::getInstance( ).getNumberofStrings( ) ) )
            //    return fmi2Error;
            FILTERED_LOG( comp, fmi2OK, LOG_FMI_CALL, std::string( "fmi2SetBinary: " ).append( std::to_string( vr[i] ) ).append( " = " ).append((char*) value[i] ).c_str( ) )

                if ( value[i] == NULL ) {
                    if ( binary ) comp->functions->freeMemory( binary );
                    comp->s[vr[i]] = NULL;
                    FILTERED_LOG( comp, fmi2Warning, LOG_ERROR, std::string( "fmi2SetBinary: binary argument value[" ).append( std::to_string( i ) ).append( "] = NULL." ).c_str( ) )

                }
                else {
                    if ( binary == NULL || strlen( (char*)binary ) <  sizes[i] ) {
                        // cppcheck-suppress nullPointerRedundantCheck
                        if ( binary ) comp->functions->freeMemory(  binary );
                        comp->bi[vr[i]] = comp->functions->allocateMemory( 1 + sizes[i], sizeof( char ) );
                        if ( !comp->bi[vr[i]] ) {
                            comp->state = modelError;
                            FILTERED_LOG( comp, fmi2Error, LOG_ERROR, "fmi2SetBinary: Out of memory." )

                                return fmi2Error;
                        }
                    }
                    //strcpy((char *)comp->s[vr[i]], (char *)value[i]);
                    //status = CFMI3Interface::getInstance().setString(comp, vr[i], value[i]);
                }
        }
    status = CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->setBinary(comp, vr, nvr, sizes, value);
    if ( nvr > 0 ) comp->isDirtyValues = fmi2True;
    return status;
}

fmi2Status fmi2GetSignalStream( fmi2Component c, char** signalstream, fmi2Integer* databytes )
{
    ModelInstance *comp = (ModelInstance *) c;

    return CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->getSerializedsignalStream(comp, signalstream, databytes);
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) 
{
	ModelInstance *comp = (ModelInstance *)c;
	if (invalidState(comp, "fmi2GetFMUstate", MASK_fmi2GetFMUstate))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetFMUstate");
	if (FMUstate)
	{
		if (!(*FMUstate))
		{
			ModelState *FMUstatePtr = (ModelState *)comp->functions->allocateMemory(1, sizeof(ModelState));
			*FMUstatePtr = comp->state;
			*((ModelState **)FMUstate) = FMUstatePtr;
		}
		else
		{
			*((ModelState *)*((ModelState **)FMUstate)) = comp->state;
		}
		return fmi2OK;
	}
	return fmi2Error;
}

fmi2Status fmi2SetFMUstate (fmi2Component c, fmi2FMUstate FMUstate)
{
	ModelInstance *comp = (ModelInstance *)c;
	ModelState *newState = (ModelState *)FMUstate;
	if (invalidState(comp, "fmi2SetFMUstate", MASK_fmi2SetFMUstate))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetFMUstate")
	if (FMUstate && comp->state)
	{
		comp->state = *newState;
		return fmi2OK;
	}
	return fmi2Error;
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate)
{
	ModelInstance *comp = (ModelInstance *)c;
	if (invalidState(comp, "fmi2FreeFMUstate", MASK_fmi2FreeFMUstate))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2FreeFMUstate");
	if (FMUstate)
	{
		if (*FMUstate)
		{
			comp->functions->freeMemory(*FMUstate);
			*FMUstate = nullptr;
		}
		return fmi2OK;
	}
	return fmi2Error;
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size)
{
	ModelInstance *comp = (ModelInstance *)c;
	if (invalidState(comp, "fmi2SerializedFMUstateSize", MASK_fmi2SerializedFMUstateSize))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SerializedFMUstateSize");
	if ((nullptr != FMUstate) && (nullptr != size))
	{
		std::stringstream stateStr;
		stateStr << *(ModelState *)FMUstate;
		*size = stateStr.str().size();

		return fmi2OK;
	}
	return fmi2Error;
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size)
{
	ModelInstance *comp = (ModelInstance *)c;
	ModelState *state = (ModelState *)FMUstate;
	if (invalidState(comp, "fmi2SerializeFMUstate", MASK_fmi2SerializeFMUstate))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SerializeFMUstate");
	if (nullptr != state)
	{
		std::stringstream stateStr;
		stateStr << *(ModelState *)FMUstate;

		if (size != stateStr.str().size())
		{
			FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "Actual and Received serializedState Size is mismatched");
			return fmi2Error;
		}

		memcpy(serializedState, stateStr.str().c_str(), stateStr.str().size());

		return fmi2OK;
	}
	return fmi2Error;
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate)
{
	ModelInstance *comp = (ModelInstance *)c;
	ModelState *state = nullptr;
	if (invalidState(comp, "fmi2DeSerializeFMUstate", MASK_fmi2DeSerializeFMUstate))
		return fmi2Error;
	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2DeSerializeFMUstate");
	if (nullptr != FMUstate)
	{
		if (nullptr == (*FMUstate))
		{
			ModelState *FMUstatePtr = (ModelState *)comp->functions->allocateMemory(1, sizeof(ModelState));
			state = FMUstatePtr;
			*((ModelState **)FMUstate) = FMUstatePtr;
		}
		else
		{
			state = ((ModelState *)*((ModelState **)FMUstate));
		}

		if (nullptr != state)
		{
			std::stringstream stateStr(serializedState);
			int stateValue = 0;

			if (size != stateStr.str().size())
			{
				FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "Actual and Received serializedState Size is mismatched");
				return fmi2Error;
			}

			stateStr >> stateValue;
			*state = (ModelState)stateValue;

			return fmi2OK;
		}
	}
	return fmi2Error;
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[] , size_t nKnown,
                                        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
    return unsupportedFunction(c, "fmi2GetDirectionalDerivative", MASK_fmi2GetDirectionalDerivative);
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
#ifdef FMI_COSIMULATION
/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer order[], const fmi2Real value[]) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetRealInputDerivatives", MASK_fmi2SetRealInputDerivatives)) {
        return fmi2Error;
    }
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetRealInputDerivatives: nvr= %d", nvr)
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR, "fmi2SetRealInputDerivatives: ignoring function call."
        " This model cannot interpolate inputs: canInterpolateInputs=\"fmi2False\"")

    return fmi2Error;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                      const fmi2Integer order[], fmi2Real value[]) {
	size_t i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetRealOutputDerivatives", MASK_fmi2GetRealOutputDerivatives))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetRealOutputDerivatives: nvr= %d", nvr)
    FILTERED_LOG(comp, fmi2Error, LOG_ERROR,"fmi2GetRealOutputDerivatives: ignoring function call."
        " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"")

    for (i = 0; i < nvr; i++) value[i] = 0;
    return fmi2Error;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2CancelStep", MASK_fmi2CancelStep)) {
        // always fmi2CancelStep is invalid, because model is never in modelStepInProgress state.
        return fmi2Error;
    }
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2CancelStep")

    comp->state = modelStepCanceled;
    return fmi2Error;
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
	fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint)
{
	ModelInstance *comp = (ModelInstance *)c;
	//comp->state = modelStepInProgress;
	if (invalidState(comp, "fmi2DoStep", MASK_fmi2DoStep))
		return fmi2Error;

	FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2DoStep: "
		"currentCommunicationPoint = %g, "
		"communicationStepSize = %g, "
		"noSetFMUStatePriorToCurrentPoint = fmi2%s",
		currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint ? "True" : "False")

		if (communicationStepSize < 0)
		{
			FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
				"fmi2DoStep: communication step size must be > 0. Fount %g.", communicationStepSize)

				comp->state = modelError;
			return fmi2Error;
		}

	//

	int valueMicroSec = int(communicationStepSize * 1000 * 1000);
    if ( fmi2Error == CFMI2Interface::getInstance(comp->instanceName).m_com_ptr->doStep(comp, std::chrono::microseconds(valueMicroSec)) )
		return fmi2Error;

	//in case we terminate the loop(discard and not error, this will terminate the loop not send a fatal error to the simulation)
	if (comp->eventInfo.terminateSimulation)
	{
		comp->eventInfo.terminateSimulation = fmi2True;
		return fmi2Discard;
	}
	comp->state = modelStepComplete;
	return fmi2OK;
}

/* Inquire slave status */
static fmi2Status getStatus(char* fname, fmi2Component c, const fmi2StatusKind s) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, fname, MASK_fmi2GetStatus)) // all get status have the same MASK_fmi2GetStatus
            return fmi2Error;

    // cppcheck-suppress variableScope
	const char *statusKind[3] = { "fmi2DoStepStatus","fmi2PendingStatus","fmi2LastSuccessfulTime" };
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "$s: fmi2StatusKind = %s", fname, statusKind[s])

    switch(s) {
        case fmi2DoStepStatus: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2DoStepStatus when fmi2DoStep returned fmi2Pending."
            " This is not the case.", fname)

            break;
        case fmi2PendingStatus: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2PendingStatus when fmi2DoStep returned fmi2Pending."
            " This is not the case.", fname)

            break;
        case fmi2LastSuccessfulTime: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2LastSuccessfulTime when fmi2DoStep returned fmi2Discard."
            " This is not the case.", fname)

            break;
        case fmi2Terminated: FILTERED_LOG(comp, fmi2Error, LOG_ERROR,
            "%s: Can be called with fmi2Terminated when fmi2DoStep returned fmi2Discard."
            " This is not the case.", fname)

            break;
    }
    return fmi2Discard;
}

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value) {
    return getStatus((char*)"fmi2GetStatus", c, s);
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value) {
    if (s == fmi2LastSuccessfulTime) {
        ModelInstance *comp = (ModelInstance *)c;
        if (invalidState(comp, "fmi2GetRealStatus", MASK_fmi2GetRealStatus))
            return fmi2Error;
        *value = comp->time;
        return fmi2OK;
    }
    return getStatus((char*)"fmi2GetRealStatus", c, s);
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value) {
    return getStatus((char*)"fmi2GetIntegerStatus", c, s);
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value) {
    if (s == fmi2Terminated) {
        ModelInstance *comp = (ModelInstance *)c;
        if (invalidState(comp, "fmi2GetBooleanStatus", MASK_fmi2GetBooleanStatus))
            return fmi2Error;
        *value = comp->eventInfo.terminateSimulation;
        return fmi2OK;
    }
    return getStatus((char*)"fmi2GetBooleanStatus", c, s);
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value) {
    return getStatus((char*)"fmi2GetStringStatus", c, s);
}

// ---------------------------------------------------------------------------
// Functions for FMI2 for Model Exchange
// ---------------------------------------------------------------------------
#else
/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterEventMode", MASK_fmi2EnterEventMode))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2EnterEventMode")

    comp->state = modelEventMode;
    comp->isNewEventIteration = fmi2True;
    return fmi2OK;
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {
    ModelInstance *comp = (ModelInstance *)c;
    int timeEvent = 0;
    if (invalidState(comp, "fmi2NewDiscreteStates", MASK_fmi2NewDiscreteStates))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2NewDiscreteStates")

    comp->eventInfo.newDiscreteStatesNeeded = fmi2False;
    comp->eventInfo.terminateSimulation = fmi2False;
    comp->eventInfo.nominalsOfContinuousStatesChanged = fmi2False;
    comp->eventInfo.valuesOfContinuousStatesChanged = fmi2False;

    if (comp->eventInfo.nextEventTimeDefined && comp->eventInfo.nextEventTime <= comp->time) {
        timeEvent = 1;
    }
    eventUpdate(comp, &comp->eventInfo, timeEvent, comp->isNewEventIteration);
    comp->isNewEventIteration = fmi2False;

    // copy internal eventInfo of component to output eventInfo
    eventInfo->newDiscreteStatesNeeded = comp->eventInfo.newDiscreteStatesNeeded;
    eventInfo->terminateSimulation = comp->eventInfo.terminateSimulation;
    eventInfo->nominalsOfContinuousStatesChanged = comp->eventInfo.nominalsOfContinuousStatesChanged;
    eventInfo->valuesOfContinuousStatesChanged = comp->eventInfo.valuesOfContinuousStatesChanged;
    eventInfo->nextEventTimeDefined = comp->eventInfo.nextEventTimeDefined;
    eventInfo->nextEventTime = comp->eventInfo.nextEventTime;

    return fmi2OK;
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterContinuousTimeMode", MASK_fmi2EnterContinuousTimeMode))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL,"fmi2EnterContinuousTimeMode")

    comp->state = modelContinuousTimeMode;
    return fmi2OK;
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2CompletedIntegratorStep", MASK_fmi2CompletedIntegratorStep))
        return fmi2Error;
    if (nullPointer(comp, "fmi2CompletedIntegratorStep", "enterEventMode", enterEventMode))
        return fmi2Error;
    if (nullPointer(comp, "fmi2CompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL,"fmi2CompletedIntegratorStep")
    *enterEventMode = fmi2False;
    *terminateSimulation = fmi2False;
    return fmi2OK;
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetTime", MASK_fmi2SetTime))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetTime: time=%.16g", time)
    comp->time = time;
    return fmi2OK;
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetContinuousStates", MASK_fmi2SetContinuousStates))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2SetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmi2SetContinuousStates", "x[]", x))
        return fmi2Error;
#if NUMBER_OF_REALS>0
	int i;
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2SetContinuousStates: #r%d#=%.16g", vr, x[i])
        assert(vr < NUMBER_OF_REALS);
        comp->r[vr] = x[i];
    }
#endif
    return fmi2OK;
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {
    int i;
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetDerivatives", MASK_fmi2GetDerivatives))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2GetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmi2GetDerivatives", "derivatives[]", derivatives))
        return fmi2Error;
#if NUMBER_OF_STATES>0
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i] + 1;
        derivatives[i] = getReal(comp, vr); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetDerivatives: #r%d# = %.16g", vr, derivatives[i])
    }
#endif
    return fmi2OK;
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetEventIndicators", MASK_fmi2GetEventIndicators))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2GetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmi2Error;
#if NUMBER_OF_EVENT_INDICATORS>0
    for (i = 0; i < ni; i++) {
        eventIndicators[i] = getEventIndicator(comp, i); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetEventIndicators: z%d = %.16g", i, eventIndicators[i])
    }
#endif
    return fmi2OK;
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetContinuousStates", MASK_fmi2GetContinuousStates))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2GetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmi2GetContinuousStates", "states[]", states))
        return fmi2Error;
#if NUMBER_OF_REALS>0
	int i;
    for (i = 0; i < nx; i++) {
        fmi2ValueReference vr = vrStates[i];
        states[i] = getReal(comp, vr); // to be implemented by the includer of this file
        FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetContinuousStates: #r%u# = %.16g", vr, states[i])
    }
#endif
    return fmi2OK;
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetNominalsOfContinuousStates", MASK_fmi2GetNominalsOfContinuousStates))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2GetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmi2GetNominalContinuousStates", "x_nominal[]", x_nominal))
        return fmi2Error;
    FILTERED_LOG(comp, fmi2OK, LOG_FMI_CALL, "fmi2GetNominalContinuousStates: x_nominal[0..%d] = 1.0", nx-1)
    for (i = 0; i < nx; i++)
        x_nominal[i] = 1;
    return fmi2OK;
}
#endif // Model Exchange

#ifdef __cplusplus
} // closing brace for extern "C"
#endif
