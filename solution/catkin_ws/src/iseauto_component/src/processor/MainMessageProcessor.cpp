#include "MainMessageProcessor.h"

MainMessageProcessor::MainMessageProcessor(NetworkInterfaceFactory::type type)
  :_type(type)
{ }

void MainMessageProcessor::processMessage(const sensor_msgs::PointCloud2 &msg) {
    //TODO implement

    // TODO this is a lazy test implementation
    std::shared_ptr<NetworkInterface> networking = NetworkInterfaceFactory::getInstance(_type);
    networking->send(msg);
}