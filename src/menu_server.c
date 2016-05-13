#include "menu_server.h"

static online *ptr_client_head;		//在线链表
static userlink *ptr_user_head;		//用户链表
static int my_serverfd = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
static FILE *ptr_logfile = NULL;	//日志文件指针
/************************************************
*函数功能：服务器主函数
*参数：void
*返回值：成功返回0，出错返回-1
************************************************/
int main(void)
{
	if(ServerInit())			//服务器初始化
		return -1;
	ServerRun();				//服务器运行
	return 0;
}
/************************************************
*函数功能：服务器初始化，登记信号，读取服务器配置，创建并加载链表，启动服务器
*参数：无
*返回值：成功返回0，出错返回-1
************************************************/
int ServerInit()
{
	if(SIG_ERR == signal(SIGALRM,CheckHeart))	//闹钟信号
	{	perror("signal error");
		return -1;
	}
	alarm(MAXALARM_S);
	if(SIG_ERR == signal(SIGINT,ServerExit))	//服务器退出信号（^C）
	{	perror("signal error");
		exit(-1);
	}
	char IPv4_addr[16];					//IP地址
	int port;							//端口号
	LoadConfig(IPv4_addr,&port);		//读取配置
	if(NULL == (ptr_user_head = CreateUser()))
		return -1;
	LoadUser(ptr_user_head);			//加载用户链表
	ptr_client_head = NULL;				//在线链表置为空
	ptr_logfile = fopen(PATH_LOG,"a+");	//打开日志文件
	if(NULL == ptr_logfile)
	{	perror("fopen error");
		return -1;
	}
	my_serverfd = socket(AF_INET,SOCK_STREAM,0);	//创建套接字
	if(-1 == my_serverfd)
	{	perror("socket error");
		return -1;
	}
	int optval = 1;
	setsockopt(my_serverfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));//套接字选项
	struct sockaddr_in serveraddr;
	memset(&serveraddr,0,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;					//IPv4
	serveraddr.sin_port = htons((short)port);			//端口
	serveraddr.sin_addr.s_addr = inet_addr(IPv4_addr);	//IP地址
	if(bind(my_serverfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)))	//绑定地址端口
	{	perror("bind error");
		return -1;
	}
	system("clear");
	printf("服务器已在端口(%d)上打开，IP地址：%s\n",port,IPv4_addr);
	if(listen(my_serverfd,20))							//监听连接
	{	perror("listen error");
		return -1;
	}
	return 0;
}
/************************************************
*函数功能：服务器运行（主线程），接收客户端连接（阻塞）
*参数：无
*返回值：void
************************************************/
void ServerRun()
{
	pthread_t thid;
	pthread_attr_t pthattr;
	pthread_attr_init(&pthattr);
	pthread_attr_setdetachstate(&pthattr,PTHREAD_CREATE_JOINABLE);//以分离状态启动线程
	int clientfd;
	while(1)
	{	clientfd = accept(my_serverfd,NULL,NULL);	//接收客户端连接
		if(-1 == clientfd)
		{	perror("accept error");
			continue;
		}
		ptr_client_head = InsertClientPrior(ptr_client_head);	//新建客户端节点
		pthread_create(&thid,&pthattr,ServerPthread,ptr_client_head);		//创建线程
		InitClientData(ptr_client_head,clientfd,thid);		//初始化客户端节点
	}
}
/************************************************
*函数功能：服务器子线程
*参数：客户端节点指针
*返回值：NULL
************************************************/
void* ServerPthread(void *arg)
{
	online *ptr_client = (online*)arg;
	while(0 == ptr_client->client.user_state);	//等待客户端节点初始化
	int clientfd = ptr_client->client.clientfd;
	printf("客户端连接--->IP:%s(%d)\n",ptr_client->client.IPv4_addr,\
		ptr_client->client.port);	//显示连接的客户端信息
	ServerFunction(clientfd,ptr_user_head,ptr_client);//读客户端数据
	printf("客户端结束-x->IP:%s(%d)",ptr_client->client.IPv4_addr,\
		ptr_client->client.port);	//显示结束的客户端信息
	printf("%s\n",NONE);//清除颜色显示
	close(clientfd);
	ptr_client_head = DeleteClient(ptr_client_head,ptr_client);	//删除客户端节点
	return NULL;
}
/************************************************
*函数功能：服务器功能，解析数据包，决定要执行的操作（子线程）
*参数：客户端套接字文件描述符，用户链表头指针，在线链表指针
*返回值：void
************************************************/
void ServerFunction(int clientfd, userlink *ptr_user, online* ptr_client)
{
	if(NULL == ptr_user || NULL == ptr_client)
		return ;
	int readsize = 0, login_count = 0, retype, sendfd;
	int exit_flag = NORMAL_FLAG;		//是否异常退出
	phead head;
	char buff1[MAXSIZE_DATA], buff2[MAXSIZE_DATA];
	packet *ptr_pdata = (packet*)calloc(1,sizeof(packet));
	while(1)
	{	ptr_pdata->buff[0] = '\0';
		readsize = ReadPacket(clientfd,&head,buff1,buff2);
		pthread_mutex_lock(&mutex);
		if(0 == readsize)
		{	if(EXIT_FLAG != exit_flag)
				printf("%s",RED);	//异常退出显示红色
			pthread_mutex_unlock(&mutex);
			break;
		}
		if(TYPE_HEART != head.data_type)
			WriteLog(head.data_type,buff1,buff2,ptr_client);
		switch(head.data_type)
		{	case TYPE_HEART:					//心跳数据包
				HeartData(ptr_client);
				fprintf(stderr,"IP:%s(%d)%s --Heart--\n",\
					ptr_client->client.IPv4_addr,\
					ptr_client->client.port,buff1);
				break;
			case TYPE_REGISTER:					//注册数据包
				retype = UserRegister(ptr_user,buff1,buff2);
				SetNone(ptr_pdata,retype);
				WritePacket(clientfd,ptr_pdata);
				break;
			case TYPE_LOGIN:					//登陆数据包
				retype = UserLogin(ptr_user,buff1,buff2,ptr_client);
				if(TYPE_OK == retype)
					login_count = 0;
				else
					if(++login_count >=3)//登陆错误3次
						retype = TYPE_ERROR_4;
				SetNone(ptr_pdata,retype);
				WritePacket(clientfd,ptr_pdata);
				break;
			case TYPE_MESSAGE:					//消息数据包
				sendfd = HandleMessage(ptr_user,buff1,buff2,ptr_client);
				SetMessage(ptr_pdata,buff1,buff2);
				WritePacket(sendfd,ptr_pdata);
				break;
			case TYPE_COMMAND:					//shell命令数据包
				ExecuteShell(buff1,buff2,MAXSIZE_DATA,ptr_pdata,clientfd);
				break;
			case TYPE_ONLINE:					//查看在线用户
				ShowAllClient(ptr_client);
				ShowAllUser(ptr_user);
				SearchOnline(buff1,MAXSIZE_DATA,ptr_client);
				SetOnlineInfo(ptr_pdata,buff1);
				WritePacket(clientfd,ptr_pdata);
				break;
			case TYPE_EXIT:						//客户端正常退出
				SetNone(ptr_pdata,TYPE_EXIT);	//返回客户端退出数据包
				WritePacket(clientfd,ptr_pdata);
				exit_flag = EXIT_FLAG;	//正常退出
				break;
			default:							//其他类型数据包
				return ;
		}
		pthread_mutex_unlock(&mutex);
	}
	free(ptr_pdata);
}
/************************************************
*函数功能：心跳检查，处理异常客户端，信号处理函数
*参数：信号值
*返回值：void
************************************************/
void CheckHeart(int signum)
{
	if(SIGALRM == signum)
	{	alarm(MAXALARM_S);
		if(NULL == ptr_client_head)
			return ;
		HeartAdd(ptr_client_head);		//增加心跳值
		online *ptr_find = ptr_client_head;
		online *ptr_prior = ptr_find->prior;
		while(1)
		{	if(ERROR_FLAG == ptr_find->client.pth_id)//删除异常线程 
			{	close(ptr_find->client.clientfd);
				ptr_client_head = DeleteClient(ptr_client_head,ptr_find);
			}
			if(ptr_find == ptr_prior || NULL == ptr_client_head)
				break;
			ptr_find = ptr_find->next;
		}
	}
}
/************************************************
*函数功能：服务器退出，信号处理函数（^C）
*参数：信号值
*返回值：void
************************************************/
void ServerExit(int signum)
{
	if(SIGINT == signum)
	{	close(my_serverfd);
		fclose(ptr_logfile);
		pthread_mutex_unlock(&mutex);
		pthread_mutex_destroy(&mutex);
		pthread_mutex_destroy(&mutex_log);
		DestroyUser(ptr_user_head);
		DestroyOnline(ptr_client_head);
		printf("\n服务器关闭……\n");
		exit(1);
	}
}
/************************************************
*函数功能：加载服务器配置文件
*参数：存放IP的字符串、存放端口号的整型变量
*返回值：void
*#格式【IP：[点分十进制][空格],PORT：[端口号]】
*#端口号限制：5001～65535
************************************************/
void LoadConfig(char *IPv4_addr, int *port)
{
	if(NULL == IPv4_addr || NULL == port)
		exit(ERROR_FLAG);
	FILE *ptr_file = fopen(PATH_CONFIG,"r");		//打开文件，只读
	if(NULL != ptr_file)
	{	int flag = fscanf(ptr_file,"IP：%s ,PORT：%d",IPv4_addr,port);	//读取数据
		fclose(ptr_file);							//关闭文件
		ptr_file = NULL;
		if(2 == flag)								//读到2个数据
			if(-1 != inet_addr(IPv4_addr))			//地址检查
				if(5000 < *port && 65536 > *port)	//端口号限制
					return ;	//加载成功，退出函数
	}
	else
		fprintf(stderr,"找不到配置文件！\n");
	fprintf(stderr,"读取配置文件出错！请检查配置文件……\n");
	fprintf(stderr,"（输入任意值退出……）\n");
	getchar();
	exit(ERROR_FLAG);			//配置异常，退出进程
}
/************************************************
*函数功能：加载用户文件，创建用户链表（尾插法）
*参数：用户链表头指针（一个空的头节点，也是尾节点）
*返回值：void
************************************************/
void LoadUser(userlink *ptr_head)
{
	if(NULL == ptr_head)
	{	fprintf(stderr,"系统故障，用户链表为空！\n");
		exit(ERROR_FLAG);
	}
	int fd = open(PATH_USER,O_RDONLY|O_CREAT,0666);	//只读，创建
	if(-1 == fd)
	{	perror("open error");
		exit(ERROR_FLAG);
	}
	userlink *ptr_tmp = ptr_head;
	int flag;
	int backup_stdin = dup(STDIN_FILENO);	//备份标准输入
	dup2(fd,STDIN_FILENO);					//标准输入重定向到文件
	while(1)
	{	flag = scanf("%s ,%s ,%ld\n", ptr_tmp->user.userID,\
			ptr_tmp->user.passwd,&ptr_tmp->user.creation_time);
		if(-1 == flag)
			break;
		ptr_tmp = InsertUserEnd(ptr_head);	//新建结点，存放下一个数据
		if(NULL == ptr_tmp)
		{	fprintf(stderr,"系统故障，加载链表失败……\n");
			exit(ERROR_FLAG);
		}
	}
	dup2(backup_stdin,STDIN_FILENO);		//恢复标准输入
	if(NULL != ptr_tmp->prior)	//读取到了至少一个数据
	{	ptr_tmp->prior->next = NULL;
		free(ptr_tmp);			//读取不到新数据时，将最后新建的节点释放
	}
	close(fd);
}
/************************************************
*函数功能：保存用户文件
*参数：用户链表头指针
*返回值：void
************************************************/
void SaveUser(userlink *ptr_head)
{
	if(NULL == ptr_head)
	{	fprintf(stderr,"系统故障，用户链表为空！\n");
		exit(ERROR_FLAG);
	}
	FILE *ptr_file = fopen(PATH_USER,"w");		//只写，清空
	if(NULL == ptr_file)
	{	perror("fopen error");
		exit(ERROR_FLAG);
	}
	userlink *ptr_tmp = ptr_head;
	if(0 != ptr_head->user.creation_time)		//头节点不是空节点
	{	while(NULL != ptr_tmp)
		{	fprintf(ptr_file,"%s ,%s ,%ld\n",ptr_tmp->user.userID,\
				ptr_tmp->user.passwd,ptr_tmp->user.creation_time);
			ptr_tmp = ptr_tmp->next;
		}
	}
	fclose(ptr_file);
}
/************************************************
*函数功能：写入日志文件
*参数：数据包类型，数据字符串，客户端节点
*返回值：void
************************************************/
void WriteLog(int type, char *str1, char *str2, online* ptr_client)
{
	if(NULL == ptr_logfile || NULL == str1 || NULL == str2 || NULL == ptr_client)
		return ;
	pthread_mutex_lock(&mutex_log);
	time_t now = time(NULL);
	struct tm *ptm = localtime(&now);		//时间
	fprintf(ptr_logfile,"%4d-%02d-%02d %02d:%02d:%02d",\
		ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,\
		ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
	fprintf(ptr_logfile," --->IP:%15s(%5d)",ptr_client->client.IPv4_addr,ptr_client->client.port);//IP和端口
	switch(type)
	{	case TYPE_REGISTER:					//注册数据包
			fprintf(ptr_logfile,"用户注册<ID:%s>\n",str1);
			break;
		case TYPE_LOGIN:					//登陆数据包
			if(strlen(str1))
				fprintf(ptr_logfile,"用户登陆<ID:%s>\n",str1);
			else
				fprintf(ptr_logfile,"用户退出登陆<ID:%s>\n",\
					ptr_client->client.current_user->user.userID);
			break;
		case TYPE_MESSAGE:					//消息数据包
			fprintf(ptr_logfile,"消息发送[%s]>>>[%s]%s\n",\
				ptr_client->client.current_user->user.userID,\
				str1,str2);
			break;
		case TYPE_COMMAND:					//shell命令数据包
			fprintf(ptr_logfile,"shell命令[%s]\n",str1);
			break;
		case TYPE_ONLINE:					//在线用户
			fprintf(ptr_logfile,"查看在线用户\n");
			break;
		case TYPE_EXIT:						//客户端正常退出
			fprintf(ptr_logfile,"客户端退出……\n");
			break;
		default:							//其他类型数据包
			fprintf(ptr_logfile, "\n");
	}
	fflush(ptr_logfile);
	pthread_mutex_unlock(&mutex_log);
}
/************************************************
*函数功能：用户注册
*参数：用户链表头指针，账号密码字符串
*返回值：数据包类型，成功返回正确信息，失败返回错误信息，出错返回-1
************************************************/
int UserRegister(userlink *ptr_user, char *ID, char *psd)
{
	if(NULL == ptr_user || NULL == ID || NULL == psd)
		return -1;
	if(NULL != FindUser(ptr_user,ID))			//该用户已存在
		return TYPE_ERROR;
	userlink *ptr_new = ptr_user;
	if(0 != ptr_user->user.creation_time)		//头节点有数据
		ptr_new = InsertUserEnd(ptr_user);		//新建节点插入链表
	strcpy(ptr_new->user.userID,ID);
	strcpy(ptr_new->user.passwd,psd);
	ptr_new->user.creation_time = time(NULL);	//当前时间作为注册时间
	SaveUser(ptr_user);
	return TYPE_OK;
}
/************************************************
*函数功能：用户登陆，如果登陆数据包为空包，表示退出登陆
*参数：用户链表头指针，账号密码字符串，在线链表头指针
*返回值：数据包类型，成功返回正确信息，失败返回错误信息，出错返回-1
************************************************/
int UserLogin(userlink *ptr_user, char *ID, char *psd, online* ptr_client)
{
	if(NULL == ptr_user || NULL == ID || NULL == psd || NULL == ptr_client)
		return -1;
	if(!(strlen(ID) || strlen(psd)))	//如果是空数据包，表示退出登陆
		ClientLogout(ptr_client);
	else
	{	userlink *ptr_tmp = FindUser(ptr_user,ID);
		if(NULL == ptr_tmp)							//该用户不存在
			return TYPE_ERROR;
		if(NULL != FindClientByID(ptr_client,ID))	//该用户已登陆
			return TYPE_ERROR_2;
		if(strcmp(ptr_tmp->user.passwd,psd))		//密码错误
			return TYPE_ERROR_3;
		ClientLogin(ptr_client, ptr_tmp);
	}
	return TYPE_OK;
}
/************************************************
*函数功能：心跳数据包正常接收，清零心跳值
*参数：客户端指针
*返回值：成功返回0，出错返回-1
************************************************/
int HeartData(online* ptr_client)
{
	if(NULL == ptr_client)
		return -1;
	ptr_client->client.heart = 0;
	return 0;
}
/************************************************
*函数功能：处理消息数据包
*参数：用户链表头指针，消息要发送的ID，消息字符串，客户端指针
*返回值：消息要发送的客户端套接字文件描述符，出错返回-1
************************************************/
int HandleMessage(userlink *ptr_user, char *ID, char *msg, online* ptr_client)
{
	if(NULL == ptr_user || NULL == ID || NULL == msg || NULL == ptr_client)
		return -1;
	char *myID = ptr_client->client.current_user->user.userID;
	if(!strcmp(ID,myID))
	{	sprintf(msg,"不要给自己发送消息……");
		return ptr_client->client.clientfd;
	}
	userlink *ptr_tmp_user = FindUser(ptr_user,ID);
	if(NULL != ptr_tmp_user)					//该用户存在
	{	online *ptr_tmp_client = FindClientByID(ptr_client,ID);
		if(NULL != ptr_tmp_client)				//该用户已登录
		{	strcpy(ID,myID);
			return ptr_tmp_client->client.clientfd;
		}
		else
			sprintf(msg,"消息发送失败……（用户[%s]未登录）",ID);
	}
	else
		sprintf(msg,"消息发送失败……（用户[%s]不存在）",ID);
	return ptr_client->client.clientfd;
}
/************************************************
*函数功能：shell命令执行并返回客户端
*参数：命令字符串，存放执行结果的字符串，字符串最大长度
*返回值：成功返回0，出错返回-1
************************************************/
int ExecuteShell(char *cmd, char *result, int length, packet *ptr_pdata, int clientfd)
{
	if(NULL == cmd || NULL == result)
		return -1;
	pthread_t thid = pthread_self();
	char path[MAXSIZE_PATH];
	sprintf(path,"%s%ld.tmp",PATH_TMP,thid);
	int fd = open(path,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(-1 == fd)
		return -1;
	int backup_stdout = dup(STDOUT_FILENO);	//备份标准输出
	dup2(fd,STDOUT_FILENO);					//标准输出重定向到文件
	if(system(cmd))
	{	printf("[%s]执行错误……请输入正确的命令……",cmd);
		fflush(stdout);
	}
	dup2(backup_stdout,STDOUT_FILENO);		//恢复标准输出
	lseek(fd,0,SEEK_SET);
	int result_len = length - strlen(cmd) - 1;
	int readsize = read(fd,result,result_len);		//数据包可能满
	result[readsize] = '\0';
	while(1)
	{	SetResult(ptr_pdata,cmd,result);
		WritePacket(clientfd,ptr_pdata);
		readsize = read(fd,result,length-1);		//分包发送
		if(0 == readsize)
			break;
		result[readsize] = '\0';
		cmd = NULL;
	}
	close(fd);
	remove(path);
	return 0;
}
/************************************************
*函数功能：查看在线用户
*参数：在线用户字符串，字符串长度，客户端节点
*返回值：成功返回0，出错返回-1
************************************************/
int SearchOnline(char *online_user, int length, online* ptr_client)
{
	if(NULL == online_user || NULL == ptr_client)
		return -1;
	online *ptr_find = ptr_client;
	userlink *ptr_user;
	online_user[0] = '\0';
	while(1)
	{	ptr_user = ptr_find->client.current_user;
		if(NULL != ptr_user)					//未登录用户不查找
			sprintf(online_user,"%s\tID:%10s\n",online_user,ptr_user->user.userID);
		if(ptr_client == ptr_find->next)
			break;
		ptr_find = ptr_find->next;
	}
	return 0;
}