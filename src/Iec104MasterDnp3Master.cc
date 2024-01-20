#include "Iec104MasterDnp3Master.h"
#include "Dnp3ISOEHandler.h"
extern "C" {
    #include "hal_time.h"
}

bool Iec104MasterDnp3Master::setConfigs(AppConfiguration configs){
    dnp3Destination = configs.dnp3Destination;
    dnp3Source = configs.dnp3Source;
    dnp3ResponseTimeoutSecond = configs.dnp3ResponseTimeoutSecond;
    dnp3ClientAddress = configs.dnp3ClientAddress;
    dnp3ClientPort = configs.dnp3ClientPort;
    dnp3MasterPort = configs.dnp3MasterPort;
    dnp3ConnectionType = configs.dnp3ConnectionType;
    dnp3ClientSerialPort = configs.dnp3ClientSerialPort;
    dnp3ClientSerialPortBaudrate = configs.dnp3ClientSerialPortBaudrate;
    for (int i=0 ; i<configs.configData.size() ; i++)
    {
        Iec104MasterDnp3MasterMessageConfig messageConfig;
        switch (configs.configData[i].messageType)
        {
        case MessageType::BINARY_INPUT:
        {
            messageConfig.messageType = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::BINARY_INPUT;
            break;
        }     
        case MessageType::BINARY_OUTPUT:
        {
            messageConfig.messageType = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::BINARY_OUTPUT;
            break;
        }
        case MessageType::ANALOG_INPUT:
        {
            messageConfig.messageType = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::ANALOG_INPUT;
            break;
        }
        case MessageType::ANALOG_OUTPUT:
        {
            messageConfig.messageType = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::ANALOG_OUTPUT;
            break;
        }
        case MessageType::COUNTER:
        {
            messageConfig.messageType = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::COUNTER;
            break;
        }
        default:
            break;
        } 
        messageConfig.dnp3Point = configs.configData[i].dnp3Point;
        messageConfig.iec104TypeId = configs.configData[i].iec104TypeId;
        messageConfig.iec104Asdu = configs.configData[i].iec104Asdu;
        messageConfig.iec104Ioa = configs.configData[i].iec104Ioa;
        this->messageConfig.push_back(messageConfig);
    }
    return true;
}

bool Iec104MasterDnp3Master::init(){
    dnp3Manager = new DNP3Manager(1, ConsoleLogger::Create());
    switch (dnp3ConnectionType)
    {
    case DNP3ConnectionType::DNP3_CONNECTION_SERIAL:
    {
        SerialSettings serialSettings;
        serialSettings.deviceName = dnp3ClientSerialPort;
        serialSettings.baud = dnp3ClientSerialPortBaudrate;
        serialSettings.flowType = FlowControl::None;
        serialSettings.dataBits = 8;
        serialSettings.parity = Parity::None;
        serialSettings.stopBits = StopBits::One;
        dnp3Channel = dnp3Manager->AddSerial("serialclient", levels::NOTHING, ChannelRetry::Default(), serialSettings, PrintingChannelListener::Create());
        break;
    } 
    case DNP3ConnectionType::DNP3_CONNECTION_TCP:
    {
        dnp3Channel = dnp3Manager->AddTCPClient("tcpclient", levels::NOTHING, ChannelRetry::Default(), {IPEndpoint(dnp3ClientAddress.data(), dnp3ClientPort)},
                                        "0.0.0.0", PrintingChannelListener::Create());
        break;
    }
    case DNP3ConnectionType::DNP3_CONNECTION_UDP:
    {
        std::cout << "Temporary not support UDP" << std::endl;
        return false;
        break;
    } 
    default:
        break;
    } 
    dnp3MasterStackConfig.master.responseTimeout = TimeDuration::Seconds(dnp3ResponseTimeoutSecond);
    dnp3MasterStackConfig.master.disableUnsolOnStartup = false;
    dnp3MasterStackConfig.link.LocalAddr = dnp3Source;
    dnp3MasterStackConfig.link.RemoteAddr = dnp3Destination;
    dnp3Master = dnp3Channel->AddMaster("master", PrintingSOEHandler::Create(), DefaultMasterApplication::Create(), dnp3MasterStackConfig);
    dnp3IsoeHandler = std::make_shared<Dnp3ISOEHandler>();
    static_cast<Dnp3ISOEHandler*>(dnp3IsoeHandler.get())->setIec104MasterDnp3Master(this);
    
    iec104Slave = CS104_Slave_create(iec104MaxLowPrioQueueSize, iec104MaxHighPrioQueueSize);
    CS104_Slave_setLocalAddress(iec104Slave, "0.0.0.0");
    CS104_Slave_setServerMode(iec104Slave, iec104ServerMode);
    iec104AppLayerParameters = CS104_Slave_getAppLayerParameters(iec104Slave);
    iec104AcpiParameters = CS104_Slave_getConnectionParameters(iec104Slave);
    CS104_Slave_setClockSyncHandler(iec104Slave, iec104ClockSyncHandler, this);
    CS104_Slave_setInterrogationHandler(iec104Slave, iec104InterrogationHandler, this);
    CS104_Slave_setCounterInterrogationHandler(iec104Slave, iec104CounterInterrogationHandler, this);
    CS104_Slave_setASDUHandler(iec104Slave, iec104AsduHandler, this);
    CS104_Slave_setConnectionRequestHandler(iec104Slave, iec104ConnectionRequestHandler, this);
    CS104_Slave_setConnectionEventHandler(iec104Slave, iec104ConnectionEventHandler, this);
}

bool Iec104MasterDnp3Master::start(){
    dnp3Master->Enable();
    CS104_Slave_start(iec104Slave);
}

bool Iec104MasterDnp3Master::iec104ClockSyncHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime){
    Iec104MasterDnp3Master *iec104MasterDnp3Master = static_cast<Iec104MasterDnp3Master*>(parameter);
    std::cout << "Process time sync command with time " << CP56Time2a_getHour(newTime)
                                                        << CP56Time2a_getMinute(newTime)
                                                        << CP56Time2a_getSecond(newTime)
                                                        << CP56Time2a_getDayOfMonth(newTime)
                                                        << CP56Time2a_getMonth(newTime)
                                                        << CP56Time2a_getYear(newTime) + 2000 << std::endl;

    uint64_t newSystemTimeInMs = CP56Time2a_toMsTimestamp(newTime);

    /* Set time for ACT_CON message */
    CP56Time2a_setFromMsTimestamp(newTime, Hal_getTimeInMs());

    /* update system time here */
    return true;
}

bool Iec104MasterDnp3Master::iec104InterrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi){
    Iec104MasterDnp3Master *iec104MasterDnp3Master = static_cast<Iec104MasterDnp3Master*>(parameter);
    if (qoi == 20) { /* only handle station interrogation */
        CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
        iec104MasterDnp3Master->pendingCommands.lock.try_lock();
        iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
        iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
        iec104MasterDnp3Master->pendingCommands.analogInput.clear();
        iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
        iec104MasterDnp3Master->pendingCommands.counter.clear();
        iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
        iec104MasterDnp3Master->pendingCommands.done = true;
        for (int i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++)
        {
            if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::BINARY_INPUT){
                Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                iec104MasterDnp3Master->pendingCommands.binaryInput.push_back(messageConfig);
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                iec104MasterDnp3Master->pendingCommands.done = false;
            }
            if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::ANALOG_INPUT){
                Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                iec104MasterDnp3Master->pendingCommands.analogInput.push_back(messageConfig);
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                iec104MasterDnp3Master->pendingCommands.done = false;
            }
        }
        
        // auto endTime = std::chnoro::now() + std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond);
        iec104MasterDnp3Master->processingCommand();
        std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
        if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                    std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                    [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                    == true){
            IMasterConnection_sendACT_CON(connection, asdu, false);
            if (iec104MasterDnp3Master->pendingCommands.binaryInput.size() > 0){
                bool sameAsdu = true;
                bool isSequence = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu){
                        sameAsdu = false;
                    }
                    if (iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Ioa 
                        != (iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Ioa + i)){
                        isSequence = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu;
                    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu, false, false);
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++) {
                        InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Ioa,
                                                                iec104MasterDnp3Master->pendingCommands.binaryInput[i].binaryValue,
                                                                IEC60870_QUALITY_GOOD);
                        while (CS101_ASDU_addInformationObject(newAsdu, io) == false){
                            IMasterConnection_sendASDU(connection, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                            newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu, false, false);
                        }
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                    }
                    IMasterConnection_sendASDU(connection, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                }
                else {
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++) {
                        int commonAddress = iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu;
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu, false, false);
                        InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Ioa,
                                                                iec104MasterDnp3Master->pendingCommands.binaryInput[i].binaryValue,
                                                                IEC60870_QUALITY_GOOD);
                        CS101_ASDU_addInformationObject(newAsdu, io);
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }
            if (iec104MasterDnp3Master->pendingCommands.analogInput.size() > 0){
                bool sameAsdu = true;
                bool isSequence = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu){
                        sameAsdu = false;
                    }
                    if (iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Ioa 
                        != (iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Ioa + i)){
                        isSequence = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu;
                    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu, false, false);
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++) {
                        InformationObject io = (InformationObject) MeasuredValueScaled_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Ioa,
                                                                iec104MasterDnp3Master->pendingCommands.analogInput[i].analogValue,
                                                                IEC60870_QUALITY_GOOD);
                        while (CS101_ASDU_addInformationObject(newAsdu, io) == false){
                            IMasterConnection_sendASDU(connection, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                            newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu, false, false);
                        }
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                    }
                    IMasterConnection_sendASDU(connection, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                }
                else {
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++) {
                        int commonAddress = iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu;
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu, false, false);
                        InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Ioa,
                                                                iec104MasterDnp3Master->pendingCommands.analogInput[i].analogValue,
                                                                IEC60870_QUALITY_GOOD);
                        CS101_ASDU_addInformationObject(newAsdu, io);
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }     
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_CON(connection, asdu, true);
        }
        iec104MasterDnp3Master->pendingCommands.lock.unlock();
    }
    else {
        IMasterConnection_sendACT_CON(connection, asdu, true);
    }
    
    return true;
}

bool Iec104MasterDnp3Master::iec104CounterInterrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, QualifierOfCIC qcc){
    Iec104MasterDnp3Master *iec104MasterDnp3Master = static_cast<Iec104MasterDnp3Master*>(parameter);
    /*if (qcc == 20) */{ /* only handle station interrogation */
        CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
        iec104MasterDnp3Master->pendingCommands.lock.try_lock();
        iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
        iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
        iec104MasterDnp3Master->pendingCommands.analogInput.clear();
        iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
        iec104MasterDnp3Master->pendingCommands.counter.clear();
        iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
        iec104MasterDnp3Master->pendingCommands.done = true;
        for (int i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++)
        {
            if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::COUNTER){
                Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                iec104MasterDnp3Master->pendingCommands.counter.push_back(messageConfig);
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                iec104MasterDnp3Master->pendingCommands.done = false;
            }
        }
        // auto endTime = std::chnoro::now() + std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond);
        iec104MasterDnp3Master->processingCommand();
        std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
        if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                    std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                    [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                    == true){
            if (iec104MasterDnp3Master->pendingCommands.counter.size() > 0){
                bool sameAsdu = true;
                bool isSequence = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.counter[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu){
                        sameAsdu = false;
                    }
                    if (iec104MasterDnp3Master->pendingCommands.counter[i].iec104Ioa 
                        != (iec104MasterDnp3Master->pendingCommands.counter[0].iec104Ioa + i)){
                        isSequence = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu;
                    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu, false, false);
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++) {
                        BinaryCounterReading binaryCounter = BinaryCounterReading_create(NULL, iec104MasterDnp3Master->pendingCommands.counter[i].counterValue, 0, false, false, false);
                        InformationObject io = (InformationObject) IntegratedTotals_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.counter[i].iec104Ioa,
                                                                binaryCounter);
                        while (CS101_ASDU_addInformationObject(newAsdu, io) == false){
                            IMasterConnection_sendASDU(connection, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                            newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                            0, iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu, false, false);
                        }
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                    }
                    IMasterConnection_sendASDU(connection, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                }
                else {
                    for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++) {
                        int commonAddress = iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu;
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, isSequence, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu, false, false);
                        InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                iec104MasterDnp3Master->pendingCommands.counter[i].iec104Ioa,
                                                                iec104MasterDnp3Master->pendingCommands.counter[i].binaryValue,
                                                                IEC60870_QUALITY_GOOD);
                        CS101_ASDU_addInformationObject(newAsdu, io);
                        SinglePointInformation_destroy(reinterpret_cast<SinglePointInformation>(io));
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_CON(connection, asdu, true);
        }
        iec104MasterDnp3Master->pendingCommands.lock.unlock();
    }
    // else {
    //     IMasterConnection_sendACT_CON(connection, asdu, true);
    // }
    
    return true;
}

bool Iec104MasterDnp3Master::iec104AsduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu){
    Iec104MasterDnp3Master *iec104MasterDnp3Master = static_cast<Iec104MasterDnp3Master*>(parameter);
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);
    if (CS101_ASDU_getTypeID(asdu) == C_SC_NA_1) {
        if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {
            for (int j=0 ; j<CS101_ASDU_getNumberOfElements(asdu) ; j++){
                InformationObject io = CS101_ASDU_getElement(asdu, j);
                // std::cout << "received single command object address " << InformationObject_getObjectAddress(io) << std::endl;
                if (io) {
                    iec104MasterDnp3Master->pendingCommands.lock.try_lock();
                    iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
                    iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
                    iec104MasterDnp3Master->pendingCommands.analogInput.clear();
                    iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
                    iec104MasterDnp3Master->pendingCommands.counter.clear();
                    iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
                    int i;
                    for (i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++)
                    {
                        if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::BINARY_OUTPUT){
                            if (InformationObject_getObjectAddress(io) == iec104MasterDnp3Master->messageConfig[i].iec104Ioa){
                                Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                                SingleCommand sc = (SingleCommand) io;
                                messageConfig.binaryValue = SingleCommand_getState(sc);
                                messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                                iec104MasterDnp3Master->pendingCommands.binaryOutput.push_back(messageConfig);
                                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                                iec104MasterDnp3Master->pendingCommands.done = false;
                                iec104MasterDnp3Master->processingCommand();
                                // auto endTime = std::chnoro::now() + std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond);
                                std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
                                if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                                            std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                                            [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                                            == true){
                                    if (iec104MasterDnp3Master->pendingCommands.binaryOutput[0].state == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS){
                                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, false);
                                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON); 
                                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                                        IMasterConnection_sendASDU(connection, tmpAsdu);
                                        CS101_ASDU_destroy(tmpAsdu);
                                    } else {
                                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON); 
                                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                                        IMasterConnection_sendASDU(connection, tmpAsdu);
                                        CS101_ASDU_destroy(tmpAsdu);
                                    }                                        
                                    iec104MasterDnp3Master->pendingCommands.lock.unlock();
                                }
                                else {
                                    CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                                    CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON);
                                    CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                    CS101_ASDU_addInformationObject(tmpAsdu, io);
                                    IMasterConnection_sendASDU(connection, tmpAsdu);
                                    CS101_ASDU_destroy(tmpAsdu);  
                                }
                                break;
                            }
                        }
                    }
                    iec104MasterDnp3Master->pendingCommands.lock.unlock();
                    InformationObject_destroy(io);
                    if (i==iec104MasterDnp3Master->messageConfig.size()){
                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_UNKNOWN_IOA);
                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                        IMasterConnection_sendASDU(connection, tmpAsdu);
                        CS101_ASDU_destroy(tmpAsdu); 
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
    }
    else if (CS101_ASDU_getTypeID(asdu) == C_SE_NB_1) {
        if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {
            for (int j=0 ; j<CS101_ASDU_getNumberOfElements(asdu) ; j++){
                InformationObject io = CS101_ASDU_getElement(asdu, j);
                // std::cout << "received single command object address " << InformationObject_getObjectAddress(io) << std::endl;
                if (io) {
                    iec104MasterDnp3Master->pendingCommands.lock.try_lock();
                    iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
                    iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
                    iec104MasterDnp3Master->pendingCommands.analogInput.clear();
                    iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
                    iec104MasterDnp3Master->pendingCommands.counter.clear();
                    iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
                    int i;
                    for (i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++)
                    {
                        if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::ANALOG_OUTPUT){
                            if (InformationObject_getObjectAddress(io) == iec104MasterDnp3Master->messageConfig[i].iec104Ioa){
                                Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                                SetpointCommandScaled sc = (SetpointCommandScaled) io;
                                messageConfig.analogValue = SetpointCommandScaled_getValue(sc);
                                messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                                iec104MasterDnp3Master->pendingCommands.analogOutput.push_back(messageConfig);
                                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                                iec104MasterDnp3Master->pendingCommands.done = false;
                                iec104MasterDnp3Master->processingCommand();
                                // auto endTime = std::chnoro::now() + std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond);
                                std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
                                if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                                            std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                                            [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                                            == true){
                                    if (iec104MasterDnp3Master->pendingCommands.analogOutput[0].state == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS){
                                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, false);
                                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON); 
                                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                                        IMasterConnection_sendASDU(connection, tmpAsdu);
                                        CS101_ASDU_destroy(tmpAsdu);
                                    } else {
                                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON); 
                                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                                        IMasterConnection_sendASDU(connection, tmpAsdu);
                                        CS101_ASDU_destroy(tmpAsdu);
                                    }                                        
                                    iec104MasterDnp3Master->pendingCommands.lock.unlock();
                                }
                                else {
                                    CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                                    CS101_ASDU_setCOT(tmpAsdu, CS101_COT_ACTIVATION_CON);
                                    CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                                    CS101_ASDU_addInformationObject(tmpAsdu, io);
                                    IMasterConnection_sendASDU(connection, tmpAsdu);
                                    CS101_ASDU_destroy(tmpAsdu);  
                                }
                                break;
                            }
                        }
                    }
                    iec104MasterDnp3Master->pendingCommands.lock.unlock();
                    InformationObject_destroy(io);
                    if (i==iec104MasterDnp3Master->messageConfig.size()){
                        CS101_ASDU tmpAsdu = CS101_ASDU_create(alParams, false, CS101_COT_ACTIVATION_CON, InformationObject_getObjectAddress(io), CS101_ASDU_getCA(asdu), false, true);
                        CS101_ASDU_setCOT(tmpAsdu, CS101_COT_UNKNOWN_IOA);
                        CS101_ASDU_setTypeID(tmpAsdu, CS101_ASDU_getTypeID(asdu)); 
                        CS101_ASDU_addInformationObject(tmpAsdu, io);
                        IMasterConnection_sendASDU(connection, tmpAsdu);
                        CS101_ASDU_destroy(tmpAsdu); 
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
    }
    else if (CS101_ASDU_getTypeID(asdu) == M_SP_NA_1) {
        // printf("received single monitor\n");
        for (int j=0 ; j<CS101_ASDU_getNumberOfElements(asdu) ; j++){
            InformationObject io = CS101_ASDU_getElement(asdu, j);
            if (io) {
                iec104MasterDnp3Master->pendingCommands.lock.try_lock();
                iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
                iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
                iec104MasterDnp3Master->pendingCommands.analogInput.clear();
                iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
                iec104MasterDnp3Master->pendingCommands.counter.clear();
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
                for (int i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++){
                    if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::BINARY_INPUT){
                        if (InformationObject_getObjectAddress(io) == iec104MasterDnp3Master->messageConfig[i].iec104Ioa){
                            Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                            messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                            iec104MasterDnp3Master->pendingCommands.binaryInput.push_back(messageConfig);
                            iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                            iec104MasterDnp3Master->pendingCommands.done = false;
                        }
                        else {
                            InformationObject_destroy(io);
                            iec104MasterDnp3Master->pendingCommands.lock.unlock();
                            return false;
                        }
                    }
                }
            }
        }
        iec104MasterDnp3Master->processingCommand();
        std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
        if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                    std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                    [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                    == true){
            IMasterConnection_sendACT_CON(connection, asdu, false);
            if (iec104MasterDnp3Master->pendingCommands.binaryInput.size() > 0){
                bool sameAsdu = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu){
                        sameAsdu = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu;
                    if (iec104MasterDnp3Master->pendingCommands.binaryInput.size() > 0){
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.binaryInput[0].iec104Asdu, false, false);
                        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++) {
                            InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                    iec104MasterDnp3Master->pendingCommands.binaryInput[i].iec104Ioa,
                                                                    iec104MasterDnp3Master->pendingCommands.binaryInput[i].binaryValue,
                                                                    IEC60870_QUALITY_GOOD);
                            CS101_ASDU_addInformationObject(newAsdu, io);
                        }
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
    }
    else if (CS101_ASDU_getTypeID(asdu) == M_ME_NB_1) {
        // printf("received single monitor\n");
        for (int j=0 ; j<CS101_ASDU_getNumberOfElements(asdu) ; j++){
            InformationObject io = CS101_ASDU_getElement(asdu, j);
            if (io) {
                iec104MasterDnp3Master->pendingCommands.lock.try_lock();
                iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
                iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
                iec104MasterDnp3Master->pendingCommands.analogInput.clear();
                iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
                iec104MasterDnp3Master->pendingCommands.counter.clear();
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
                for (int i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++){
                    if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::ANALOG_INPUT){
                        if (InformationObject_getObjectAddress(io) == iec104MasterDnp3Master->messageConfig[i].iec104Ioa){
                            Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                            messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                            iec104MasterDnp3Master->pendingCommands.analogInput.push_back(messageConfig);
                            iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                            iec104MasterDnp3Master->pendingCommands.done = false;
                        }
                        else {
                            InformationObject_destroy(io);
                            iec104MasterDnp3Master->pendingCommands.lock.unlock();
                            return false;
                        }
                    }
                }
            }
        }
        iec104MasterDnp3Master->processingCommand();
        std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
        if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                    std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                    [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                    == true){
            IMasterConnection_sendACT_CON(connection, asdu, false);
            if (iec104MasterDnp3Master->pendingCommands.analogInput.size() > 0){
                bool sameAsdu = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu){
                        sameAsdu = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu;
                    if (iec104MasterDnp3Master->pendingCommands.analogInput.size() > 0){
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.analogInput[0].iec104Asdu, false, false);
                        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++) {
                            InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                    iec104MasterDnp3Master->pendingCommands.analogInput[i].iec104Ioa,
                                                                    iec104MasterDnp3Master->pendingCommands.analogInput[i].analogValue,
                                                                    IEC60870_QUALITY_GOOD);
                            CS101_ASDU_addInformationObject(newAsdu, io);
                        }
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
    }
    else if (CS101_ASDU_getTypeID(asdu) == M_IT_NA_1) {
        // printf("received single monitor\n");
        for (int j=0 ; j<CS101_ASDU_getNumberOfElements(asdu) ; j++){
            InformationObject io = CS101_ASDU_getElement(asdu, j);
            if (io) {
                iec104MasterDnp3Master->pendingCommands.lock.try_lock();
                iec104MasterDnp3Master->pendingCommands.binaryInput.clear();
                iec104MasterDnp3Master->pendingCommands.binaryOutput.clear();
                iec104MasterDnp3Master->pendingCommands.analogInput.clear();
                iec104MasterDnp3Master->pendingCommands.analogOutput.clear();
                iec104MasterDnp3Master->pendingCommands.counter.clear();
                iec104MasterDnp3Master->pendingCommands.pendingCommandCounter = 0;
                for (int i=0 ; i<iec104MasterDnp3Master->messageConfig.size() ; i++){
                    if (iec104MasterDnp3Master->messageConfig[i].messageType == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageType::COUNTER){
                        if (InformationObject_getObjectAddress(io) == iec104MasterDnp3Master->messageConfig[i].iec104Ioa){
                            Iec104MasterDnp3MasterMessageConfig messageConfig = iec104MasterDnp3Master->messageConfig[i];
                            messageConfig.state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT;
                            iec104MasterDnp3Master->pendingCommands.counter.push_back(messageConfig);
                            iec104MasterDnp3Master->pendingCommands.pendingCommandCounter++;
                            iec104MasterDnp3Master->pendingCommands.done = false;
                        }
                        else {
                            InformationObject_destroy(io);
                            iec104MasterDnp3Master->pendingCommands.lock.unlock();
                            return false;
                        }
                    }
                }
            }
        }
        iec104MasterDnp3Master->processingCommand();
        std::unique_lock<std::mutex> uniqueLock(iec104MasterDnp3Master->pendingCommands.lock);
        if (iec104MasterDnp3Master->pendingCommands.sync.wait_for(uniqueLock, 
                                                    std::chrono::seconds(iec104MasterDnp3Master->dnp3ResponseTimeoutSecond), 
                                                    [&](){return iec104MasterDnp3Master->pendingCommands.done;})
                                                    == true){
            IMasterConnection_sendACT_CON(connection, asdu, false);
            if (iec104MasterDnp3Master->pendingCommands.counter.size() > 0){
                bool sameAsdu = true;
                for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++)
                {
                    if (iec104MasterDnp3Master->pendingCommands.counter[i].iec104Asdu 
                        != iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu){
                        sameAsdu = false;
                    }
                }
                if (sameAsdu){
                    int commonAddress = iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu;
                    if (iec104MasterDnp3Master->pendingCommands.counter.size() > 0){
                        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                                                0, iec104MasterDnp3Master->pendingCommands.counter[0].iec104Asdu, false, false);
                        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++) {
                            InformationObject io = (InformationObject) SinglePointInformation_create(NULL, 
                                                                    iec104MasterDnp3Master->pendingCommands.counter[i].iec104Ioa,
                                                                    iec104MasterDnp3Master->pendingCommands.counter[i].counterValue,
                                                                    IEC60870_QUALITY_GOOD);
                            CS101_ASDU_addInformationObject(newAsdu, io);
                        }
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                    }
                }
            }
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_TERM(connection, asdu);
        }
    }
    return true;
}
bool Iec104MasterDnp3Master::iec104ConnectionRequestHandler(void* parameter, const char* ipAddress){
    return true;
}
void Iec104MasterDnp3Master::iec104ConnectionEventHandler(void* parameter, IMasterConnection con, CS104_PeerConnectionEvent event){
    Iec104MasterDnp3Master *iec104MasterDnp3Master = static_cast<Iec104MasterDnp3Master*>(parameter);
    CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(con);
    if (event == CS104_CON_EVENT_CONNECTION_OPENED) {
        std::cout << ("Connection opened (%p)\n", con) << std::endl;
    }
    else if (event == CS104_CON_EVENT_CONNECTION_CLOSED) {
        std::cout << ("Connection closed (%p)\n", con) << std::endl;
    }
    else if (event == CS104_CON_EVENT_ACTIVATED) {
        std::cout << ("Connection activated (%p)\n", con) << std::endl;
    }
    else if (event == CS104_CON_EVENT_DEACTIVATED) {
        std::cout << ("Connection deactivated (%p)\n", con) << std::endl;
    }
}
bool Iec104MasterDnp3Master::processingCommand(){
    // pendingCommands.lock.try_lock();

    for (int i=0 ; i<pendingCommands.binaryInput.size() ; i++){
        dnp3Master->ScanRange(GroupVariationID(1, 2), pendingCommands.binaryInput[i].dnp3Point, pendingCommands.binaryInput[i].dnp3Point, dnp3IsoeHandler);
    }
    for (int i=0 ; i<pendingCommands.analogInput.size() ; i++){
        dnp3Master->ScanRange(GroupVariationID(30, 0), pendingCommands.analogInput[i].dnp3Point, pendingCommands.analogInput[i].dnp3Point, dnp3IsoeHandler);
    }
    for (int i=0 ; i<pendingCommands.counter.size() ; i++){
        dnp3Master->ScanRange(GroupVariationID(20, 0), pendingCommands.counter[i].dnp3Point, pendingCommands.counter[i].dnp3Point, dnp3IsoeHandler);
    }
    for (int i=0 ; i<pendingCommands.binaryOutput.size() ; i++){
        ControlRelayOutputBlock crob(pendingCommands.binaryOutput[i].binaryValue?OperationType::LATCH_ON:OperationType::LATCH_OFF);
        auto callback = [&](const ICommandTaskResult& result) -> void
        {		
            pendingCommands.lock.try_lock();	
            auto checkErr = [&](const CommandPointResult& res)
            {
                if (res.status == CommandStatus::SUCCESS){
                    // std::cout << "binary output success" << std::endl;
                    pendingCommands.binaryOutput[0].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS;
                } else {
                    // std::cout << "binary output failed" << std::endl;
                    pendingCommands.binaryOutput[0].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_FAILED;
                }
            };
            result.ForeachItem(checkErr);
            
            pendingCommands.pendingCommandCounter--;
            if (pendingCommands.pendingCommandCounter == 0){
                pendingCommands.done = true;
                pendingCommands.sync.notify_all();
            }
            pendingCommands.lock.unlock();
        };
        dnp3Master->SelectAndOperate(crob, pendingCommands.binaryOutput[i].dnp3Point, callback);
    }

    for (int i=0 ; i<pendingCommands.analogOutput.size() ; i++){
        AnalogOutputInt32 analogOutput;
        analogOutput.status = CommandStatus::SUCCESS;
        analogOutput.value = pendingCommands.analogOutput[i].analogValue;
        auto callback = [&](const ICommandTaskResult& result) -> void
        {			
            pendingCommands.lock.try_lock();	
            auto checkErr = [&](const CommandPointResult& res)
            {
                if (res.status == CommandStatus::SUCCESS){
                    // std::cout << "binary output success" << std::endl;
                    pendingCommands.analogOutput[0].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS;
                } else {
                    // std::cout << "binary output failed" << std::endl;
                    pendingCommands.analogOutput[0].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_FAILED;
                }
            };
            result.ForeachItem(checkErr);

            pendingCommands.pendingCommandCounter--;
            if (pendingCommands.pendingCommandCounter == 0){
                pendingCommands.done = true;
                pendingCommands.sync.notify_all();
            }
            pendingCommands.lock.unlock();
        };
        dnp3Master->SelectAndOperate(analogOutput, pendingCommands.analogOutput[i].dnp3Point, callback);
    }
    // pendingCommands.lock.unlock();
    return true;
}