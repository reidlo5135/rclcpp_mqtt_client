#include "ros_mqtt_bridge/ros_mqtt_bridge.hpp"

RosMqttSubscription::RosMqttSubscription(MqttMgr * mqtt_mgr_ptr, rclcpp::Node::SharedPtr rcl_node_ptr)
: log_ros_subscription_(LOG_ROS_SUBSCRITPION),
mqtt_mgr_ptr_(mqtt_mgr_ptr),
ros_node_ptr_(rcl_node_ptr) {   
    
}

RosMqttSubscription::~RosMqttSubscription() {

}

void RosMqttSubscription::create_ros_mqtt_bridge() {
    ros_mqtt_connections::subscription::ros_std_subscription_ = ros_node_ptr_->create_subscription<std_msgs::msg::String>(
            mqtt_topics::publisher::chatter_topic,
            rclcpp::QoS(rclcpp::KeepLast(10)),
            [this](const std_msgs::msg::String::SharedPtr msg) {
                const char* callback_data = const_cast<char*>(msg->data.c_str());
                RCLCPP_INFO(ros_node_ptr_->get_logger(), "I heard: '%s'", callback_data);
                this->mqtt_mgr_ptr_->mqtt_publish("/chatter", callback_data);
            }
        );

    ros_mqtt_connections::subscription::ros_odom_subscription_ = ros_node_ptr_->create_subscription<nav_msgs::msg::Odometry>(
        mqtt_topics::publisher::odom_topic,
        rclcpp::QoS(rclcpp::KeepLast(20)),
        [this](const nav_msgs::msg::Odometry::SharedPtr odom_message) {
            auto callback_data = odom_message->pose.pose.position.x;
            RCLCPP_INFO(ros_node_ptr_->get_logger(), "odom callback : '%s'", callback_data);
            this->mqtt_mgr_ptr_->mqtt_publish("/odom", std::to_string(callback_data));
        }
    );
}

RosMqttBridge::RosMqttBridge(MqttMgr * mqtt_mgr_ptr) 
: Node("ros_mqtt_bridge"),
mqtt_mgr_ptr_(mqtt_mgr_ptr) {
    ros_node_ptr_ = std::shared_ptr<rclcpp::Node>(this, [](rclcpp::Node*){});
    ros_subscription_ptr_ = new RosMqttSubscription(mqtt_mgr_ptr_, ros_node_ptr_);
    ros_subscription_ptr_->create_ros_mqtt_bridge();
}

RosMqttBridge::~RosMqttBridge() {
    delete ros_subscription_ptr_;
}

int main(int argc, char** argv) {
    MqttMgr * mqtt_ptr = new MqttMgr(MQTT_ADDRESS, MQTT_CLIENT_ID);
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RosMqttBridge>(mqtt_ptr);
    while(rclcpp::ok()) {
        rclcpp::spin(node);
    }
    rclcpp::shutdown();
    
    delete mqtt_ptr;
    return 0;
}