#include "MainMessageProcessor.h"

MainMessageProcessor::MainMessageProcessor(NetworkInterfaceFactory::type type)
  :_type(type)
{ }

void MainMessageProcessor::processMessage(const sensor_msgs::PointCloud2 &msg) {
    //TODO implement
}