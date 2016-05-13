#ifndef __MYTYPE_H__
#define __MYTYPE_H__
/************************************************
*这个头文件存放自定义的宏定义（一些常量、字符串）
************************************************/
/*==================|最大限制|==================*/
#define	MAXSIZE_UI		50			//界面显示
#define	MAXSIZE_STR		20			//账号密码字符串
#define	MAXSIZE_DATA	1024		//发送的数据长度
#define	MAXSIZE_PATH	1024		//文件路径长度
#define	MAXALARM_C		3			//客户端闹钟
#define	MAXALARM_S		3			//服务器闹钟
#define	MAXVAL_HEART	5			//最大心跳值
/*==================|其他常量|==================*/
#define	VERSION			"Ver1.0"	//版本号
#define	DEL				127			//ascii码
#define	PSD_KEY			1			//加密解密key值
#define	EXIT_FLAG		1			//程序退出标志
#define	NORMAL_FLAG		0			//程序正常标志
#define	ERROR_FLAG		-1			//程序出错返回值
#define	USER_LOGIN		1			//用户登陆状态
#define	USER_LOGOUT		-1			//用户未登陆状态
#define	USER_LOGERR		-2			//用户登陆错误
/*==================|颜色显示|==================*/
#define	NONE		"\033[0m"		//显示改为默认
#define	RED			"\033[1;31m"	//显示改为红色
#define	GREEN		"\033[1;32m"	//显示改为绿色
#define	YELLOW		"\033[1;33m"	//显示改为黄色
#define	BLUE		"\033[1;34m"	//显示改为蓝色
#define	PURPLE		"\033[1;35m"	//显示改为紫色
#define	CYANS		"\033[1;36m"	//显示改为青色
/*==================|文件路径|==================*/
#define	PATH_USER	"./file/user.txt"	//用户信息文件路径
#define	PATH_CONFIG	"./file/config.txt"	//服务器配置文件
#define	PATH_LOG	"./file/log.txt"	//服务器日志文件
#define	PATH_TMP	"./file/tmp/"		//服务器临时文件目录，存放执行shell命令的结果

#endif	/*my_type.h*/