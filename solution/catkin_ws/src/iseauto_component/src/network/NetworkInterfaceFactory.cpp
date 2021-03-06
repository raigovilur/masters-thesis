
#include "NetworkInterfaceFactory.h"
#include "DummyNetwork.h"

std::shared_ptr<NetworkInterface> NetworkInterfaceFactory::getInstance(type type)
{
    switch (type) {
        case Dummy_Type:
            return std::make_shared<DummyNetwork>();
        default:
            throw std::logic_error("Invalid network interface type");
    }
}
