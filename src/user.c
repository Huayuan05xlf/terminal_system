#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

/************************************************
*函数功能：创建用户链表节点，初始化节点数据
*参数：无
*返回值：新建节点的指针，创建出错返回NULL
************************************************/
userlink* CreateUser()
{
	userlink *ptr_head = (userlink*)calloc(1,sizeof(userlink));
	if(NULL == ptr_head)
		return NULL;
	ptr_head->user.creation_time = 0;	//这个值为零表示空节点，无数据
	ptr_head->prior = NULL;
	ptr_head->next  = NULL;
	return ptr_head;
}
/************************************************
*函数功能：销毁链表
*参数：用户链表头指针
*返回值：void，传参错误不进行操作
************************************************/
void DestroyUser(userlink *ptr_head)
{
	if(NULL == ptr_head)
		return ;
	userlink *ptr_free = ptr_head;
	userlink *ptr_next = ptr_head->next;
	while(1)
	{	free(ptr_free);
		if(NULL == ptr_next)			//循环到了尾节点
			break;
		ptr_free = ptr_next;
		ptr_next = ptr_next->next;
	}
}
/************************************************
*函数功能：根据账号查找用户
*参数：用户链表头指针，账号字符串
*返回值：找到用户返回该用户的指针，找不到或传参错误返回NULL
************************************************/
userlink* FindUser(userlink *ptr_head, char *userID)
{
	if(NULL == ptr_head || NULL == userID)
		return NULL;
	userlink *ptr_find = ptr_head;
	while(1)
	{	if(NULL == ptr_find)			//遍历结束，未找到用户
			return NULL;
		else if(!strcmp(userID, ptr_find->user.userID))
			return ptr_find;
		ptr_find = ptr_find->next;
	}
}
/************************************************
*函数功能：删除用户，如果要删除的是头节点，将第下一个用户作为新的头节点
*参数：用户链表头指针，要删除的用户指针
*返回值：删除操作完成之后的链表头指针，传参错误返回NULL
************************************************/
userlink* DeleteUser(userlink *ptr_head, userlink *ptr_del)
{
	if(NULL == ptr_head || NULL == ptr_del)
		return NULL;
	if(ptr_head == ptr_del)				//如果要删除的是链表头节点
		ptr_head = ptr_head->next;		//下一节点改为新的头节点
	else								//删除的不是头节点时
		ptr_del->prior->next = ptr_del->next;
	if(NULL != ptr_del->next)			//删除的不是尾节点时
		ptr_del->next->prior = ptr_del->prior;
	free(ptr_del);
	return ptr_head;
}
/************************************************
*函数功能：新建节点插入链表尾，初始化指针数据
*参数：用户链表头指针
*返回值：新建节点的指针，创建出错或传参错误返回NULL
************************************************/
userlink* InsertUserEnd(userlink *ptr_head)
{
	if(NULL == ptr_head)
		return NULL;
	userlink *ptr_new = CreateUser();	//新建空节点
	if(NULL == ptr_new)
		return NULL;
	userlink *ptr_tmp = ptr_head;
	while(NULL != ptr_tmp->next)
		ptr_tmp = ptr_tmp->next;		//找到尾节点
	ptr_tmp->next  = ptr_new;
	ptr_new->prior = ptr_tmp;
	return ptr_new;
}
/************************************************
*函数功能：显示链表中的所有用户
*参数：用户链表头指针
*返回值：void
************************************************/
void ShowAllUser(userlink *ptr_head)
{
	if(NULL == ptr_head)
		return ;
	if(0 == ptr_head->user.creation_time)
		return ;
	userlink *ptr_show = ptr_head;
	int count = 0;
	struct tm *ptm = NULL;
	printf("%s",GREEN);
	while(NULL != ptr_show)		//找到链表尾
	{	printf("账号：%10s 密码：%5s ",\
			ptr_show->user.userID,ptr_show->user.passwd);
		ptm = localtime(&ptr_show->user.creation_time);
		printf("注册时间：%4d-%02d-%02d %02d:%02d:%02d\n",\
			ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,\
			ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
		count++;
		ptr_show = ptr_show->next;
	}
	printf("------------------------->>>>已注册 %d 个用户%s\n",count,NONE);
}