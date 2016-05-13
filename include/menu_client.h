#ifndef __MENU_CLIENT_H__
#define __MENU_CLIENT_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "my_type.h"
#include "my_protocol.h"
/*================|客户端功能调用|================*/
//客户端初始化
extern int ClientInit(const char *IPv4_addr, int port);
//客户端运行
extern int ClientRun(int clientfd, pthread_t *ptr_thid);
//客户端正常退出
extern void ClientExit(int clientfd, pthread_t *ptr_thid);
//客户端子线程操作
extern void* ClientPthread(void *arg);
//定时发送心跳数据包
extern void HeartBeat(int signum);
//检查服务器是否运行
extern void CheckServer(int signum);
/*================|菜单界面函数|================*/
//游客界面
extern int MenuGuestCommand(int clientfd, int *state);
//用户界面
extern int MenuUserCommand(int clientfd, int *state);
/*================|注册登陆函数|================*/
//注册函数
extern int Register(int clientfd);
//登陆函数
extern int Login(int clientfd, int *state);
//退出登陆
extern int Logout(int clientfd, int *state);
//显示注册界面，输入注册信息
extern void GetRegisterInfo(char *userID, char *passwd, int length);
//显示登陆界面，输入登陆信息
extern void GetLoginInfo(char *userID, char *passwd, int length);
/*================|服务器其余操作|================*/
//发送shell命令
extern int SendShell(int clientfd);
//查看在线用户
extern int ShowOnline(int clientfd);
//用户聊天
extern int SendChatMessage(int clientfd, char *chatID);
/*================|其他操作函数|================*/
//清空输入缓冲区
extern void EmptyInputBuffer();
//消息输入
extern void GetMessage(char *msg, int length);
//密码输入
extern void GetPassword(char *password, int length);
//加密
extern char CharEncrypt(char ch);
//解密
extern char CharDecrypt(char ch);
//字符串解密
extern char* StringDecrypt(char *str);
//读取一个字符，不回显
extern char mygetch();
//还原终端
extern void resetmygetch();

#endif/*menu_client.h*/