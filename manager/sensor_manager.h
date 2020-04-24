/******************************************************************************
 * Copyright 2017 RoboSense All rights reserved.
 * Suteng Innovation Technology Co., Ltd. www.robosense.ai

 * This software is provided to you directly by RoboSense and might
 * only be used to access RoboSense LiDAR. Any compilation,
 * modification, exploration, reproduction and redistribution are
 * restricted without RoboSense's prior consent.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROBOSENSE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#pragma once

#include <memory>
#include <rs_common/common.h>
#include <rs_lidar/lidar_driver.h>
#include "adapter/lidar_points_ros_adapter.h"
#include "adapter/lidar_packets_ros_adapter.h"
#include "adapter/lidar_points_proto_adapter.h"
#include "adapter/lidar_packets_proto_adapter.h"


namespace robosense
{
namespace sensor
{
enum MessageSource
{
  MessageSourceNotUsed = 0,
  MessageSourceRsDriver = 1,
  MessageSourceRos = 2,
  MessageSourceProto = 3
};
class SensorManager : virtual public common::CommonBase
{
public:
  SensorManager()=default;
  ~SensorManager();

  common::ErrCode init(const YAML::Node &sensor_config, std::string config_path);
  common::ErrCode start();
  common::ErrCode stop();

  inline void regExceptionCallback(const std::function<void(const common::ErrCode &)> &callBack)
  {
    std::lock_guard<std::mutex> lock(mutex_exception_);
    excbs_.emplace_back(callBack);
  }

  inline void regRecvCallback(const std::function<void(const common::LidarPointsMsg &)> &callBack)
  {
    std::lock_guard<std::mutex> lock(mutex_points_);
    lidarPointscbs_.emplace_back(callBack);
  }

private:
  template <typename T>
  T *configReceiver(const YAML::Node &sensor_config, const std::string &type, const int &msg_source);

  template <typename T>
  T *configTransmitter(const YAML::Node &sensor_config, const std::string &type, bool send_msg_ros, bool send_msg_proto);

private:

  common::ErrCode initLidar(const YAML::Node &config);

  inline void localLidarPointsCallback(const common::LidarPointsMsg &msg)
  {
    std::lock_guard<std::mutex> lock(mutex_points_);
    if (!lidarPointscbs_.empty())
    {
      for (auto &cb : lidarPointscbs_)
        cb(msg);
    }
  }

  inline void localExceptionCallback(const common::ErrCode &code)
  {
    std::lock_guard<std::mutex> lock(mutex_exception_);
    if (!excbs_.empty())
    {
      for (auto &excb : excbs_)
        excb(code);
    }
  }

  std::vector<std::function<void(const common::ErrCode &)>> excbs_;
  std::vector<std::function<void(const common::LidarPointsMsg &)>> lidarPointscbs_;


private:
  bool use_ros_;
  bool use_proto_;
  bool run_flag_;
  bool use_lidar_;
  bool lidarpkts_run_flag_;
  bool lidarpoints_run_flag_;
  std::string config_path_;

private:
  std::vector<common::LidarPacketsInterface *> lidar_packets_receivers_;
  std::vector<common::LidarPointsInterface *> lidar_points_receivers_;
  std::vector<common::LidarPacketsInterface *> lidar_packets_ros_transmitters_;
    std::vector<common::LidarPacketsInterface *> lidar_packets_proto_transmitters_;
  std::vector<common::LidarPointsInterface *> lidar_points_ros_transmitters_;
  std::vector<common::LidarPointsInterface *> lidar_points_proto_transmitters_;
  std::shared_ptr<std::thread> ros_thread_ptr_;

private:
  std::mutex mutex_exception_;
  std::mutex mutex_points_;
  std::mutex mutex_scan_;
  std::mutex mutex_packets_;

private:
  template <class R>
  R *construct(const std::string &device_type, const std::string &frame_id);
  template <class R, class T>
  inline R *localConstruct(uint16_t api_request)
  {
    if ((api_request | T::getApi()) != 0)
    {
      return dynamic_cast<R *>(new T);
    }
    else
    {
      return NULL;
    }
  }
  std::map<std::string, std::map<std::string, common::CommonBase *>> sensors_;
};
}; // namespace sensor
} // namespace robosense
