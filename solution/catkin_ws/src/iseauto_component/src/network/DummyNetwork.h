#ifndef ISEAUTO_COMPONENT_DUMMYNETWORK_H
#define ISEAUTO_COMPONENT_DUMMYNETWORK_H


#include "NetworkInterface.h"

/**
 * This is a dummy network interface. It won't actually send anything
 * It's a placeholder until all libraries are implemented
 * and there's a server to actually send data to.
 */
class DummyNetwork : public NetworkInterface {

public:
    DummyNetwork();
    ~DummyNetwork();

    bool init(std::string address) override;

    bool send(const sensor_msgs::PointCloud2 &msg) override;

    bool close() override;
};


#endif //ISEAUTO_COMPONENT_DUMMYNETWORK_H
