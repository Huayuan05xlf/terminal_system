#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "online.h"

/************************************************
*函数功能：创建在线链表节点，初始化客户端节点数据（客户端连接之后创建）
*参数：无
*返回值：新建节点的指针，创建出错返回NULL
************************************************/
online* CreateOnline()
{
	online *ptr_head = (online*)malloc(sizeof(online));
	if(NULL == ptr_head)
		return NULL;
	ptr_head->client.user_state  = 0;	//初始状态，此时处理客户端的线程不进行任何操作，直到这个值被修改
	ptr_head->client.heart  = 0;		//心跳值清零
	ptr_head->client.pth_id = 0;		//这个值为零表示空节点，无数据
	ptr_head->client.current_user = NULL;	//指针为空表示用户未登录
	ptr_head->prior = ptr_head;
	ptr_head->next  = ptr_head;
	return ptr_head;
}
/************************************************
*函数功能：销毁链表
*参数：在线链表头指针
*返回值：void，传空指针不进行操作
************************************************/
void DestroyOnline(online *ptr_head)
{
	if(NULL == ptr_head)
		return ;
	online *ptr_free = ptr_head;
	online *ptr_next = ptr_head->next;
	while(1)
	{	free(ptr_free);
		if(ptr_head == ptr_next)		//循环到了头节点
			break;
		ptr_free = ptr_next;
		ptr_next = ptr_next->next;
	}
}
/************************************************
*函数功能：根据用户账号查询客户端（在线用户）
*参数：在线链表头指针，账号字符串
*返回值：找到客户端返回该客户端的指针，找不到或传参错误返回NULL
************************************************/
online *FindClientByID(online *ptr_head, char *userID)
{
	if(NULL == ptr_head || NULL == userID)
		return NULL;
	online *ptr_find = ptr_head;
	userlink *ptr_user;
	while(1)
	{	ptr_user = ptr_find->client.current_user;
		if(NULL != ptr_user)					//未登录用户不查找
			if(!strcmp(userID, ptr_user->user.userID))
				return ptr_find;
		if(ptr_head == ptr_find->next)
			return NULL;
		ptr_find = ptr_find->next;
	}
}
/************************************************
*函数功能：删除客户端节点，如果要删除的是头节点，将第下一个客户端作为新的头节点
*参数：在线链表头指针，要删除的客户端指针
*返回值：删除操作完成之后的链表头指针，删除的是最后一个节点或传参错误返回NULL
************************************************/
online *DeleteClient(online *ptr_head, online* ptr_del)
{
	if(NULL == ptr_head || NULL == ptr_del)
		return NULL;
	if(ptr_head == ptr_del)				//如果要删除的是链表头节点
		ptr_head = ptr_head->next;
	if(ptr_head == ptr_head->next)		//链表只有这一个节点
		ptr_head = NULL;
	else
	{	ptr_del->prior->next = ptr_del->next;
		ptr_del->next->prior = ptr_del->prior;
	}
	free(ptr_del);
	return ptr_head;
}
/************************************************
*函数功能：新建节点插入链表头节点之前，初始化指针数据
*参数：在线链表头指针（如果传入空指针，新建头节点）
*返回值：新建节点的指针作为新的头节点，创建出错返回原来的头节点
************************************************/
online* InsertClientPrior(online *ptr_head)
{
	online *ptr_new = CreateOnline();	//新建节点
	if(NULL == ptr_new)
		return ptr_head;
	if(NULL != ptr_head)
	{	ptr_new->prior = ptr_head->prior;
		ptr_new->next  = ptr_head;
		ptr_head->prior->next = ptr_new;
		ptr_head->prior = ptr_new;
	}
	return ptr_new;
}
/************************************************
*函数功能：设置客户端初始属性，客户端发起连接，线程创建完成后执行
*参数：客户端指针，客户端套接字文件描述符，处理客户端的线程ID
*返回值：void
************************************************/
void InitClientData(online *ptr_client, int fd, pthread_t thid)
{
	if(NULL == ptr_client)
		return ;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	getpeername(fd,(struct sockaddr*)&addr,&addr_len);
	ptr_client->client.port = ntohs(addr.sin_port);	//端口
	char *IP = ptr_client->client.IPv4_addr;
	inet_ntop(AF_INET,&addr.sin_addr.s_addr,IP,16);	//IP地址
	ptr_client->client.connect_time = time(NULL);	//连接时间
	ptr_client->client.clientfd = fd;
	ptr_client->client.pth_id = thid;
	ptr_client->client.user_state = USER_LOGOUT;	//用户未登录
}
/************************************************
*函数功能：设置客户端属性，用户登陆成功后执行
*参数：客户端指针，用户指针
*返回值：void
************************************************/
void ClientLogin(online *ptr_client, userlink *ptr_user)
{
	if(NULL == ptr_client || NULL == ptr_user)
		return ;
	ptr_client->client.current_user = ptr_user;
	ptr_client->client.user_state = USER_LOGIN;		//用户登录
}
/************************************************
*函数功能：设置客户端属性，用户退出登陆后执行
*参数：客户端指针
*返回值：void
************************************************/
void ClientLogout(online *ptr_client)
{
	if(NULL == ptr_client)
		return ;
	ptr_client->client.current_user = NULL;
	ptr_client->client.user_state = USER_LOGOUT;	//用户未登录
}
/************************************************
*函数功能：增加心跳值，遍历链表，标记异常客户端的线程（信号处理函数中调用）
*参数：在线链表头指针
*返回值：void
************************************************/
void HeartAdd(online *ptr_head)
{
	if(NULL == ptr_head)
		return ;
	online *ptr_find = ptr_head;
	pthread_t thid;
	while(1)
	{	if(0 != ptr_find->client.user_state);
		{	ptr_find->client.heart += 1;			//心跳值增加
			thid = ptr_find->client.pth_id;
			if(ptr_find->client.heart >= MAXVAL_HEART)
			{	ptr_find->client.pth_id = ERROR_FLAG;//标记异常线程
				fprintf(stderr,"%s心跳异常----->IP:%s(%d)%s\n",\
					RED,ptr_find->client.IPv4_addr,\
					ptr_find->client.port,NONE);
				if(thid == pthread_self())	//在哪个线程中调用次函数
					pthread_exit(NULL);		//退出本线程
				else
					pthread_cancel(thid);	//取消其他线程
			}
		}
		if(ptr_head == ptr_find->next)
			break;
		ptr_find = ptr_find->next;
	}
}
/************************************************
*函数功能：显示链表中所有节点
*参数：在线链表头指针
*返回值：void
************************************************/
void ShowAllClient(online *ptr_head)
{
	if(NULL == ptr_head)
		return ;
	online *ptr_show = ptr_head;
	int clientfd, count = 0;
	pthread_t pth_id;
	userlink *ptr_user;
	printf("%s=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n",YELLOW);
	while(1)
	{	ptr_user = ptr_show->client.current_user;
		clientfd = ptr_show->client.clientfd;
		pth_id = ptr_show->client.pth_id;
		printf("--->IP:%s(%d) clientfd = %2d, pth_id = %#15lx",\
			ptr_show->client.IPv4_addr,ptr_show->client.port,\
			clientfd,pth_id);
		if(NULL != ptr_user)
			printf(" ID:%s ",ptr_user->user.userID);
		else
			printf(" 未登录…… ");
		printf("[heart = %d]\n",ptr_show->client.heart);
		count++;
		if(ptr_head == ptr_show->next)		//循环到了头节点
			break;
		ptr_show = ptr_show->next;
	}
	printf("=========================>>>>已连接 %d 个客户端%s\n",count,NONE);
}