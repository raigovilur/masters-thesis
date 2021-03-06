#ifndef ISEAUTO_COMPONENT_NETWORKINTERFACEFACTORY_H
#define ISEAUTO_COMPONENT_NETWORKINTERFACEFACTORY_H

#include "NetworkInterface.h"

class NetworkInterfaceFactory {

public:
    enum type {
        Dummy_Type
    };

    static std::shared_ptr<NetworkInterface> getInstance(type type);
};

#endif //ISEAUTO_COMPONENT_NETWORKINTERFACEFACTORY_H

