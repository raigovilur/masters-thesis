//
// Created by raigo on 06.03.21.
//

#ifndef ISEAUTO_COMPONENT_MAINMESSAGEPROCESSOR_H
#define ISEAUTO_COMPONENT_MAINMESSAGEPROCESSOR_H


#include <sensor_msgs/PointCloud2.h>
#include "../network/NetworkInterfaceFactory.h"

class MainMessageProcessor {
public:
    MainMessageProcessor(NetworkInterfaceFactory::type);

    void processMessage(const sensor_msgs::PointCloud2 &msg);

private:
    NetworkInterfaceFactory::type _type;
};


#endif //ISEAUTO_COMPONENT_MAINMESSAGEPROCESSOR_H
