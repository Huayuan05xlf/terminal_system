#ifndef __ONLINE_H__
#define __ONLINE_H__
#include <pthread.h>
#include "user.h"
typedef struct online_client_information
{	int clientfd;				//客户端文件描述符
	short user_state;			//用户状态
	short heart;				//心跳值
	userlink *current_user;		//当前用户指针
	pthread_t pth_id;			//处理客户端的线程ID
	char IPv4_addr[16];			//客户端IP
	int port;					//端口
	time_t connect_time;		//连接时间
}client_data;
typedef struct client_node
{	client_data client;			//客户端信息
	struct client_node *prior;	//前一节点
	struct client_node *next;	//后一节点
}online;
/*================|双向环链操作|================*/
//创建链表
extern online* CreateOnline();
//销毁链表
extern void DestroyOnline(online *ptr_head);
//根据用户账号查询客户端
extern online *FindClientByID(online *ptr_head, char *userID);
//删除客户端
extern online *DeleteClient(online *ptr_head, online* ptr_del);
//新建节点插入链表头节点之前
extern online* InsertClientPrior(online *ptr_head);
/*================|结构体数据修改|================*/
//设置客户端初始属性
extern void InitClientData(online *ptr_tmp, int fd, pthread_t thid);
//客户端登陆
extern void ClientLogin(online *ptr_client, userlink *ptr_user);
//客户端注销
extern void ClientLogout(online *ptr_client);
//所有客户端心跳值增加
extern void HeartAdd(online *ptr_head);
/*================|结构体数据显示|================*/
//显示链表中所有节点
extern void ShowAllClient(online *ptr_head);

#endif	/*online.h*/