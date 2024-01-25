#ifndef IEC104MASTERDNP3MASTER_H
#define IEC104MASTERDNP3MASTER_H

#include <iostream>
#include <condition_variable>
#include <opendnp3/ConsoleLogger.h>
#include <opendnp3/DNP3Manager.h>
#include <opendnp3/channel/PrintingChannelListener.h>
#include <opendnp3/logging/LogLevels.h>
#include <opendnp3/master/DefaultMasterApplication.h>
#include <opendnp3/master/PrintingCommandResultCallback.h>
#include <opendnp3/master/PrintingSOEHandler.h>
#include <opendnp3/master/ISOEHandler.h>
#include "AppConfiguration.h"
extern "C" {
    #include "cs104_slave.h"
}

using namespace std;
using namespace opendnp3;
using namespace std::chrono_literals;

class Iec104MasterDnp3MasterMessageConfig {
public:
    enum Iec104MasterDnp3MasterMessageType {BINARY_INPUT, BINARY_OUTPUT, ANALOG_INPUT, ANALOG_OUTPUT, COUNTER, TIMESYNC};
    enum Iec104MasterDnp3MasterMessageState {WAIT_CLIENT, RESPONSE_SUCCESS, RESPONSE_FAILED};
    Iec104MasterDnp3MasterMessageType messageType;
    Iec104MasterDnp3MasterMessageState state = WAIT_CLIENT;
    int dnp3Point;
    int iec104TypeId;
    int iec104Asdu;
    int iec104Ioa;
    bool binaryValue;
    int analogValue;
    int counterValue;
    uint64_t timeMs;
};

class Iec104MasterDnp3MasterPendingCommand {
public:
    std::mutex lock;
    std::vector<Iec104MasterDnp3MasterMessageConfig> binaryInput;
    std::vector<Iec104MasterDnp3MasterMessageConfig> binaryOutput;
    std::vector<Iec104MasterDnp3MasterMessageConfig> analogInput;
    std::vector<Iec104MasterDnp3MasterMessageConfig> analogOutput;
    std::vector<Iec104MasterDnp3MasterMessageConfig> counter;
    std::vector<Iec104MasterDnp3MasterMessageConfig> timeSync;
    bool done;
    int pendingCommandCounter;
    std::condition_variable sync;
};

class Iec104MasterDnp3Master
{
public:
    Iec104MasterDnp3Master() = default;
    ~Iec104MasterDnp3Master() {
        if (dnp3Manager != nullptr){
            delete dnp3Manager;
        }
        if (iec104Slave != nullptr){
            CS104_Slave_destroy(iec104Slave);
        }
    };
    bool setConfigs(AppConfiguration configs);
    bool init();
    bool start();
    Iec104MasterDnp3MasterPendingCommand pendingCommands;
private:
    std::vector<Iec104MasterDnp3MasterMessageConfig> messageConfig;
    uint8_t dnp3Destination;
    uint8_t dnp3Source;
    int dnp3ResponseTimeoutSecond;
    std::string dnp3ClientAddress;
    int dnp3ClientPort;
    int dnp3MasterPort;
    DNP3ConnectionType dnp3ConnectionType;
    std::string dnp3ClientSerialPort;
    int dnp3ClientSerialPortBaudrate;

    DNP3Manager *dnp3Manager = nullptr;
    std::shared_ptr<IChannel> dnp3Channel;
    std::shared_ptr<ISOEHandler> dnp3IsoeHandler;
    MasterStackConfig dnp3MasterStackConfig;
    std::shared_ptr<IMaster> dnp3Master;

    CS104_Slave iec104Slave = nullptr;
    const int iec104MaxLowPrioQueueSize = 10;
    const int iec104MaxHighPrioQueueSize = 10;
    const std::string iec104LocalAddress = std::string("0.0.0.0");
    const CS104_ServerMode iec104ServerMode = CS104_MODE_SINGLE_REDUNDANCY_GROUP;
    CS101_AppLayerParameters iec104AppLayerParameters;
    CS104_APCIParameters iec104AcpiParameters;

    static bool iec104ClockSyncHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);
    static bool iec104InterrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi);
    static bool iec104CounterInterrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, QualifierOfCIC qcc);
    static bool iec104AsduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu);
    static bool iec104ConnectionRequestHandler(void* parameter, const char* ipAddress);
    static void iec104ConnectionEventHandler(void* parameter, IMasterConnection con, CS104_PeerConnectionEvent event);
    bool processingCommand();
};
#endif

