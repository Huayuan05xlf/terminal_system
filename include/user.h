#ifndef __USER_H__
#define __USER_H__
#include <time.h>
#include "my_type.h"
typedef struct user_information
{	char userID[MAXSIZE_STR];		//账号
	char passwd[MAXSIZE_STR];		//密码
	time_t creation_time;			//账号创建时间
}user_data;
typedef struct user_node
{	user_data user;					//用户信息
	struct user_node *prior;		//前一节点
	struct user_node *next;			//后一节点
}userlink;
/*================|双向链表操作|================*/
//创建链表
extern userlink* CreateUser();
//销毁链表
extern void DestroyUser(userlink *ptr_head);
//根据账号查找用户
extern userlink* FindUser(userlink *ptr_head, char *userID);
//删除用户
extern userlink* DeleteUser(userlink *ptr_head, userlink *ptr_del);
//在链表尾新增用户
extern userlink* InsertUserEnd(userlink *ptr_head);
/*================|结构体数据显示|================*/
//显示所有用户
extern void ShowAllUser(userlink *ptr_head);

#endif	/*user.h*/