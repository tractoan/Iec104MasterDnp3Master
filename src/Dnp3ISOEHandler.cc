#include "Iec104MasterDnp3Master.h"
#include "Dnp3ISOEHandler.h"
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<Binary>>& values) {
    iec104MasterDnp3Master->pendingCommands.lock.try_lock();
    auto logItem = [this](const Indexed<Binary>& item) {
        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.binaryInput.size() ; i++){
            if(item.index == iec104MasterDnp3Master->pendingCommands.binaryInput[i].dnp3Point){
                iec104MasterDnp3Master->pendingCommands.binaryInput[i].binaryValue = item.value.value;
                if (iec104MasterDnp3Master->pendingCommands.binaryInput[i].state == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT){
                    iec104MasterDnp3Master->pendingCommands.pendingCommandCounter--;
                    iec104MasterDnp3Master->pendingCommands.binaryInput[i].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS;
                }
            }
        }
    };
    values.ForeachItem(logItem);
    if (iec104MasterDnp3Master->pendingCommands.pendingCommandCounter == 0){
        iec104MasterDnp3Master->pendingCommands.done = true;
        iec104MasterDnp3Master->pendingCommands.sync.notify_all();
    }
    iec104MasterDnp3Master->pendingCommands.lock.unlock();
};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<DoubleBitBinary>>& values) {
    return;
}
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<Analog>>& values) {
    iec104MasterDnp3Master->pendingCommands.lock.try_lock();
    auto logItem = [this](const Indexed<Analog>& item) {
        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.analogInput.size() ; i++){
            if(item.index == iec104MasterDnp3Master->pendingCommands.analogInput[i].dnp3Point){
                iec104MasterDnp3Master->pendingCommands.analogInput[i].binaryValue = item.value.value;
                if (iec104MasterDnp3Master->pendingCommands.analogInput[i].state == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT){
                    iec104MasterDnp3Master->pendingCommands.pendingCommandCounter--;
                    iec104MasterDnp3Master->pendingCommands.analogInput[i].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS;
                }
            }
        }
    };
    values.ForeachItem(logItem);
    if (iec104MasterDnp3Master->pendingCommands.pendingCommandCounter == 0){
        iec104MasterDnp3Master->pendingCommands.done = true;
        iec104MasterDnp3Master->pendingCommands.sync.notify_all();
    }
    iec104MasterDnp3Master->pendingCommands.lock.unlock();
};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<Counter>>& values) {
    iec104MasterDnp3Master->pendingCommands.lock.try_lock();
    auto logItem = [this](const Indexed<Counter>& item) {
        for (int i=0 ; i<iec104MasterDnp3Master->pendingCommands.counter.size() ; i++){
            if(item.index == iec104MasterDnp3Master->pendingCommands.counter[i].dnp3Point){
                iec104MasterDnp3Master->pendingCommands.counter[i].binaryValue = item.value.value;
                if (iec104MasterDnp3Master->pendingCommands.counter[i].state == Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::WAIT_CLIENT){
                    iec104MasterDnp3Master->pendingCommands.pendingCommandCounter--;
                    iec104MasterDnp3Master->pendingCommands.counter[i].state = Iec104MasterDnp3MasterMessageConfig::Iec104MasterDnp3MasterMessageState::RESPONSE_SUCCESS;
                }
            }
        }
    };
    values.ForeachItem(logItem);
    if (iec104MasterDnp3Master->pendingCommands.pendingCommandCounter == 0){
        iec104MasterDnp3Master->pendingCommands.done = true;
        iec104MasterDnp3Master->pendingCommands.sync.notify_all();
    }
    iec104MasterDnp3Master->pendingCommands.lock.unlock();
};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<FrozenCounter>>& values) {};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<BinaryOutputStatus>>& values) {
    std::cout << "hihihi" << std::endl;
};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<AnalogOutputStatus>>& values) {};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<OctetString>>& values) {};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<TimeAndInterval>>& values) {};
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<BinaryCommandEvent>>& values) {
    std::cout << "hehehehe" << std::endl;
}
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<Indexed<AnalogCommandEvent>>& values) {};    
void Dnp3ISOEHandler::Process(const HeaderInfo& info, const ICollection<DNPTime>& values) {};