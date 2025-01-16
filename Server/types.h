#ifndef EVENTTYPES_H
#define EVENTTYPES_H

enum EventType {
    // 用户事件
    EVENT_LOGIN_REQUEST = 1000,      // 用户登录请求
    EVENT_LOGIN_RESPONSE,            // 登录响应
    EVENT_LOGOUT_RESPONSE,           // 登出响应
    EVENT_UPDATE_STATUS,             // 更新用户在线状态
    EVENT_GET_FRIENDLIST,            // 获取用户好友列表
    EVENT_GET_USERPROFILE,           // 获取用户个人资料
    EVENT_GET_GROUPLIST,             // 获取群组列表
    EVENT_GET_GROUPMEMBERS,          // 获取群组成员列表
    EVENT_REQUEST_REGISTER,          // 注册请求
    EVENT_QUERY_USER,                // 查询用户信息
    EVENT_QUERY_GROUP,               // 查询群组信息
    EVENT_REQUEST_ADD_FRIEND,        // 请求添加好友
    EVENT_ADD_FRIENDREMARK,          // 添加好友备注
    EVENT_RENAME_GROUP,              // 重命名群组

    // 消息事件
    EVENT_SEND_MESSAGE,              // 发送文本消息
    EVENT_RECV_MESSAGE,              // 接收文本消息
    EVENT_RECV_FRIENDLIST,           // 接收好友列表
    EVENT_RECV_GROUPLIST,            // 接收群组列表
    EVENT_SEND_FRIENDLOGIN,          // 发送好友上线信息
    EVENT_SEND_FRIENDLOGOUT,         // 发送好友下线信息
    EVENT_RECV_FRIENDLOGIN,          // 接收好友上线信息
    EVENT_RECV_FRIENDLOGOUT,         // 接收好友下线信息
    EVENT_RECV_USERPROFILE,          // 接收用户个人资料
    EVENT_SEND_OFFLINEMESSAGES,      // 发送用户离线时接收到的信息
    EVENT_RECV_OFFLINEMESSAGES,      // 接收用户离线时接收到的信息
    EVENT_RECV_USER,                 // 接收查询的用户信息
    EVENT_RECV_GROUP,                // 接收查询的群组信息
    EVENT_RECV_GROUPMEMBERS,         // 接收群组成员列表
    EVENT_CONFIRM_FRIENDREQUEST,     // 接受者返回添加好友状态 同意/拒绝
    EVENT_RECV_CONFIRM_FRIENDREQUEST,// 接收到好友添加状态结果
    EVENT_DELETE_FRIENDGROUP,        // 删除好友/群组
    EVENT_QUIT_GROUP,                // 退出群组
    EVENT_DELETED_RELATION,          // 被删除好友/踢出群组
    EVENT_RENAMED_GROUP,             // 群组被重命名
    EVENT_RECV_ADDFRIENDREQ,         // 接收到添加好友请求

    // 群组事件 管理功能待实现
    EVENT_CREATE_GROUP,              // 创建群组
    EVENT_JOIN_GROUP,                // 加入群组
    EVENT_LEAVE_GROUP,               // 退出群组
    EVENT_GROUP_MESSAGE,             // 群组消息

    // 数据传输事件
    EVENT_ERROR,                      // 错误事件
    EVENT_ACK                         // ACK数据包
};

#endif // EVENTTYPES_H
