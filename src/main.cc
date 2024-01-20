#include <iostream>
#include "file_configuration.h"
#include "Iec104MasterDnp3Master.h"
#include <thread>
#include <future>

int main(int argc, char *argv[])
{
    Iec104MasterDnp3Master iec104MasterDnp3Master;
    AppConfiguration appConfiguration = AppConfiguration::fromFile("./configs");
    if (appConfiguration.getConfigValid() == false){
        std::cout << "Error in Config!" << std::endl;
        return -1;
    }
    if (iec104MasterDnp3Master.setConfigs(appConfiguration)){
        iec104MasterDnp3Master.init();
        iec104MasterDnp3Master.start();
        std::promise<void>().get_future().wait();
    }
    return 0;
}