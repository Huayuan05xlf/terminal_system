#ifndef __MY_PROTOCOL_H__
#define __MY_PROTOCOL_H__
#include "my_type.h"
#define TYPE_HEART		0x0001	//心跳数据包
#define TYPE_REGISTER	0x0002	//注册数据包
#define TYPE_LOGIN		0x0004	//登陆数据包
#define TYPE_MESSAGE	0x0008	//消息数据包
#define TYPE_COMMAND	0x0010	//shell命令数据包
#define TYPE_RESULT		0x0020	//shell命令结果包
#define TYPE_ONLINE		0x0040	//查看在线用户
#define TYPE_OK			0x0080	//服务器返回正确信息
#define TYPE_ERROR		0x0100	//服务器返回错误1
#define TYPE_ERROR_2	0x0200	//服务器返回错误2
#define TYPE_ERROR_3	0x0400	//服务器返回错误3
#define TYPE_ERROR_4	0x0800	//服务器返回错误4
#define TYPE_EXIT		0xFFFF	//客户端正常退出
typedef struct my_protocol_head
{	int data_type;				//数据包类型
	int size;					//数据包长度（含协议头）
	int data_size[2];			//数据长度，2种数据
}phead;
typedef struct
{	phead head;					//协议头
	char buff[MAXSIZE_DATA];	//数据
}packet;
/*================|打包数据包|================*/
//写入心跳数据包（当前时间）
extern int SetHeart(packet *pdata);
//将用户注册登陆信息写入数据包（账号+密码）
extern int SetUserData(packet *pdata, char *ID, char *psd, int type);
//将shell命令写入数据包（命令）
extern int SetCommand(packet *pdata, char *cmd);
//将shell命令执行结果写入数据包（命令+结果）
extern int SetResult(packet *pdata, char *cmd, char *result);
//将要发给其他用户的消息写入数据包（账号+消息）
extern int SetMessage(packet *pdata, char *chatID, char *msg);
//发送在线用户信息（客户端：空数据包；服务器：在线用户）
extern int SetOnlineInfo(packet *pdata, char *online_user);
//发送空数据包（空数据包）
extern int SetNone(packet *pdata, int type);
/*================|读取数据包|================*/
//读取数据包类型或长度（含协议头）（需要调用2次）
extern int ReadTypeOrSize(int sockfd);
//读取心跳包数据（时间）
extern int ReadHeart(int sockfd, char *stime);
//读取用户注册登陆数据（账号+密码）
extern int ReadUserData(int sockfd, char *ID, char *psd);
//读取shell命令（命令）
extern int ReadCommand(int sockfd, char *cmd);
//读取shell命令执行结果（命令+结果）
extern int ReadResult(int sockfd, char *cmd, char *result);
//读取消息数据（账号+消息）
extern int ReadMessage(int sockfd, char *chatID, char *msg);
//读取在线用户信息（客户端：在线用户；服务器：空数据包）
extern int ReadOnlineInfo(int sockfd, char *online_user);
//读取空数据包（空数据包）
extern int ReadNone(int sockfd);
/*================|收发数据包|================*/
//解析收到的数据包（读取socket）
extern int ReadPacket(int sockfd, phead *head, char *str1, char *str2);
//将打包好的数据包发送出去（写入socket）
extern int WritePacket(int sockfd, const packet *pdata);

#endif	/*my_protocol.h*/