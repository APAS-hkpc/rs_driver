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
#ifdef PROTO_FOUND

#include <rs_common/msg/rs_msg/lidar_points_msg.h>
#include <rs_common/msg/rs_msg/lidar_packet_msg.h>
#include <rs_common/msg/rs_msg/lidar_scan_msg.h>
#include <rs_common/msg/proto_msg/Proto_msg.LidarPoints.pb.h>
#include <rs_common/msg/proto_msg/Proto_msg.LidarPacket.pb.h>
#include <rs_common/msg/proto_msg/Proto_msg.LidarScan.pb.h>
#include <pcl/io/io.h>
namespace robosense
{
namespace common
{

/************************************************************************/
/**Translation functions between Robosense message and protobuf message**/
/************************************************************************/
inline Proto_msg::LidarPoints toProtoMsg(const LidarPointsMsg &rs_msg)
{
    Proto_msg::LidarPoints proto_msg;
    proto_msg.set_timestamp(rs_msg.timestamp);
    proto_msg.set_seq(rs_msg.seq);
    proto_msg.set_parent_frame_id(rs_msg.parent_frame_id);
    proto_msg.set_frame_id(rs_msg.frame_id);
    proto_msg.set_is_motion_correct(rs_msg.is_motion_correct);
    proto_msg.set_height(rs_msg.height);
    proto_msg.set_width(rs_msg.width);
    proto_msg.set_is_dense(rs_msg.is_dense);
    proto_msg.set_is_transform(rs_msg.is_transform);
    proto_msg.set_lidar_model(rs_msg.lidar_model);
    proto_msg.set_points_type(rs_msg.points_type);

    for (size_t i = 0; i < rs_msg.cloudPtr->size(); i++)
    {
        proto_msg.add_data(rs_msg.cloudPtr->points[i].x);
        proto_msg.add_data(rs_msg.cloudPtr->points[i].y);
        proto_msg.add_data(rs_msg.cloudPtr->points[i].z);
        proto_msg.add_data(rs_msg.cloudPtr->points[i].intensity);
    }

    return std::move(proto_msg);
}

inline LidarPointsMsg toRsMsg(const Proto_msg::LidarPoints &proto_msg)
{
    LidarPointsMsg rs_msg;
    rs_msg.timestamp = proto_msg.timestamp();
    rs_msg.seq = proto_msg.seq();
    rs_msg.parent_frame_id = proto_msg.parent_frame_id();
    rs_msg.frame_id = proto_msg.frame_id();
    rs_msg.is_motion_correct = proto_msg.is_motion_correct();
    rs_msg.height = proto_msg.height();
    rs_msg.width = proto_msg.width();
    rs_msg.is_dense = proto_msg.is_dense();
    rs_msg.is_transform = proto_msg.is_transform();
    rs_msg.lidar_model = proto_msg.lidar_model();
    rs_msg.points_type = proto_msg.points_type();
    PointCloud *ptr_tmp = new PointCloud();
    for (int i = 0; i < proto_msg.data_size(); i += 4)
    {
        pcl::PointXYZI point;
        point.x = proto_msg.data(i);
        point.y = proto_msg.data(i + 1);
        point.z = proto_msg.data(i + 2);
        point.intensity = proto_msg.data(i + 3);
        ptr_tmp->push_back(point);
    }
    ptr_tmp->height = rs_msg.height;
    ptr_tmp->width = rs_msg.width;
    ptr_tmp->is_dense = rs_msg.is_dense;
    rs_msg.cloudPtr.reset(ptr_tmp);
    return rs_msg;
}

inline Proto_msg::LidarScan toProtoMsg(const LidarScanMsg &rs_msg)
{
    Proto_msg::LidarScan proto_msg;
    proto_msg.set_timestamp(rs_msg.timestamp);
    proto_msg.set_seq(rs_msg.seq);
    for (auto iter : rs_msg.packets)
    {
        void *data_ptr = malloc(1248);
        memcpy(data_ptr, iter.packet.data(), 1248);
        proto_msg.add_data(data_ptr, 1248);
        free(data_ptr);
    }
    return proto_msg;
}

inline LidarScanMsg toRsMsg(const Proto_msg::LidarScan &proto_msg)
{
    LidarScanMsg rs_msg;
    rs_msg.timestamp = proto_msg.timestamp();
    rs_msg.seq = proto_msg.seq();
    for (int i = 0; i < proto_msg.data_size(); i++)
    {
        std::string data_str = proto_msg.data(i);
        LidarPacketMsg tmp_pkt;
        memcpy(tmp_pkt.packet.data(), data_str.data(), data_str.size());
        rs_msg.packets.emplace_back(tmp_pkt);
    }
    return rs_msg;
}

inline Proto_msg::LidarPacket toProtoMsg(const LidarPacketMsg &rs_msg)
{
    Proto_msg::LidarPacket proto_msg;
    proto_msg.set_timestamp(rs_msg.timestamp);
    void *data_ptr = malloc(1248);
    memcpy(data_ptr, rs_msg.packet.data(), 1248);
    proto_msg.set_data(data_ptr, 1248);
    free(data_ptr);
    return proto_msg;
}

inline LidarPacketMsg toRsMsg(const Proto_msg::LidarPacket &proto_msg)
{
    LidarPacketMsg rs_msg;
    rs_msg.timestamp = proto_msg.timestamp();
    std::string data_str = proto_msg.data();
    memcpy(rs_msg.packet.data(), data_str.data(), data_str.size());
    return rs_msg;
}


} // namespace common

} // namespace robosense
#endif // PROTO_FOUND
