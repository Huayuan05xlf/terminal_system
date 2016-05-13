#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "my_protocol.h"

/************************************************
*函数功能：修改数据包的数据内容，最多包含两串字符串，使用前注意清空结构体
*参数：数据包结构体指针（默认指针有效），字符串，字符串指针为空表示无数据
*返回值：void
************************************************/
static void SetBuff(packet *pdata, const char *str1, const char *str2)
{
	if(NULL == str1)
	{	if(NULL == str2)					//空数据包
		{	pdata->head.data_size[0] = 0;
			pdata->head.data_size[1] = 0;
		}
		else								//只有第二串数据
		{	pdata->head.data_size[0] = 0;
			pdata->head.data_size[1] = strlen(str2);
			strcpy(pdata->buff,str2);
		}
	}
	else
	{	if(NULL == str2)					//只有第一串数据
		{	pdata->head.data_size[0] = strlen(str1);
			pdata->head.data_size[1] = 0;
			strcpy(pdata->buff,str1);
		}
		else								//写入两串数据
		{	pdata->head.data_size[0] = strlen(str1);
			pdata->head.data_size[1] = strlen(str2);
			sprintf(pdata->buff,"%s%s",str1,str2);
		}
	}
	pdata->head.size = strlen(pdata->buff) + sizeof(phead);		//设置数据包长度
}
/************************************************
*函数功能：读取数据包的数据内容，最多包含两串字符串，使用前注意清空字符串
*参数：socket文件描述符，字符串（默认字符串指针有效）
*返回值：成功返回1，如果写端关闭返回0，出错返回-1
************************************************/
static int GetBuff(int sockfd, char *str1, char *str2)
{
	int data_size[2];
	size_t readsize = read(sockfd,data_size,2*sizeof(int));	//读取数据长度，根据数据长度读取数据		//至少读取8字节
	if(readsize <= 0)
		return (int)readsize;				//read异常
	if(0 != data_size[0] && NULL != str1)	//数据长度不为零才读数据
	{	if((readsize = read(sockfd,str1,data_size[0])) <= 0)
			return (int)readsize;			//read异常
		str1[readsize] = '\0';
	}
	if(0 != data_size[1] && NULL != str2)	//数据长度不为零才读数据
	{	if((readsize = read(sockfd,str2,data_size[1])) <= 0)
			return (int)readsize;			//read异常
		str2[readsize] = '\0';
	}
	return 1;
}

/*=====================|以上函数为静态函数|=====================*/

/************************************************
*函数功能：写入心跳数据包（当前时间）
*参数：数据包结构体指针
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetHeart(packet *pdata)
{
	if(NULL == pdata)
		return -2;
	pdata->head.data_type = TYPE_HEART;
	time_t now = time(NULL);
	struct tm *ptm = localtime(&now);		//获取时间
	char stime[20];							//时间字符串
	sprintf(stime,"%4d-%02d-%02d %02d:%02d:%02d",\
		ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,\
		ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
	SetBuff(pdata, stime, NULL);
	return 0;
}
/************************************************
*函数功能：将用户注册登陆信息写入数据包（账号+密码）（空账号密码表示退出登陆）
*参数：数据包结构体指针，账号、密码字符串，数据包类型（注册、登陆）
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetUserData(packet *pdata, char *ID, char *psd, int type)
{
	if(NULL == pdata)
		return -2;
	if(TYPE_REGISTER != type && TYPE_LOGIN != type)
		return -2;
	pdata->head.data_type = type;
	SetBuff(pdata, ID, psd);
	return 0;
}
/************************************************
*函数功能：将shell命令写入数据包（命令）
*参数：数据包结构体指针，命令字符串
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetCommand(packet *pdata, char *cmd)
{
	if(NULL == pdata || NULL == cmd)
		return -2;
	pdata->head.data_type = TYPE_COMMAND;
	SetBuff(pdata, cmd, NULL);
	return 0;
}
/************************************************
*函数功能：将shell命令执行结果写入数据包（命令+结果）（空命令表示未发完的结果）
*参数：数据包结构体指针，结果字符串
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetResult(packet *pdata, char *cmd, char *result)
{
	if(NULL == pdata || NULL == result)
		return -2;
	pdata->head.data_type = TYPE_RESULT;
	SetBuff(pdata, cmd, result);
	return 0;
}
/************************************************
*函数功能：将要发给其他用户的消息写入数据包（账号+消息）
*参数：数据包结构体指针，账号、消息字符串
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetMessage(packet *pdata, char *chatID, char *msg)
{
	if(NULL == pdata || NULL == chatID || NULL == msg)
		return -2;
	pdata->head.data_type = TYPE_MESSAGE;
	SetBuff(pdata, chatID, msg);
	return 0;
}
/************************************************
*函数功能：发送在线用户信息，客户端发送空数据包，服务器返回在线用户字符串
*参数：数据包结构体指针，在线用户字符串
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetOnlineInfo(packet *pdata, char *online_user)
{
	if(NULL == pdata)
		return -2;
	pdata->head.data_type = TYPE_ONLINE;
	SetBuff(pdata, online_user, NULL);
	return 0;
}
/************************************************
*函数功能：发送只含有协议头数据包（空数据包）（服务器返回值、客户端退出等）
*参数：数据包结构体指针，数据包类型
*返回值：成功返回0，传参错误返回-2
************************************************/
int SetNone(packet *pdata, int type)
{
	if(NULL == pdata)
		return -2;
	if(TYPE_OK != type && TYPE_ERROR != type \
		&& TYPE_ERROR_2 != type && TYPE_ERROR_3 != type \
		&& TYPE_ERROR_4 != type && TYPE_EXIT != type)
		return -2;
	pdata->head.data_type = type;
	SetBuff(pdata,NULL,NULL);
	return 0;
}
/************************************************
*函数功能：读取数据包类型或长度（含协议头）（需要调用2次）
*参数：socket文件描述符
*返回值：成功返回数据包类型，如果写端关闭返回0，出错返回-1
************************************************/
int ReadTypeOrSize(int sockfd)
{
	int type_size = 1000;
	size_t readsize = read(sockfd,&type_size,sizeof(int));
	if(readsize > 0)
		return type_size;//数据包类型//空数据包至少16字节（协议头）
	else
		return (int)readsize;	//read异常
}
/************************************************
*函数功能：读取心跳包数据（时间）
*参数：socket文件描述符，时间存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadHeart(int sockfd, char *stime)
{
	if(NULL == stime)
		return -2;
	int retval = GetBuff(sockfd, stime, NULL);
	return retval;
}
/************************************************
*函数功能：读取用户注册登陆数据（账号+密码）
*参数：socket文件描述符，账号、密码存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadUserData(int sockfd, char *ID, char *psd)
{
	if(NULL == ID || NULL == psd)
		return -2;
	int retval = GetBuff(sockfd, ID, psd);
	return retval;
}
/************************************************
*函数功能：读取shell命令（命令）
*参数：socket文件描述符，命令存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadCommand(int sockfd, char *cmd)
{
	if(NULL == cmd)
		return -2;
	int retval = GetBuff(sockfd, cmd, NULL);
	return retval;
}
/************************************************
*函数功能：读取shell命令执行结果（命令+结果）
*参数：socket文件描述符，结果存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadResult(int sockfd, char *cmd, char *result)
{
	if(NULL == cmd || NULL == result)
		return -2;
	int retval = GetBuff(sockfd, cmd, result);
	return retval;
}
/************************************************
*函数功能：读取消息数据（账号+消息）
*参数：socket文件描述符，账号、消息存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadMessage(int sockfd, char *chatID, char *msg)
{
	if(NULL == chatID || NULL == msg)
		return -2;
	int retval = GetBuff(sockfd, chatID, msg);
	return retval;
}
/************************************************
*函数功能：读取在线用户信息，服务器接收空数据包，客户端接收在线用户字符串
*参数：socket文件描述符，在线用户存放的字符串
*返回值：成功返回1，如果写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadOnlineInfo(int sockfd, char *online_user)
{
	if(NULL == online_user)
		return -2;
	int retval = GetBuff(sockfd, online_user, NULL);
	return retval;
}
/************************************************
*函数功能：读取空数据包（空数据包）（服务器返回值、客户端退出）
*参数：socket文件描述符
*返回值：成功返回1，如果写端关闭返回0，出错返回-1
************************************************/
int ReadNone(int sockfd)
{
	int retval = GetBuff(sockfd, NULL, NULL);
	return retval;
}
/************************************************
*函数功能：解析收到的数据包（读取socket）
*参数：socket文件描述符，协议头指针，两个字符串
*返回值：成功返回读取的字节数，写端关闭返回0，出错返回-1，传参错误返回-2
************************************************/
int ReadPacket(int sockfd, phead *head, char *str1, char *str2)
{
	if(NULL == head || NULL == str1 || NULL == str2)
		return -2;
	int retval = 1, readsize = sizeof(phead);
	memset(head, 0, readsize);				//清空协议头结构体
	head->data_type = ReadTypeOrSize(sockfd);
	head->size = ReadTypeOrSize(sockfd);	//执行这两句会读取8字节
	str1[0] = str2[0] = '\0';				//字符串长度设为0;
	switch(head->data_type)					//会读取协议头中的数据长度来决定读取读取大小（至少读取8字节）
	{	case TYPE_HEART:					//心跳数据包
			retval = ReadHeart(sockfd,str1);
			break;
		case TYPE_REGISTER:					//注册数据包
		case TYPE_LOGIN:					//登陆数据包
			retval = ReadUserData(sockfd,str1,str2);
			break;
		case TYPE_MESSAGE:					//消息数据包
			retval = ReadMessage(sockfd,str1,str2);
			break;
		case TYPE_COMMAND:					//shell命令数据包
			retval = ReadCommand(sockfd,str1);
			break;
		case TYPE_RESULT:					//shell命令结果包
			retval = ReadResult(sockfd,str1,str2);
			break;
		case TYPE_ONLINE:					//查看在线用户
			retval = ReadOnlineInfo(sockfd,str1);
			break;
		case TYPE_OK:						//服务器返回正确信息
		case TYPE_ERROR:					//服务器返回错误1
		case TYPE_ERROR_2:					//服务器返回错误2
		case TYPE_ERROR_3:					//服务器返回错误3
		case TYPE_ERROR_4:					//服务器返回错误4
		case TYPE_EXIT:						//客户端正常退出
			retval = ReadNone(sockfd);
			break;
		default:return head->data_type;		//读取数据包类型失败
	}
	if(retval <= 0)
		return retval;						//读取数据失败
	head->data_size[0] = strlen(str1);
	head->data_size[1] = strlen(str2);
	readsize += (head->data_size[0] + head->data_size[1]);
	if(readsize != head->size)
		readsize = -1;						//数据包长度校验
#if 0
	printf("[%#06x](%02d+%02d+16=%03d)%s{%s}{%s}%s<read: %d>\n",\
		head->data_type,head->data_size[0],head->data_size[1],\
		head->size,YELLOW,str1,str2,NONE,readsize);	//测试读显示
#endif
	return readsize;
}
/************************************************
*函数功能：将打包好的数据包发送出去（写入socket）
*参数：socket文件描述符，数据包结构体指针
*返回值：成功返回写入的字节数，出错返回-1，传参错误返回-2
************************************************/
int WritePacket(int sockfd, const packet *pdata)
{
	if(NULL == pdata)
		return -2;
	if(0 == pdata->head.data_type)		//未指定数据包类型
		return -2;
	int writesize = write(sockfd, pdata, pdata->head.size);
	if(writesize != pdata->head.size)
		writesize = -1;					//数据包长度校验
#if 0
	printf("[%#06x](%02d+%02d+16=%03d)%s{%s}%s<write: %d>\n",\
		pdata->head.data_type,pdata->head.data_size[0],\
		pdata->head.data_size[1],pdata->head.size,\
		BLUE,pdata->buff,NONE,writesize);	//测试写显示
#endif
	return writesize;
}