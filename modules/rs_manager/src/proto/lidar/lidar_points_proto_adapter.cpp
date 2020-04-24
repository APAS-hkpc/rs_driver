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

#ifdef PROTO_FOUND
#include "rs_sensor/proto/lidar_points_proto_adapter.h"
#define RECEIVE_BUF_SIZE 10000000
namespace robosense
{
namespace sensor
{
using namespace robosense::common;
LidarPointsProtoAdapter::LidarPointsProtoAdapter() : old_frmNum_(0), new_frmNum_(0)
{
    setName("LidarPointsProtoAdapter");
    thread_pool_ptr_ = ThreadPool::getInstance();
}
ErrCode LidarPointsProtoAdapter::init(const YAML::Node &config)
{
    setinitFlag(true);
    int msg_source = 0;
    bool send_points_proto;
    std::string points_send_port;
    uint16_t points_recv_port;
    std::string points_send_ip;
    YAML::Node proto_config = yamlSubNodeAbort(config, "proto");
    yamlRead<int>(config, "msg_source", msg_source);
    yamlRead<bool>(config, "send_points_proto", send_points_proto, false);
    yamlReadAbort<std::string>(proto_config, "points_send_port", points_send_port);
    yamlReadAbort<std::string>(proto_config, "points_send_ip", points_send_ip);
    yamlReadAbort<uint16_t>(proto_config, "points_recv_port", points_recv_port);
    if (msg_source == 5)
    {
        INFO << "Receive Points From : Protobuf-UDP" << REND;
        INFO << "Receive Points Port: " << points_recv_port << REND;
        if (initReceiver(points_recv_port) == -1)
        {
            ERROR << "LidarPointsProtoAdapter: Create UDP Receiver Socket Failed OR Bind Network failed!" << REND;
            exit(-1);
        }
        send_points_proto = false;
    }
    if (send_points_proto)
    {
        DEBUG << "Send Points Through : Protobuf-UDP" << REND;
        DEBUG << "ReceSendive Points Port: " << points_send_port << REND;
        DEBUG << "ReceSendive Points IP: " << points_send_ip << REND;
        if (initSender(points_send_port, points_send_ip) == -1)
        {
            ERROR << "LidarPointsProtoAdapter: Create UDP Sender Socket Failed ! " << REND;
            exit(-1);
        }
    }
    return ErrCode_Success;
}

ErrCode LidarPointsProtoAdapter::start()
{
    buff_ = malloc(RECEIVE_BUF_SIZE);
    recv_thread_.start = true;
    recv_thread_.m_thread.reset(new std::thread([this]() { recvPoints(); }));
    return ErrCode_Success;
}
ErrCode LidarPointsProtoAdapter::stop()
{
    if (recv_thread_.start.load())
    {
        recv_thread_.start.store(false);
        recv_thread_.m_thread->join();
        free(buff_);
    }
    return ErrCode_Success;
}

void LidarPointsProtoAdapter::send(const LidarPointsMsg &msg) // Will send NavSatStatus and Odometry
{
    points_send_queue_.push(msg);
    if (points_send_queue_.is_task_finished.load())
    {
        points_send_queue_.is_task_finished.store(false);
        thread_pool_ptr_->commit([this]() { sendPoints(); });
    }
}

void LidarPointsProtoAdapter::sendPoints()
{
    while (points_send_queue_.m_quque.size() > 0)
    {
        Proto_msg::LidarPoints proto_msg = toProtoMsg(points_send_queue_.m_quque.front());
        if (!sendSplitMsg<Proto_msg::LidarPoints>(proto_msg))
        {
            reportError(ErrCode_LidarPointsProtoSendError);
        }
        points_send_queue_.pop();
    }
    points_send_queue_.is_task_finished.store(true);
}

void LidarPointsProtoAdapter::recvPoints()
{
    bool start_check = true;
    while (recv_thread_.start.load())
    {
        void *pMsgData = malloc(MAX_RECEIVE_LENGTH);
        proto_MsgHeader tmp_header;
        int ret = receiveProtoMsg(pMsgData, MAX_RECEIVE_LENGTH, tmp_header);
        if (start_check)
        {
            if (tmp_header.msgID == 0)
            {
                start_check = false;
            }
            else
            {
                continue;
            }
        }
        if (ret == -1)
        {
            reportError(ErrCode_LidarPointsProtoReceiveError);
            continue;
        }

        points_recv_queue_.push(std::make_pair(pMsgData, tmp_header));

        if (points_recv_queue_.is_task_finished.load())
        {
            points_recv_queue_.is_task_finished.store(false);
            thread_pool_ptr_->commit([&]() {
                splicePoints();
            });
        }
    }
}

void LidarPointsProtoAdapter::splicePoints()
{
    while (points_recv_queue_.m_quque.size() > 0)
    {
        if (recv_thread_.start.load())
        {
            auto pair = points_recv_queue_.m_quque.front();
            old_frmNum_ = new_frmNum_;
            new_frmNum_ = pair.second.frmNumber;
            memcpy((uint8_t *)buff_ + pair.second.msgID * SPLIT_SIZE, pair.first, SPLIT_SIZE);
            if ((old_frmNum_ == new_frmNum_) && (pair.second.msgID == pair.second.totalMsgCnt - 1))
            {
                Proto_msg::LidarPoints proto_msg;
                proto_msg.ParseFromArray(buff_, pair.second.totalMsgLen);
                localCallback(toRsMsg(proto_msg));
            }
        }
        free(points_recv_queue_.m_quque.front().first);
        points_recv_queue_.pop();
    }
    points_recv_queue_.is_task_finished.store(true);
}

} // namespace sensor
} // namespace robosense
#endif // ROS_FOUND