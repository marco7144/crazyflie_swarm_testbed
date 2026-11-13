#ifndef CRAZYFLIE_WEBOTS_CPP_CRAZYFLIE_DRIVER_HPP
#define CRAZYFLIE_WEBOTS_CPP_CRAZYFLIE_DRIVER_HPP

#include <string>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "webots_ros2_driver/PluginInterface.hpp"
#include "webots_ros2_driver/WebotsNode.hpp"

#include "crazyflie_interfaces/msg/full_state.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace crazyflie_webots_cpp {

class CrazyflieDriver : public webots_ros2_driver::PluginInterface {
public:
  void step() override;
  void init(
    webots_ros2_driver::WebotsNode *node,
    std::unordered_map<std::string, std::string> &parameters) override;

private:
  void resetPid();
  void stopMotors();
  void onSetpointFullState(const crazyflie_interfaces::msg::FullState::SharedPtr msg);

  webots_ros2_driver::WebotsNode *node_{nullptr};
  std::string robot_name_;
  int timestep_{8};

  // Dispositivi Webots (4 motori e 3 sensori)
  WbDeviceTag m1_motor_{0};
  WbDeviceTag m2_motor_{0};
  WbDeviceTag m3_motor_{0};
  WbDeviceTag m4_motor_{0};
  WbDeviceTag imu_{0};
  WbDeviceTag gps_{0};
  WbDeviceTag gyro_{0};

  // Stato del controllore PID in cascata
  double past_time_{0.0};
  bool first_step_{true};
  double past_x_{0.0};
  double past_y_{0.0};
  double past_z_{0.0};
  double altitude_integrator_{0.0};

  // Setpoint FullState ricevuti dal server Crazyswarm2
  bool is_idle_{true};
  double des_pos_[3]{0.0, 0.0, 0.0};
  double des_vel_[3]{0.0, 0.0, 0.0};
  double des_acc_[3]{0.0, 0.0, 0.0};
  double des_yaw_{0.0};

  // Publisher e Subscriber ROS 2
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
  rclcpp::Subscription<crazyflie_interfaces::msg::FullState>::SharedPtr setpoint_sub_;
};

}  // namespace crazyflie_webots_cpp

#endif  // CRAZYFLIE_WEBOTS_CPP_CRAZYFLIE_DRIVER_HPP
