#include "AppConfiguration.h"
#include "file_configuration.h"
#include <cstring>
#include <algorithm>
#include <cctype>

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

AppConfiguration AppConfiguration::fromFile(const std::string &fileName) {
    AppConfiguration appConfiguration;
    appConfiguration.configValid = true;
    FileConfiguration fileConfiguration(fileName);

    appConfiguration.dnp3ClientAddress = fileConfiguration.property("dnp3ClientAddress", std::string(""));
    appConfiguration.dnp3ClientPort = fileConfiguration.property("dnp3ClientPort", 0U);
    appConfiguration.dnp3MasterPort = fileConfiguration.property("dnp3MasterPort", 0U);
    appConfiguration.dnp3ClientSerialPort = fileConfiguration.property("dnp3ClientSerialPort", std::string(""));
    appConfiguration.dnp3ClientSerialPortBaudrate = fileConfiguration.property("dnp3ClientSerialPortBaudrate", 0U);
    std::string dnp3ConnectionType = toUpper(fileConfiguration.property("dnp3ConnectionType", std::string("")));
    if (dnp3ConnectionType == std::string("TCP")){
        appConfiguration.dnp3ConnectionType = DNP3_CONNECTION_TCP;
    }
    else if (dnp3ConnectionType == std::string("UDP")){
        appConfiguration.dnp3ConnectionType = DNP3_CONNECTION_UDP;
    }
    else if (dnp3ConnectionType == std::string("SERIAL")){
        appConfiguration.dnp3ConnectionType = DNP3_CONNECTION_SERIAL;
    }   
    else {
        std::cout << std::string("Error in config dnp3ConnectionType") << std::endl;
        appConfiguration.configValid = false;
        return appConfiguration;
    }

    appConfiguration.dnp3Destination = fileConfiguration.property("dnp3Destination", 0U);
    appConfiguration.dnp3Source = fileConfiguration.property("dnp3Source", 0U);
    appConfiguration.dnp3ResponseTimeoutSecond = fileConfiguration.property("dnp3ResponseTimeoutSecond", 0U);
    int numMessage = fileConfiguration.property("numMessage", 0U);
    if (numMessage <= 0)
    {
        appConfiguration.configValid = false;
        return appConfiguration;
    }
    for (int i=0 ; i<numMessage ; i++){
        std::string messageId = std::string("message") + std::to_string(i+1);
        MessageConfig config;
        config.iec104Asdu = fileConfiguration.property(messageId+std::string(".iec104Asdu"), 0U);
        std::string messageType = toUpper(fileConfiguration.property(messageId+std::string(".messageType"), std::string("")));
        if (messageType == std::string("BINARY_INPUT")){
            config.messageType = BINARY_INPUT;
        }
        else if (messageType == std::string("BINARY_OUTPUT")){
            config.messageType = BINARY_OUTPUT;
        }
        else if (messageType == std::string("ANALOG_INPUT")){
            config.messageType = ANALOG_INPUT;
        }
        else if (messageType == std::string("ANALOG_OUTPUT")){
            config.messageType = ANALOG_OUTPUT;
        }
        else if (messageType == std::string("COUNTER")){
            config.messageType = COUNTER;
        }
        else {
            std::cout << std::string("Error in config ") << messageId+std::string(".messageType") << std::endl;
            appConfiguration.configValid = false;
            return appConfiguration;
        }
        config.dnp3Point = fileConfiguration.property(messageId+std::string(".dnp3Point"), static_cast<int>(0));
        config.iec104Ioa = fileConfiguration.property(messageId+std::string(".iec104Ioa"), static_cast<int>(0));
        for (int j=0 ; j<appConfiguration.configData.size() ; j++){
            if ((appConfiguration.configData[j].messageType == config.messageType) 
            && (appConfiguration.configData[j].iec104Asdu == config.iec104Asdu) 
            && (appConfiguration.configData[j].iec104Ioa == config.iec104Ioa)){
                std::cout << std::string("Error in config ") << messageId << " same as message" << j << std::endl;
                appConfiguration.configValid = false;
                return appConfiguration;
            }
        }
        appConfiguration.configData.push_back(config);
    }
    return appConfiguration;
}

bool AppConfiguration::getConfigValid(){
    return configValid;
}