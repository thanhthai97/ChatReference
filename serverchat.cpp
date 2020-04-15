#include<iostream>
//#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/signalfd.h>
#include<inttypes.h>
#include<fcntl.h>
#include<stdarg.h>
#include<memory.h>
//#include<sys/socketvar.h>
//#include<fcntl.h>
#include<vector>

#define SERV_PORT 8000
#define MAX_BUFFER 1024
#define MAX_CLIENT 65525
#define SIZE_MESSAGE 16

#define GROUP_CHAT '0'
#define PRIVATE_CHAT '1'
#define SEND_FILE '2'
#define END_SEND_FILE '3'
#define LOG_IN '4'
#define LOG_OUT '5'
#define LOG_IN_FAILURE '6'
#define LOG_IN_SUCCESS '7'
#define ADD_USER '8'
#define ADD_ERROR '9'
#define ADD_SUCCESS 'a'
#define SEND_CONTENT_FILE 'b'
#define SOCKET_FILE 'c'
#define NEW_LOG_IN 'e'
#define NEW_LOG_OUT 'f'


int n_data, client3, client4;
char clientname[SIZE_MESSAGE];
char buff[MAX_BUFFER], buffer[MAX_BUFFER], str[INET_ADDRSTRLEN];
int connfd, sockfd; // varibale for file disriptor
struct sockaddr_in servaddr;
int listenfd;


int setup_signalfd() {
	int sfd, ret;
	sigset_t sigset;

	ret = sigprocmask(SIG_SETMASK, NULL, &sigset);
	if (ret < 0)
		perror("sigprocmask.1");

	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	ret = sigprocmask(SIG_SETMASK, &sigset, NULL);
	if (ret < 0)
		perror("sigprocmask.2");

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);

	sfd = signalfd(-1, &sigset, 0);
	if (sfd < 0)
		perror("signalfd");

	//printf("sfd is %i\n", sfd);

	return sfd;
}

void read_sig(int sfd) {
	struct signalfd_siginfo info;
	int ret;

	ret = read(sfd, &info, sizeof info);
	if (ret != sizeof info)
		perror("read sfd");

	printf("Recibida signal\n");
	printf("signo = %" PRIu32 "\n", info.ssi_signo);
	printf("pid   = %" PRIu32 "\n", info.ssi_pid);
	printf("uid   = %" PRIu32 "\n", info.ssi_uid);
    printf("Exiting !!!\n");
    exit(0);
}


void init_socket_server()
{
    listenfd = socket(AF_INET,SOCK_STREAM, 0); // create socket with stype sock stream
    memset(&servaddr,0,sizeof(servaddr));

    servaddr.sin_family=AF_INET; // socket stype AF_NET
    servaddr.sin_addr.s_addr = INADDR_ANY; //address for socket server apply any address of client
    servaddr.sin_port = htons(SERV_PORT); // port of socket server is 8000
    // bind socket server to server address
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    int status = fcntl(listenfd,F_SETFL,O_NONBLOCK);
    if(status == -1) perror("control fcnl");

    listen(listenfd,20); // always listen
}
FILE *file_user;

char username[100];char password[100];
bool find_user(char* uname,const char* file_name)
{
    char *line_buf = NULL;
    bool retread = false;
    size_t line_buf_size = 0;
    int line_count = 0;
    ssize_t line_size;
    
    // Check username in file list user
    file_user = fopen(file_name,"r");
    /* Đọc dòng đầu tiên trong file */
    line_size = getline(&line_buf, &line_buf_size, file_user);
    
    /* Lặp lại việc đọc và hiển thị nội dung cho tới khi hoàn tất */
    while (line_size >= 0)
    {
        /* Tăng số lượng dòng 0 đơn vị */
        line_count++;
        
        if((std::string) line_buf == (std::string) uname+'\n')
        {
            //printf("Find...\n");
            strcpy(username,line_buf );
            line_size = getline(&line_buf, &line_buf_size, file_user);
            strcpy(password,line_buf);
            retread = true;
            break;
        }
        
        /* Đọc dòng tiếp theo */
        line_size = getline(&line_buf, &line_buf_size, file_user);
        
    }
    free(line_buf);
    fclose(file_user);
    //printf("* %s\n",username);
    //printf("# %s\n",password);
    return retread;
}

struct listuser{
    int socket_no;
    char user[100];
};


struct listuser listuser[65525];
int all_user;
int position;
void erase_user(int socket_fd)
{
    
    position = 0;
    while(listuser[position].socket_no != socket_fd)
    {
        position++;
    }
    for(int i=position;i<all_user;i++)
    {
        listuser[i].socket_no = listuser[i+1].socket_no;
        strcpy(listuser[i].user, listuser[i+1].user);
    }
}

int main()
{
    std::cout<<"PID process server running: "<<(int)getpid()<<std::endl;
    
    ssize_t nready, efd, retepoll, sfd;
    
    socklen_t clilen; // lenght of socket client
    //int client[MAX_CLIENT];
    struct sockaddr_in cliaddr; // struct for socket server and client

    struct epoll_event tep, ep[MAX_CLIENT]; //struct list event of epoll
    // Initial SOCKET SERVER--------------------------------------------------------------------
    init_socket_server();
    //----INITIAL EPOLL------------------------------------------------------------------------
    efd = epoll_create(MAX_CLIENT); //create epoll with 65525 element
    sfd = setup_signalfd();

    tep.events = EPOLLIN; tep.data.fd = listenfd; // create epoll event and add event of socket server
    retepoll = epoll_ctl(efd,EPOLL_CTL_ADD,listenfd,&tep); // add event listen of socket server to epoll list
    tep.data.fd = sfd; // add event from signal
    tep.events = EPOLLIN;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &tep) != 0)
    {
	    perror("epoll_ctl");
    }
    //---------------------------------------------------------------------------------------------
    // create file to write data from clients
    //create_file();
    //----------------------------------------------
    all_user = 0;
    listuser[all_user].socket_no = sfd;
    strcpy(listuser[all_user].user,"signal");
    
    bool status_find;
    printf("server start!!!\n");
    char packets[1500];
    char reply[10];
    char u_username[100];
    char u_password[100];
    char user_recive[100];
    char user_recive_file[100];
    char user_file[100];
    char user_out[100];
    
    //memset(packets,'\0',sizeof(packets));
    
    while(true)
    {
        printf("\tTotal user: %d\n",all_user);
        // epoll wait
        nready = epoll_wait(efd,ep,65525,-1);
        //check event
        
        for(int i =0;i<nready;i++)
        {
            // if there is a event happen if not exist on list
            if(ep[i].data.fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                // server accept the connect from client to server socket
                connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
                
                //printf("received from %s at PORT %d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,str,INET_ADDRSTRLEN),ntohs(cliaddr.sin_port));
                               
                tep.events = EPOLLIN; tep.data.fd = connfd; 
                retepoll = epoll_ctl(efd, EPOLL_CTL_ADD, connfd,&tep);
                
            }
	        else if(ep[i].data.fd == sfd)
            {
                printf("Close all socket\n");
                for(int s=1;s<=all_user;s++)
                {
                    close(listuser[s].socket_no);
                }
                read_sig(sfd);
            }
            else
            {
                sockfd = ep[i].data.fd;
                                
                //n = read(sockfd,buf,MAXLINE);
                ioctl(sockfd,FIONREAD,&n_data);
                // if don't have the data read from client server will erase the connect
                if(n_data==0)
                {
                    retepoll = epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
                    close(sockfd);
                    // earase socket from listuser
                    //bzero(packets,sizeof(packets));
                    for(int s=1;s<=all_user;s++)
                    {
                        if(listuser[s].socket_no == sockfd){
                            //packets[0]=A_USER_LOG_OUT;
                            //strcat(packets,listuser[s].user);
                            erase_user(sockfd);
                            all_user--;
                        }
                    }
                    
                    /*for(int s=1;s<=all_user;s++)
                    {
                        write(listuser[s].socket_no,packets,sizeof(packets));
                    }*/
                    
                    printf("client [%d] closed connection\n",sockfd);
                }
                else
                {
                // check fd of the socket to determine the client
                    //memset(packets,'\0',sizeof(packets));
                    bzero(packets,sizeof(packets));
                    int read_packets = read(sockfd,packets,sizeof(packets));
                    if(read_packets<0){
                        perror("Server read error");
                        exit(1);
                    }
                    //char *message = packets;
                    printf("%s",packets);
                    // block do working
                    
                    if(packets[0]==GROUP_CHAT)
                        for(int k=1;k<=all_user;k++)
                        {
                            if(listuser[k].socket_no != sockfd)
                            {
                                printf("Send for other\n");
                                write(listuser[k].socket_no,packets,sizeof(packets));
                            }
                        }
                    else if(packets[0]==PRIVATE_CHAT)
                        {
                            int len = strlen(packets);
                            //memset(user_recive,'\0',sizeof(user_recive));
                            bzero(user_recive,sizeof(user_recive));
                            for(int n=0;n<len && packets[n+1]!='\t';n++)
                            {
                                user_recive[n] = packets[n+1];
                            }
                            
                            for(int n=0;n<=all_user;n++)
                            {
                                if(strcmp(listuser[n].user,user_recive)==0)
                                {
                                    write(listuser[n].socket_no,packets,sizeof(packets));
                                }
                            }

                        }
                    
                    else if(packets[0]==SEND_CONTENT_FILE)
                    {
                        int lent=strlen(packets);
                        bzero(user_recive_file,sizeof(user_recive_file));
                        for(int n=0;n<lent && packets[n+1]!='\t';n++)
                        {
                            user_recive_file[n] = packets[n+1];
                        }
                        for(int l=1;l<=all_user;l++)
                        {
                            if(strcmp(listuser[l].user,user_recive_file)==0)
                            {
                                write(listuser[l].socket_no,packets,sizeof(packets));
                            }
                        }
                    }
                    else if(packets[0] == SEND_FILE)
                        {
                            int len = strlen(packets);
                            bzero(user_recive_file,sizeof(user_recive_file));
                            for(int n=0;n<len && packets[n+1]!='\t';n++)
                            {
                                user_recive_file[n] = packets[n+1];
                            }
                            for(int n=0;n<=all_user;n++)
                            {
                                if(strcmp(listuser[n].user,user_recive_file)==0)
                                {
                                    write(listuser[n].socket_no,packets,sizeof(packets));
                                }
                            }
                        }
                    else if(packets[0] == END_SEND_FILE)
                    {
                        int len = strlen(packets);
                            bzero(user_recive_file,sizeof(user_recive_file));
                            for(int n=0;n<len && packets[n+1]!='\t';n++)
                            {
                                user_recive_file[n] = packets[n+1];
                            }
                            
                            for(int n=0;n<=all_user;n++)
                            {
                                if(strcmp(listuser[n].user,user_recive_file)==0)
                                {
                                    write(listuser[n].socket_no,packets,sizeof(packets));
                                }
                            }

                    }
                    else if(packets[0]==LOG_IN)
                        {
                            int len = strlen(packets);
                            char* key = strstr(packets,"\t");
                            key++;
                            // tach duoc password
                            //memset(u_password,'\0',sizeof(u_password));
                            bzero(u_password,sizeof(u_password));
                            strcpy(u_password,key);
                            //printf("%s\n",u_password);
                            // tach username
                            //memset(u_username,'\0',sizeof(u_username));
                            bzero(u_username,sizeof(u_username));
                            for(int n=0;n<len && packets[n+1] != '\t';n++)
                            {
                                u_username[n] = packets[n+1];
                            }
                            //printf("%s\n",u_username);
                            status_find = find_user(u_username,"file_user.txt");
                            
                            if(status_find)
                            {
                                if((std::string)u_username+'\n' == (std::string)username && 
                                (std::string)password == (std::string)u_password+'\n')
                                {
                                    reply[0] = LOG_IN_SUCCESS;
                                    write(sockfd,reply,sizeof(reply));
                                    /*for(int s=1;s<=all_user;s++)
                                    {
                                        write(listuser[s].socket_no,packets,sizeof(packets));
                                    }*/
                                    all_user++;
                                    listuser[all_user].socket_no = sockfd;
                                    strcpy(listuser[all_user].user,u_username);
                                    //printf("we have: socket %d, user %s\n",listuser[all_user].socket_no,listuser[all_user].user);
                                    bzero(packets,sizeof(packets));
                                    packets[0]=NEW_LOG_IN;
                                    strcat(packets,u_username);
                                    for(int k=1;k<=all_user;k++)
                                    {
                                        if(listuser[k].socket_no != sockfd)
                                        {
                                            printf("Note new\n");
                                            write(listuser[k].socket_no,packets,sizeof(packets));
                                        }
                                    }
                                }
                                else
                                {
                                    reply[0] = LOG_IN_FAILURE;
                                    write(sockfd,reply,sizeof(reply));
                                    
                                }
                                
                            }
                            else if(!status_find)
                            {
                                reply[0] = LOG_IN_FAILURE;
                                write(sockfd,reply,sizeof(reply));
                                
                            }
                            
                        }
                    else if(packets[0]==LOG_OUT)
                        {
                            
                            bzero(user_out,sizeof(user_out));
                            for(int k=1;k<=all_user;k++)
                            {
                                if(listuser[k].socket_no==sockfd)
                                {
                                    strcpy(user_out,listuser[k].user);
                                }
                            }
                            bzero(packets,sizeof(packets));
                            packets[0]=NEW_LOG_OUT;
                            strcat(packets,user_out);
                            printf("Note new out\n");
                            for(int k=1;k<=all_user;k++)
                            {
                                if(listuser[k].socket_no != sockfd)
                                {
                                    
                                    write(listuser[k].socket_no,packets,sizeof(packets));
                                }
                            }
                            erase_user(sockfd);
                            all_user--;
                            /*for(int s=1;s<=all_user;s++)
                            {
                                write(listuser[s].socket_no,packets,sizeof(packets));
                            }*/
                            close(sockfd);
                        }
                    else if(packets[0]==ADD_USER)
                        {
                            int len = strlen(packets);
                            char* key = strstr(packets,"\t");
                            key++;
                            // tach duoc password
                            //memset(u_password,'\0',sizeof(u_password));
                            bzero(u_password,sizeof(u_password));
                            strcpy(u_password,key);
                            // tach username
                            //memset(u_username,'\0',sizeof(u_username));
                            bzero(u_username,sizeof(u_username));
                            for(int n=0;n<len && packets[n+1] != '\t';n++)
                            {
                                u_username[n] = packets[n+1];
                            }
                            status_find = find_user(u_username,"file_user.txt");
                            if(status_find)
                            {
                                reply[0] = ADD_ERROR;
                                write(sockfd,reply,sizeof(reply));
                            }
                            else
                            {
                                file_user = fopen("file_user.txt","a");
                                fprintf(file_user,"%s\n",u_username);
                                fprintf(file_user,"%s\n",u_password);
                                fclose(file_user);
                                reply[0] = ADD_SUCCESS;
                                write(sockfd,reply,sizeof(reply));
                            }
                        }
                        
                     // end if why error with switch case
                    
                    
                } // end else ndata != 0
                
            } // end else epoll event was exist
            
        } //end for
    } // end while
    return 0;
}