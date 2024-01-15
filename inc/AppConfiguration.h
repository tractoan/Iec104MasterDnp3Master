#include <iostream>
#include <string>
#include <vector>

enum MessageType {BINARY_INPUT, BINARY_OUTPUT, ANALOG_INPUT, ANALOG_OUTPUT, COUNTER};
enum DNP3ConnectionType {DNP3_CONNECTION_TCP, DNP3_CONNECTION_UDP, DNP3_CONNECTION_SERIAL};
class MessageConfig {
public:
    MessageType messageType;
    int dnp3Point;
    int iec104TypeId;
    int iec104Asdu;
    int iec104Ioa;
};

class AppConfiguration {
public:    
    AppConfiguration() = default;
    ~AppConfiguration() = default;
    bool configValid = false;
    uint8_t dnp3Destination;
    uint8_t dnp3Source;
    int dnp3ResponseTimeoutSecond;
    std::string dnp3ClientAddress;
    int dnp3ClientPort;
    DNP3ConnectionType dnp3ConnectionType;
    std::string dnp3ClientSerialPort;
    int dnp3ClientSerialPortBaudrate;
    std::vector<MessageConfig> configData;
    static AppConfiguration fromFile(const std::string &fileName);
};
