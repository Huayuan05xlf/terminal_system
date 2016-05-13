#ifndef __MENU_SERVER_H__
#define __MENU_SERVER_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "my_type.h"
#include "my_protocol.h"
#include "online.h"
/*================|服务器功能调用|================*/
//服务器初始化
extern int ServerInit();
//服务器运行
extern void ServerRun();
//服务器子线程
extern void* ServerPthread(void *arg);
//服务器功能
extern void ServerFunction(int clientfd, userlink *ptr_head, online* ptr_client);
//心跳检查
extern void CheckHeart(int signum);
//服务器退出
extern void ServerExit(int signum);
/*================|服务器文件操作|================*/
//读取服务器配置文件
extern void LoadConfig(char *IPv4_addr, int *port);
//加载用户文件
extern void LoadUser(userlink *ptr_head);
//保存用户文件
extern void SaveUser(userlink *ptr_head);
//写入日志文件
extern void WriteLog(int type, char *str1, char *str2, online* ptr_client);
/*================|注册登陆函数|================*/
//用户注册
extern int UserRegister(userlink *ptr_head, char *ID, char *psd);
//用户登陆
extern int UserLogin(userlink *ptr_user, char *ID, char *psd, online* ptr_client);
/*================|数据处理函数|================*/
//心跳包接收
extern int HeartData(online* ptr_client);
//消息包处理
extern int HandleMessage(userlink *ptr_user, char *ID, char *msg, online* ptr_client);
//执行shell命令
extern int ExecuteShell(char *cmd, char *result, int length, packet *ptr_pdata, int clientfd);
//查看在线用户
extern int SearchOnline(char *online_user, int length, online* ptr_client);
/*================|其他操作函数|================*/




#endif/*menu_server.h*/