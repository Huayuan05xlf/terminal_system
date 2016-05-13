#include "menu_client.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int flag_type = 0;	//注册登陆操作、保存服务器返回的数据包类型
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int my_clientfd = 0;	//客户端套接字文件描述符，信号处理函数中使用
static char my_ID[MAXSIZE_STR];		//当前登陆的用户名，用于界面显示
static int flag_chat = 0;	//聊天标志，0为正在其他操作，1为正在聊天
static char *ptr_chat_ID = NULL;	//聊天对象ID字符串指针
static char *ptr_msg = NULL;//聊天消息字符串指针
/************************************************
*函数功能：客户端主函数
*参数：命令行参数
*返回值：成功返回0，出错返回-1
************************************************/
int main(int argc, char const *argv[])
{
	if(3 != argc)	//IP地址以及端口号作为命令行参数传入
	{	fprintf(stderr,"命令格式：%s [IP地址] [端口号]\n",argv[0]);
		return -1;
	}
	int clientfd = ClientInit(argv[1],atoi(argv[2]));
	if(-1 == clientfd)			//初始化客户端
		return -1;
	pthread_t thid;
	ClientRun(clientfd,&thid);	//运行客户端
	ClientExit(clientfd,&thid);	//客户端结束
	return 0;
}
/************************************************
*函数功能：客户端初始化，登记信号，连接服务器
*参数：服务器IP地址字符串，端口号
*返回值：成功返回客户端套接字文件描述符，出错返回-1
************************************************/
int ClientInit(const char *IPv4_addr, int port)
{
	if(SIG_ERR == signal(SIGALRM,HeartBeat))	//闹钟信号
	{	perror("signal error");
		return -1;
	}
	alarm(MAXALARM_C);							//开始定时
	if(SIG_ERR == signal(SIGPIPE,CheckServer))	//服务器异常信号
	{	perror("signal error");
		return -1;
	}
	int clientfd = socket(AF_INET,SOCK_STREAM,0);		//创建套接字
	if(-1 == clientfd)
	{	perror("socket error");
		return -1;
	}
	struct sockaddr_in serveraddr;
	memset(&serveraddr,0,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;					//IPv4
	serveraddr.sin_port = htons((short)port);			//端口号
	serveraddr.sin_addr.s_addr = inet_addr(IPv4_addr);	//IP地址
	if(connect(clientfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)))	//连接服务器
	{	perror("connect error");
		close(clientfd);
		return -1;
	}
	my_clientfd = clientfd;	//修改全局变量
	return clientfd;
}
/************************************************
*函数功能：客户端运行，开启读数据包的子线程
*参数：客户端套接字文件描述符，存放子线程ID的地址
*返回值：成功返回0，出错返回-1
************************************************/
int ClientRun(int clientfd, pthread_t *ptr_thid)
{
	if(NULL == ptr_thid)
		return -1;
	pthread_create(ptr_thid,NULL,ClientPthread,(void*)(long)clientfd);	//开启读线程
	int exit_flag = NORMAL_FLAG;	//程序正常标志，系统处于循环中
	int login_state = USER_LOGOUT;	//用户未登录
	while(1)
	{	if(EXIT_FLAG == exit_flag)	//程序正常退出
			return NORMAL_FLAG;
		switch(login_state)
		{	case USER_LOGOUT:		//游客界面
				exit_flag = MenuGuestCommand(clientfd,&login_state);
				break;
			case USER_LOGIN:		//用户界面
				exit_flag = MenuUserCommand(clientfd,&login_state);
				break;
			case USER_LOGERR:		//登陆出错，退出客户端
				exit_flag = EXIT_FLAG;
				break;
			default:				//程序异常
				exit_flag = ERROR_FLAG;
		}
		if(ERROR_FLAG == exit_flag)
		{	close(clientfd);
			printf("程序异常！\n（输入任意值退出……）\n");
			EmptyInputBuffer();
			getchar();
			exit(ERROR_FLAG);
		}
	}
}
/************************************************
*函数功能：客户端正常退出
*参数：客户端套接字文件描述符，存放子线程ID的地址
*返回值：void
************************************************/
void ClientExit(int clientfd, pthread_t *ptr_thid)
{
	if(NULL == ptr_thid)
		return ;
	packet pdata;
	memset(&pdata,0,sizeof(pdata));
	SetNone(&pdata,TYPE_EXIT);
	WritePacket(clientfd,&pdata);		//发送退出数据包
	pthread_join(*ptr_thid,NULL);		//等待读线程退出
	close(clientfd);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}
/************************************************
*函数功能：客户端子线程，负责读取服务器发送的数据包
*参数：客户端套接字文件描述符
*返回值：NULL
************************************************/
void* ClientPthread(void *arg)
{
	int clientfd = (int)(long)arg;
	phead head;
	int i, count;
	char buff1[MAXSIZE_DATA], buff2[MAXSIZE_DATA];
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	while(1)
	{	if(ReadPacket(clientfd,&head,buff1,buff2) <= 0)
			continue;
		switch(head.data_type)
		{	case TYPE_MESSAGE:					//消息数据包
				if(1 == flag_chat)
					printf("\r%s[%s]>>>>%s:%s",PURPLE,buff1,NONE,buff2);
				else
					printf("\n%s[%s]>>>>%s:%s\n",PURPLE,buff1,NONE,buff2);
				if(NULL != ptr_chat_ID && NULL != ptr_msg)
				{	count = strlen(ptr_chat_ID) + strlen(ptr_msg);
					count -= (strlen(buff1) + strlen(buff2));
					for(i = 0; i < count; ++i)
						putchar(' ');			//空格补齐
					printf("\n%s[%s]<<<<%s:%s",CYANS,ptr_chat_ID,NONE,ptr_msg);
					fflush(stdout);
				}
				break;
			case TYPE_RESULT:					//shell命令结果包
				if(head.data_size[0] != 0)
					printf("%s命令[%s]执行结果:%s\n",YELLOW,buff1,NONE);
				printf("%s",buff2);
				if(head.size != (sizeof(packet) - 1))
					printf("%sServer@shell:%s $",YELLOW,NONE);
				fflush(stdout);
				break;
			case TYPE_ONLINE:					//在线用户信息
				printf("在线用户如下：\n%s",buff1);
				break;
			case TYPE_OK:						//服务器返回正确信息
			case TYPE_ERROR:					//服务器返回错误1
			case TYPE_ERROR_2:					//服务器返回错误2
			case TYPE_ERROR_3:					//服务器返回错误3
			case TYPE_ERROR_4:					//服务器返回错误4
				pthread_mutex_lock(&mutex);
				if(0 == flag_type)
					flag_type = head.data_type;	//修改注册登陆标志
				pthread_mutex_unlock(&mutex);
				pthread_cond_signal(&cond);
				break;
			case TYPE_EXIT:						//客户端正常退出
				free(ptr_pdata);
				return NULL;
			default:							//其他数据包类型
				break;
		}
	}
}
/************************************************
*函数功能：定时发送心跳数据包，信号处理函数
*参数：信号值
*返回值：void
************************************************/
void HeartBeat(int signum)
{
	if(SIGALRM == signum)
	{	alarm(MAXALARM_C);
		if(0 != my_clientfd)
		{	packet pdata;
			memset(&pdata,0,sizeof(pdata));
			SetHeart(&pdata);
			WritePacket(my_clientfd,&pdata);	//发送心跳包
		}
	}
}
/************************************************
*函数功能：检查服务器是否运行，配合发送心跳包工作，信号处理函数
*参数：信号值，服务器套接字关闭，客户端发送心跳包会产生SIGPIPE信号
*返回值：void
************************************************/
void CheckServer(int signum)
{
	if(SIGPIPE == signum)
	{	printf("%s\n\t\t服务器异常！%s\n",RED,NONE);
		resetmygetch();
		close(my_clientfd);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
		exit(-1);
	}
}
/************************************************
*函数功能：客户端启动界面，注册登陆（游客）
*参数：无
*返回值：void
************************************************/
static void MenuGuestUI()
{
	system("clear");
	printf("远程客户端(%s)\n",VERSION);
	int i;
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('~');
	printf("\n\t\t1：用户注册\n\t\t2：用户登陆\n\t\t0：退出系统\n");
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('~');
	putchar('\n');
}
/************************************************
*函数功能：游客命令，显示界面，注册登陆功能
*参数：客户端套接字文件描述符，登陆状态
*返回值：返回(NORMAL_FLAG)表示界面需要切换，返回(EXIT_FLAG)表示退出系统
************************************************/
int MenuGuestCommand(int clientfd, int *state)
{
	if(NULL == state)
		return ERROR_FLAG;
	MenuGuestUI();
	int flag;
	while(1)
	{	printf("~~~~请输入指令（游客）：");
		while(!scanf("%d",&flag))
		{	EmptyInputBuffer();		//清空输入缓冲区
			printf("~~~~输入有误！请重新输入\n");
			printf("~~~~请输入指令（游客）：");
		}
		switch(flag)
		{	case 1 :Register(clientfd);						//注册
					break;
			case 2 :if(EXIT_FLAG == Login(clientfd,state))	//登陆
						return NORMAL_FLAG;
					break;
			case 0 :return EXIT_FLAG;	//退出标志，表示调用此函数的函数需要退出（退出系统）
			default:printf("~~~~输入有误！请重新输入\n");
		}
	}
}
/************************************************
*函数功能：用户登陆成功界面，用户操作
*参数：无
*返回值：void
************************************************/
static void MenuUserUI()
{
	system("clear");
	printf("远程客户端(%s)\n",VERSION);
	int i;
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('#');
	printf("\n\t1：发送服务器指令\n");
	printf("\t2：查看在线用户\n");
	printf("\t3：发送消息\n");
	printf("\t4：退出登陆\t0：退出系统\n");
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('#');
	putchar('\n');
}
/************************************************
*函数功能：用户命令
*参数：客户端套接字文件描述符，登陆状态
*返回值：返回(NORMAL_FLAG)表示界面需要切换，返回(EXIT_FLAG)表示退出系统
************************************************/
int MenuUserCommand(int clientfd, int *state)
{
	if(NULL == state)
		return ERROR_FLAG;
	MenuUserUI();
	int flag;
	while(1)
	{	printf("####请输入指令（%s）：",my_ID);
		while(!scanf("%d",&flag))
		{	EmptyInputBuffer();		//清空输入缓冲区
			printf("####输入有误！请重新输入\n");
			printf("####请输入指令（%s）：",my_ID);
		}
		switch(flag)
		{	case 1 :SendShell(clientfd);			//发送shell命令
					break;
			case 2 :ShowOnline(clientfd);			//显示在线用户
					break;
			case 3 :SendChatMessage(clientfd,NULL);	//发送聊天消息
					break;
			case 4 :if(EXIT_FLAG == Logout(clientfd,state))
						return NORMAL_FLAG;
					break;
			case 0 :return EXIT_FLAG;	//退出标志，表示调用此函数的函数需要退出（退出系统）
			default:printf("####输入有误！请重新输入\n");
		}
	}
}
/************************************************
*函数功能：注册函数
*参数：客户端套接字文件描述符
*返回值：出错返回-1
************************************************/
int Register(int clientfd)
{
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	char ID[MAXSIZE_STR];
	char psd[MAXSIZE_STR];
	GetRegisterInfo(ID,psd,MAXSIZE_STR);			//输入账号密码
	SetUserData(ptr_pdata,ID,psd,TYPE_REGISTER);
	WritePacket(clientfd,ptr_pdata);				//发送注册数据包
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond,&mutex);				//等待通知
	MenuGuestUI();
	switch(flag_type)
	{	case TYPE_OK:
			printf("注册成功！\n");
			break;
		case TYPE_ERROR:
			printf("该用户已存在！\n");
			break;
		default:
			break;
	}
	flag_type = 0;
	pthread_mutex_unlock(&mutex);
	free(ptr_pdata);
	return 0;
}
/************************************************
*函数功能：登陆函数
*参数：客户端套接字文件描述符，登陆状态
*返回值：登陆成功或登陆错误返回(EXIT_FLAG)，界面切换，登陆失败返回0，出错-1
************************************************/
int Login(int clientfd, int *state)
{
	if(NULL == state)
		return -1;
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	char ID[MAXSIZE_STR];
	char psd[MAXSIZE_STR];
	GetLoginInfo(ID,psd,MAXSIZE_STR);			//输入账号密码
	SetUserData(ptr_pdata,ID,psd,TYPE_LOGIN);
	WritePacket(clientfd,ptr_pdata);			//发送登陆数据包
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond,&mutex);			//等待通知
	if(TYPE_OK == flag_type)
	{	strcpy(my_ID,ID);		//修改全局变量，用于界面显示
		*state = USER_LOGIN;
	}
	else
	{	MenuGuestUI();
		switch(flag_type)
		{	case TYPE_ERROR:
				printf("该用户不存在！\n");
				break;
			case TYPE_ERROR_2:
				printf("该用户已在其他客户端登陆！\n");
				break;
			case TYPE_ERROR_3:
				printf("密码错误！\n");
				break;
			case TYPE_ERROR_4:
				printf("登陆错误3次！\n");
				*state = USER_LOGERR;
				break;
			default:
				break;
		}
	}
	flag_type = 0;
	pthread_mutex_unlock(&mutex);
	free(ptr_pdata);
	if(USER_LOGIN == *state || USER_LOGERR == *state)
		return EXIT_FLAG;		//登陆成功、登陆错误，界面切换
	return 0;
}
/************************************************
*函数功能：退出登陆
*参数：客户端套接字文件描述符，登陆状态
*返回值：确认退出登陆返回(EXIT_FLAG)，界面切换，选择不退出返回0，出错-1
************************************************/
int Logout(int clientfd, int *state)
{
	char flag;
	EmptyInputBuffer();
	printf("\t是否退出登陆？（输入y退出）");
	flag = getchar();
	if('y' != flag)
		return 0;
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	SetUserData(ptr_pdata,NULL,NULL,TYPE_LOGIN);	//发送空的登陆包
	WritePacket(clientfd,ptr_pdata);
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond,&mutex);				//等待通知
	if(TYPE_OK == flag_type)
		*state = USER_LOGOUT;
	flag_type = 0;
	pthread_mutex_unlock(&mutex);
	free(ptr_pdata);
	return EXIT_FLAG;		//退出登陆，界面切换
}
/************************************************
*函数功能：显示注册界面，输入注册信息
*参数：存放账号、密码的字符串，字符串最大长度
*返回值：void，传参错误不进行操作
************************************************/
void GetRegisterInfo(char *userID, char *passwd, int length)
{
	if(NULL == userID || NULL == passwd || length < 1)
		return ;
	memset(userID,'\0',length);
	memset(passwd,'\0',length);		//清除账号密码内存
	int i;
	char pswd1[length], pswd2[length];
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('=');
	printf("\n===>请输入注册信息……\n===>请输入账号：");
	scanf("%s",userID);
	EmptyInputBuffer();
	while(1)
	{	printf("===>请输入密码：");
		GetPassword(pswd1,length);
		if(!strlen(pswd1))
		{	printf("===>%s密码不允许为空！%s\n",RED,NONE);
			continue;
		}
		printf("===>请再输入一次：");
		GetPassword(pswd2,length);
		if(strcmp(pswd1,pswd2))
		{	printf("===>%s两次输入的密码不一致！%s\n",RED,NONE);
			pswd1[0] = pswd2[0] = '\0';
		}
		else
			break;
	}
	strcpy(passwd,pswd1);
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('=');
	putchar('\n');
}
/************************************************
*函数功能：显示登陆界面，输入登陆信息
*参数：存放账号、密码的字符串，字符串最大长度
*返回值：void，传参错误不进行操作
************************************************/
void GetLoginInfo(char *userID, char *passwd, int length)
{
	if(NULL == userID || NULL == passwd || length < 1)
		return ;
	memset(userID,'\0',length);
	memset(passwd,'\0',length);		//清除账号密码内存
	int i;
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('-');
	printf("\n-->>欢迎登陆……\n-->>账号：");
	scanf("%s",userID);
	EmptyInputBuffer();
	printf("-->>密码：");
	GetPassword(passwd,length);
	for(i = 0; i < MAXSIZE_UI; ++i)
		putchar('-');
	putchar('\n');
}
/************************************************
*函数功能：发送shell命令
*参数：客户端套接字文件描述符
*返回值：成功返回0，出错返回-1
************************************************/
int SendShell(int clientfd)
{
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	char cmd[MAXSIZE_DATA/2];
	int length;
	EmptyInputBuffer();
	printf("输入[exit]退出\n%sServer@shell:%s $",YELLOW,NONE);
	while(1)
	{	cmd[0] = '\0';
		fgets(cmd,MAXSIZE_DATA/2,stdin);
		if((length = strlen(cmd)) <= 1)		//空命令不发送
		{	printf("%sServer@shell:%s $",YELLOW,NONE);
			continue;
		}
		cmd[length - 1] = '\0';
		if(!strcmp(cmd,"exit"))				//退出shell
			break;
		SetCommand(ptr_pdata,cmd);
		WritePacket(clientfd,ptr_pdata);
	}
	MenuUserUI();
	printf("退出shell命令模式……\n");
	free(ptr_pdata);
	return 0;
}
/************************************************
*函数功能：查看在线用户
*参数：客户端套接字文件描述符
*返回值：成功返回0，出错返回-1
************************************************/
int ShowOnline(int clientfd)
{
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	SetOnlineInfo(ptr_pdata,NULL);
	WritePacket(clientfd,ptr_pdata);
	EmptyInputBuffer();
	getchar();				//等待显示
	free(ptr_pdata);
	return 0;
}
/************************************************
*函数功能：用户聊天，聊天用户ID为空则需要输入ID
*参数：客户端套接字文件描述符
*返回值：成功返回0，出错返回-1
************************************************/
int SendChatMessage(int clientfd, char *chatID)
{
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	if(NULL == ptr_pdata)
		return -1;
	char ID_tmp[MAXSIZE_STR];
	memset(ID_tmp,0,MAXSIZE_STR);
	if(NULL == chatID)
	{	printf("对方的ID：");
		scanf("%s",ID_tmp);
		chatID = ID_tmp;
	}
	flag_chat = 1;		//修改全局变量
	ptr_chat_ID = chatID;
	char msg[MAXSIZE_DATA];
	ptr_msg = msg;
	int length;
	EmptyInputBuffer();
	printf("输入[:end]退出\n");
	while(1)
	{	msg[0] = '\0';
		printf("%s[%s]<<<<%s:",CYANS,ID_tmp,NONE);
		GetMessage(msg,MAXSIZE_DATA);		//输入消息
		if(0 == (length = strlen(msg)))
		{	printf("\n");
			continue;
		}
		printf("\r%s[%s]<<<<%s:%s\n",CYANS,ID_tmp,NONE,msg);
		if(!strcmp(msg,":end"))				//退出聊天
			break;
		SetMessage(ptr_pdata,chatID,msg);
		WritePacket(clientfd,ptr_pdata);
	}
	ptr_chat_ID = NULL;
	ptr_msg = NULL;
	flag_chat = 0;		//修改全局变量
	printf("退出聊天……\n");
	free(ptr_pdata);
	return 0;
}
/************************************************
*函数功能：清空输入缓冲区，若输入缓冲区已经为空，阻塞
*参数：无
*返回值：void
************************************************/
void EmptyInputBuffer()
{
	scanf("%*[^\n]");	//将输入缓冲区中所有不是 \n 的字符读入并抛弃
	scanf("%*c");		//读入一个字符并抛弃
}
/************************************************
*函数功能：消息输入，一个字符一个字符输入，显示，回车表示输入结束
*参数：消息存放的字符串
*返回值：void
************************************************/
void GetMessage(char *msg, int length)
{
	if(NULL == msg || length < 1)
		return ;
	char ch;
	int i = 0;
	while(1)
	{	ch = mygetch();		//从终端读取一个字符，但不显示在屏幕上
		if('\n' == ch)		//读取输入的字符，输入回车结束密码输入
			break;
		else if(i > 0 && DEL == ch)	//判断是否输入了“删除”命令
		{	msg[--i] = '\0';	//结束符
			printf("\b \b");	//显示退格
		}
		else if('\n' != ch && '\r' != ch && DEL != ch)
		{	if(i < length-1)	//密码长度限制，保留最后一位放结束符
			{	msg[i++] = ch;
				msg[i] = '\0';	//结束符
				putchar(ch);
			}
		}
	}
}
/************************************************
*函数功能：密码输入，输入密码显示*号
*参数：密码存放的字符串，字符串最大长度
*返回值：void，传参错误不进行操作
************************************************/
void GetPassword(char *password, int length)
{
	if(NULL == password || length < 1)
		return ;
	char ch;
	int i = 0;
	while(1)
	{	ch = mygetch();		//从终端读取一个字符，但不显示在屏幕上
		if('\n' == ch)		//读取输入的字符，输入回车结束密码输入
			break;
		else if(i > 0 && DEL == ch)	//判断是否输入了“删除”命令
		{	i--;
			printf("\b \b");	//显示退格
		}
		else if('\n' != ch && '\r' != ch && DEL != ch)
		{	if(i < length-1)	//密码长度限制，保留最后一位放结束符
			{	ch = CharEncrypt(ch);	//密码加密存储
				password[i++] = ch;
				putchar('*');
			}
		}
	}
	printf("\n");
	password[i] = '\0';			//结束符
}
/************************************************
*函数功能：单个字符加密函数
*参数：字符
*返回值：加密后字符
************************************************/
char CharEncrypt(char ch)
{
	ch += PSD_KEY;
	return ch;
}
/************************************************
*函数功能：单个字符解密函数
*参数：字符
*返回值：解密后字符
************************************************/
char CharDecrypt(char ch)
{
	ch -= PSD_KEY;
	return ch;
}
/************************************************
*函数功能：字符串解密
*参数：要解密的字符串
*返回值：解密后的字符串
************************************************/
char* StringDecrypt(char *str)
{
	if(NULL == str)
		return NULL;
	int i = 0;
	for(; str[i] != '\0'; ++i)
		str[i] = CharDecrypt(str[i]);
	return str;
}
/************************************************
*函数功能：从终端（标准输入：键盘）读取一个字符，不显示在屏幕上
*参数：无
*返回值：读取到的字符，如果输入缓冲区无数据，阻塞
************************************************/
char mygetch()
{
	struct termios oldt, newt;			//定义2个结构体，用于保存终端的2种状态（输入回显和不回显）
	char ch;
	tcgetattr(STDIN_FILENO, &oldt);		//获取当前终端状态（标准输入的接口）
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);	//其中一个（新状态）设置为输入不回显并且读取字符不用等待回车
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);//设置终端为新状态
	ch = getchar();							//读取字符，不需要回车
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);//还原终端状态
	return ch;
}
/************************************************
*函数功能：还原 因为不回显读取时 异常关闭时 的终端状态
*参数：无
*返回值：void
************************************************/
void resetmygetch()
{
	struct termios reset;
	tcgetattr(STDIN_FILENO, &reset);
	reset.c_lflag |= (ICANON | ECHO);	//设置为输入回显，读取字符需要等待回车
	tcsetattr(STDIN_FILENO, TCSANOW, &reset);
}