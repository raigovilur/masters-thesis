#include "ros/ros.h"
#include <ros/console.h>

#include <sensor_msgs/PointCloud2.h>

#include <string>

void step(const sensor_msgs::PointCloud2 &msg) {
    uint32_t size = msg.row_step * msg.height;
    uint32_t sizeInKb = size / 8 / 1024;
    ROS_INFO("Message received, size: %d MB", sizeInKb);
}

ros::Subscriber sub;
int main(int argc, char **argv) {

    if( ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug) ) {
        ros::console::notifyLoggerLevelsChanged();
    }

    ROS_INFO("Starting up");

    ros::init(argc, argv, "imageProcessor");
    ros::NodeHandle n("~");

    sub = n.subscribe("/lidar_back/points_ground", 1, &step);

    ros::spin();

    return 0;
}
