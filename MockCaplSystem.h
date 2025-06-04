#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <dlfcn.h>
#include <cstring>

// Include Vector-provided headers
#include "VIA.h"
#include "VIA_CDLL.h"

using std::string;
using std::unordered_map;
using std::function;

// --- Mock VIACaplFunction Implementation ---
class MockCaplFunction : public VIACaplFunction {
public:
    std::string name;
    std::function<void(void*)> callback;
    std::string paramTypes;
    char returnType;

    MockCaplFunction(const string& n, function<void(void*)> cb,
                    const std::string& ptype)
        : name(n), callback(cb),paramTypes(ptype) {}

    VIASTDDECL ParamSize(int32* size) override {
        *size = 0; // For simplicity
        return kVIA_OK;
    }

    VIASTDDECL ParamCount(int32* size) override {
        if (!size) return kVIA_DBParameterInvalid;
        *size = static_cast<int32>(paramTypes.length());
        return kVIA_OK;
    }

    VIASTDDECL ParamType(char* type, int32 nth) override {
        if (!type || nth < 0 || nth >= static_cast<int32>(paramTypes.length()))
        return kVIA_DBParameterInvalid;
        *type = paramTypes[nth];
        return kVIA_OK;
    }

    VIASTDDECL ResultType(char* type) override {
        strcpy(type, "V"); // Dummy return type
        return kVIA_OK;
    }

    VIASTDDECL Call(uint32* result, void* params) override {
        std::cout << "Mock: Calling CAPL function " << name << std::endl;
        if (callback) callback(params);
        *result = 0;
        return kVIA_OK;
    }

    VIASTDDECL CallReturnsDouble(double* result, void* params) override {
        std::cout << "Mock: Calling CAPL function (returns double) " << name << std::endl;
        if (callback) callback(params);
        *result = 0.0;
        return kVIA_OK;
    }
};

// --- Mock VIACapl Implementation ---
class MockCapl : public VIACapl {
public:
    unordered_map<string, MockCaplFunction*> functionMap;

    MockCapl() {
        // Register mock CAPL callback functions
        registerFunction("CALLBACK_WriteEthFrame", [](void* params) 
        {
            std::cout << "Executing mock CALLBACK_WriteEthFrame" << std::endl;
            // You could cast params to your struct if needed
        },"DUUDDDB");

        registerFunction("CALLBACK_WriteCanFrame", [](void* params) 
        {
            std::cout << "Executing mock CALLBACK_WriteCanFrame" << std::endl;
        },"DDDDDDDDDDB");

        registerFunction("CALLBACK_WriteFlexrayFrame", [](void* params) 
        {
            std::cout << "Executing mock CALLBACK_WriteFlexrayFrame" << std::endl;
        },"DDDDDDBDD");
    }

    void registerFunction(const string& name, function<void(void*)> func, std::string paramTypes) 
    {
        functionMap[name] = new MockCaplFunction(name, func,paramTypes);
    }

    VIASTDDECL GetVersion(int32* major, int32* minor) override 
    {
        *major = 1;
        *minor = 0;
        return kVIA_OK;
    }

    VIASTDDECL GetCaplHandle(uint32* handle) override 
    {
        *handle = 0xBEEF; // Dummy handle
        return kVIA_OK;
    }

    VIASTDDECL GetCaplFunction(VIACaplFunction** caplfct, const char* functionName) override 
    {
        auto it = functionMap.find(functionName);
        if (it != functionMap.end()) {
            *caplfct = it->second;
            std::cout << "Mock: Provided function handle for " << functionName << std::endl;
            return kVIA_OK;
        }
        std::cerr << "Mock: Function not found: " << functionName << std::endl;
        return kVIA_FunctionNotImplemented;
    }

    VIASTDDECL ReleaseCaplFunction(VIACaplFunction* caplfct) override 
    {
        std::cout << "Mock: Released function handle" << std::endl;
        return kVIA_OK;
    }
};
