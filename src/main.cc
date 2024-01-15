#include <iostream>
#include "file_configuration.h"
#include "Iec104MasterDnp3Master.h"
#include <thread>
#include <future>

int main(int argc, char *argv[])
{
    Iec104MasterDnp3Master iec104MasterDnp3Master;
    if (iec104MasterDnp3Master.setConfigs(AppConfiguration::fromFile("./configs"))){
        iec104MasterDnp3Master.init();
        iec104MasterDnp3Master.start();
        std::promise<void>().get_future().wait();
    }
    return 0;
}