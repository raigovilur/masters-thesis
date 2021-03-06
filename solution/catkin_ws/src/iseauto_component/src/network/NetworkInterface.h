#ifndef ISEAUTO_COMPONENT_NETWORKINTERFACE_H
#define ISEAUTO_COMPONENT_NETWORKINTERFACE_H

#include <sensor_msgs/PointCloud2.h>

/**
 * This is an interface class for all networking components
 * Networking components implement a connection via a particular
 * library and protocol
 */
class NetworkInterface {

public:
    NetworkInterface() = default;
    virtual ~NetworkInterface() = default;

    virtual bool init(std::string address) = 0;

    // Add these for each type of message we support
    // In the future, depending on what data the actual cloud server
    // is going to recieve, maybe change it to raw data.
    virtual bool send(const sensor_msgs::PointCloud2 &msg) = 0;

    virtual bool close() = 0;

};


#endif //ISEAUTO_COMPONENT_NETWORKINTERFACE_H
