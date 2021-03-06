#include <ros/console.h>
#include "DummyNetwork.h"


DummyNetwork::DummyNetwork()
: NetworkInterface()
{
    ROS_INFO("Dummy network instance constructed");
}

bool DummyNetwork::init(std::string address)
{
    ROS_INFO("Dummy network connected to %s", address.c_str());
}

bool DummyNetwork::send(const sensor_msgs::PointCloud2 &msg)
{
    uint32_t size = msg.row_step * msg.height;
    uint32_t sizeInKb = size / 8 / 1024;
    ROS_INFO("Message sent, size: %d MB", sizeInKb);
}

bool DummyNetwork::close()
{
    ROS_INFO("Dummy network connection closed");
}

DummyNetwork::~DummyNetwork() {
    ROS_INFO("Dummy network instance destroyed");
}
