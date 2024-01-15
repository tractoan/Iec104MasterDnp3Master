#include <opendnp3/ConsoleLogger.h>
#include <opendnp3/DNP3Manager.h>
#include <opendnp3/channel/PrintingChannelListener.h>
#include <opendnp3/logging/LogLevels.h>
#include <opendnp3/master/DefaultMasterApplication.h>
#include <opendnp3/master/PrintingCommandResultCallback.h>
#include <opendnp3/master/PrintingSOEHandler.h>
#include "Iec104MasterDnp3Master.h"
extern "C" {
    #include "cs104_slave.h"
}

using namespace std;
using namespace opendnp3;

class Dnp3ISOEHandler : public ISOEHandler
{
    virtual void BeginFragment(const ResponseInfo& info){};
    virtual void EndFragment(const ResponseInfo& info){};

    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Binary>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<DoubleBitBinary>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Analog>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<Counter>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<FrozenCounter>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<BinaryOutputStatus>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<AnalogOutputStatus>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<OctetString>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<TimeAndInterval>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<BinaryCommandEvent>>& values);
    virtual void Process(const HeaderInfo& info, const ICollection<Indexed<AnalogCommandEvent>>& values);    
    virtual void Process(const HeaderInfo& info, const ICollection<DNPTime>& values);
    Iec104MasterDnp3Master *iec104MasterDnp3Master;

public:
    bool setIec104MasterDnp3Master(Iec104MasterDnp3Master *iec104MasterDnp3Master){
        this->iec104MasterDnp3Master = iec104MasterDnp3Master;
    }
};