/**
 * @file CrazyflieDriver.cpp
 * @brief Plugin C++ Webots ROS 2 per il controllo fisico del Crazyflie 2.1.
 *
 * Strutturato secondo la guida ufficiale Cyberbotics `webots_ros2_driver::PluginInterface`
 * (come in MyRobotDriver.cpp) e sui guadagni del controllore Bitcraze Webots
 * integrato con il server Crazyswarm2 (`crazyflie_sim`).
 */

#include "crazyflie_webots_cpp/CrazyflieDriver.hpp"

#include <algorithm>
#include <cmath>

#include <webots/gps.h>
#include <webots/gyro.h>
#include <webots/inertial_unit.h>
#include <webots/motor.h>
#include <webots/robot.h>

#include "pluginlib/class_list_macros.hpp"

namespace crazyflie_webots_cpp {

namespace {
// Velocita' angolare esatta di hovering per il modello fisico Webots Crazyflie.proto:
// m = 0.05 kg, g = 9.81 m/s^2, 4 eliche con thrustConstant ct = 4e-5 N/(rad/s)^2
// omega_hover = sqrt((0.05 * 9.81) / (4 * 4e-5)) = 55.3681 rad/s
const double kHoverOmega = std::sqrt((0.05 * 9.81) / (4.0 * 4e-5));
}  // namespace

void CrazyflieDriver::init(
    webots_ros2_driver::WebotsNode *node,
    std::unordered_map<std::string, std::string> & /*parameters*/) {
  node_ = node;
  timestep_ = static_cast<int>(wb_robot_get_basic_time_step());
  robot_name_ = wb_robot_get_name();

  // 1. Inizializzazione Motori (4 eliche in modalita' velocita' angolare rad/s)
  m1_motor_ = wb_robot_get_device("m1_motor");
  wb_motor_set_position(m1_motor_, INFINITY);
  wb_motor_set_velocity(m1_motor_, 0.0);

  m2_motor_ = wb_robot_get_device("m2_motor");
  wb_motor_set_position(m2_motor_, INFINITY);
  wb_motor_set_velocity(m2_motor_, 0.0);

  m3_motor_ = wb_robot_get_device("m3_motor");
  wb_motor_set_position(m3_motor_, INFINITY);
  wb_motor_set_velocity(m3_motor_, 0.0);

  m4_motor_ = wb_robot_get_device("m4_motor");
  wb_motor_set_position(m4_motor_, INFINITY);
  wb_motor_set_velocity(m4_motor_, 0.0);

  // 2. Inizializzazione Sensori di bordo Webots
  imu_ = wb_robot_get_device("inertial_unit");
  wb_inertial_unit_enable(imu_, timestep_);

  gps_ = wb_robot_get_device("gps");
  wb_gps_enable(gps_, timestep_);

  gyro_ = wb_robot_get_device("gyro");
  wb_gyro_enable(gyro_, timestep_);

  // 3. Stato iniziale del controllore
  past_time_ = wb_robot_get_time();
  first_step_ = true;

  // 4. Publisher e Subscriber ROS 2 sul WebotsNode C++
  pose_pub_ = node_->create_publisher<geometry_msgs::msg::PoseStamped>(
      "/" + robot_name_ + "/pose", 10);

  setpoint_sub_ = node_->create_subscription<crazyflie_interfaces::msg::FullState>(
      "/" + robot_name_ + "/setpoint_full_state", 10,
      [this](const crazyflie_interfaces::msg::FullState::SharedPtr msg) {
        this->onSetpointFullState(msg);
      });

  RCLCPP_INFO(
      node_->get_logger(),
      "[%s] CrazyflieDriver C++ pronto (timestep=%d ms)",
      robot_name_.c_str(), timestep_);
}

void CrazyflieDriver::resetPid() {
  altitude_integrator_ = 0.0;
}

void CrazyflieDriver::stopMotors() {
  wb_motor_set_velocity(m1_motor_, 0.0);
  wb_motor_set_velocity(m2_motor_, 0.0);
  wb_motor_set_velocity(m3_motor_, 0.0);
  wb_motor_set_velocity(m4_motor_, 0.0);
}

void CrazyflieDriver::onSetpointFullState(
    const crazyflie_interfaces::msg::FullState::SharedPtr msg) {
  if (msg->header.frame_id == "IDLE") {
    is_idle_ = true;
    return;
  }

  is_idle_ = false;
  des_pos_[0] = msg->pose.position.x;
  des_pos_[1] = msg->pose.position.y;
  des_pos_[2] = msg->pose.position.z;

  des_vel_[0] = msg->twist.linear.x;
  des_vel_[1] = msg->twist.linear.y;
  des_vel_[2] = msg->twist.linear.z;

  des_acc_[0] = msg->acc.x;
  des_acc_[1] = msg->acc.y;
  des_acc_[2] = msg->acc.z;

  const double qw = msg->pose.orientation.w;
  const double qx = msg->pose.orientation.x;
  const double qy = msg->pose.orientation.y;
  const double qz = msg->pose.orientation.z;
  des_yaw_ = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));
}

void CrazyflieDriver::step() {
  const double current_time = wb_robot_get_time();
  const double dt = current_time - past_time_;
  if (dt <= 1e-5) {
    return;
  }

  // Lettura sensori Webots tramite API C (IMU + Giroscopio 3 assi + GPS)
  const double *rpy = wb_inertial_unit_get_roll_pitch_yaw(imu_);
  const double actual_roll = rpy[0];
  const double actual_pitch = rpy[1];
  const double actual_yaw = rpy[2];

  const double *gyro_vals = wb_gyro_get_values(gyro_);
  const double actual_roll_rate = gyro_vals[0];
  const double actual_pitch_rate = gyro_vals[1];
  const double actual_yaw_rate = gyro_vals[2];

  const double *gps_vals = wb_gps_get_values(gps_);
  const double x_g = gps_vals[0];
  const double y_g = gps_vals[1];
  const double z_g = gps_vals[2];

  // Pubblicazione posa reale su /<name>/pose
  const double cy = std::cos(actual_yaw * 0.5);
  const double sy = std::sin(actual_yaw * 0.5);
  const double cp = std::cos(actual_pitch * 0.5);
  const double sp = std::sin(actual_pitch * 0.5);
  const double cr = std::cos(actual_roll * 0.5);
  const double sr = std::sin(actual_roll * 0.5);

  geometry_msgs::msg::PoseStamped pose_msg;
  pose_msg.header.stamp = node_->get_clock()->now();
  pose_msg.header.frame_id = "world";
  pose_msg.pose.position.x = x_g;
  pose_msg.pose.position.y = y_g;
  pose_msg.pose.position.z = z_g;
  pose_msg.pose.orientation.w = cr * cp * cy + sr * sp * sy;
  pose_msg.pose.orientation.x = sr * cp * cy - cr * sp * sy;
  pose_msg.pose.orientation.y = cr * sp * cy + sr * cp * sy;
  pose_msg.pose.orientation.z = cr * cp * sy - sr * sp * cy;
  pose_pub_->publish(pose_msg);

  if (first_step_) {
    past_x_ = x_g;
    past_y_ = y_g;
    past_z_ = z_g;
    past_time_ = current_time;
    first_step_ = false;
    return;
  }

  // Calcolo velocita' lineari nel frame mondo (da GPS Webots)
  const double vx_g = (x_g - past_x_) / dt;
  const double vy_g = (y_g - past_y_) / dt;
  const double vz_g = (z_g - past_z_) / dt;

  past_x_ = x_g;
  past_y_ = y_g;
  past_z_ = z_g;
  past_time_ = current_time;

  // Gestione stato IDLE / Disarmato o touchdown completato
  if (is_idle_ || (des_pos_[2] <= 0.035 && z_g <= 0.030 && des_vel_[2] <= 0.0)) {
    resetPid();
    stopMotors();
    return;
  }

  // ===========================================================================
  // Controllore in Cascata (Posizione -> Velocita' -> Assetto -> Motori)
  // ===========================================================================

  // 1. Anello di Posizione XY (World Frame) -> Velocita' desiderata (Body Frame)
  const double ex = des_pos_[0] - x_g;
  const double ey = des_pos_[1] - y_g;
  const double v_cmd_world_x = std::clamp(0.60 * ex + 0.70 * des_vel_[0], -1.2, 1.2);
  const double v_cmd_world_y = std::clamp(0.60 * ey + 0.70 * des_vel_[1], -1.2, 1.2);

  const double cosyaw = std::cos(actual_yaw);
  const double sinyaw = std::sin(actual_yaw);
  const double actual_vx = vx_g * cosyaw + vy_g * sinyaw;
  const double actual_vy = -vx_g * sinyaw + vy_g * cosyaw;
  const double desired_vx = v_cmd_world_x * cosyaw + v_cmd_world_y * sinyaw;
  const double desired_vy = -v_cmd_world_x * sinyaw + v_cmd_world_y * cosyaw;
  const double ax_ff_body = des_acc_[0] * cosyaw + des_acc_[1] * sinyaw;
  const double ay_ff_body = -des_acc_[0] * sinyaw + des_acc_[1] * cosyaw;

  // 2. Anello Velocita' Orizzontale -> Roll/Pitch desiderati (limitati a +/- 0.22 rad)
  const double vx_error = desired_vx - actual_vx;
  const double vy_error = desired_vy - actual_vy;
  const double desired_pitch = std::clamp(0.14 * vx_error + 0.45 * ax_ff_body / 9.81, -0.22, 0.22);
  const double desired_roll = std::clamp(-0.14 * vy_error - 0.45 * ay_ff_body / 9.81, -0.22, 0.22);

  // 3. Anello di Quota Z in cascata (Posizione Z -> Velocita' Z -> Spinta Motori)
  const double alt_error = des_pos_[2] - z_g;
  const double vz_cmd = std::clamp(2.0 * alt_error + des_vel_[2], -1.0, 1.0);
  const double vz_error = vz_cmd - vz_g;
  altitude_integrator_ = std::clamp(altitude_integrator_ + alt_error * dt, -0.5, 0.5);
  const double tilt_comp = std::max(std::cos(actual_roll) * std::cos(actual_pitch), 0.7);
  const double alt_command =
      (kHoverOmega / tilt_comp) +
      15.0 * std::clamp(vz_error, -1.0, 1.0) +
      0.5 * altitude_integrator_;

  // 4. Anello Interno di Assetto (Roll, Pitch, Yaw)
  const double roll_error = desired_roll - actual_roll;
  const double pitch_error = desired_pitch - actual_pitch;
  const double yaw_error = std::atan2(std::sin(des_yaw_ - actual_yaw), std::cos(des_yaw_ - actual_yaw));
  const double desired_yaw_rate = std::clamp(1.5 * yaw_error, -1.0, 1.0);
  const double yaw_rate_error = desired_yaw_rate - actual_yaw_rate;

  const double roll_command = 1.8 * std::clamp(roll_error, -0.5, 0.5) - 0.45 * actual_roll_rate;
  const double pitch_command = -1.8 * std::clamp(pitch_error, -0.5, 0.5) + 0.45 * actual_pitch_rate;
  const double yaw_command = 1.0 * std::clamp(yaw_rate_error, -1.0, 1.0);

  // 5. Motor Mixing (configurazione X-quadrotor ufficiale Bitcraze Webots)
  const double m1 = std::clamp(alt_command - roll_command + pitch_command + yaw_command, 0.0, 600.0);
  const double m2 = std::clamp(alt_command - roll_command - pitch_command - yaw_command, 0.0, 600.0);
  const double m3 = std::clamp(alt_command + roll_command - pitch_command + yaw_command, 0.0, 600.0);
  const double m4 = std::clamp(alt_command + roll_command + pitch_command - yaw_command, 0.0, 600.0);

  wb_motor_set_velocity(m1_motor_, -m1);
  wb_motor_set_velocity(m2_motor_, m2);
  wb_motor_set_velocity(m3_motor_, -m3);
  wb_motor_set_velocity(m4_motor_, m4);
}

}  // namespace crazyflie_webots_cpp

PLUGINLIB_EXPORT_CLASS(
    crazyflie_webots_cpp::CrazyflieDriver,
    webots_ros2_driver::PluginInterface)
