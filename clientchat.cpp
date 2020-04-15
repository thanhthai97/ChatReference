#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<string.h>
#include<memory.h>
#include<vector>
#include<string>

#define MAX_BUFF 1024
#define PORT 8000
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
#define NEW_LOG_IN 'e'
#define NEW_LOG_OUT 'f'
bool alive = true;
int sockfd;
char packets[1500];
char packets_file[1500];
char message[1024];
char buff[1024];
char private_mess[1024];
char file_name[500];
char username[100];
char new_user[100];
char user_recive[100];
char user_recive_file[100];
char password[100];
char reply[10];
pthread_mutex_t lock_packets;
pthread_t manager_thread_send_file;
pthread_t manager_thread_recive_file;
void* retval_thread;
void *thread_check_socket(void *arg)
{
    
    while(recv(sockfd,NULL,1,MSG_PEEK | MSG_DONTWAIT) != 0)
    {
        sleep(1);
    }
    
    alive = false;
    printf("\nServer not found\n");
    close(sockfd);
    exit(0);
    
}

struct listuser{
    char user[100];
};


struct listuser listuser[65525];
int all_user=0;
int position;
void erase_user(char *user_name)
{
    
    position = 1;
    while(strcmp(listuser[position].user, user_name)==0)
    {
        position++;
    }
    for(int i=position;i<all_user;i++)
    {
        strcpy(listuser[i].user, listuser[i+1].user);
    }
}
void *thread_recive_packets(void *arg)
{
    
    while(alive){
        pthread_mutex_lock(&lock_packets);
        int h = read(sockfd,packets,sizeof(packets));
        if(h<0) {perror("Read error"); exit(0);}
        //printf("%s",packets);
        
        if(packets[0]==NEW_LOG_IN)
            {
                bzero(new_user,sizeof(new_user));
                for(int c=0;c<strlen(packets);c++)
                {
                    new_user[c]=packets[c+1];
                }
                printf("\t%s log in\n",new_user);
                all_user++;
                strcpy(listuser[all_user].user,new_user);
            }
        else if(packets[0]==NEW_LOG_OUT)
            {
                bzero(new_user,sizeof(new_user));
                for(int c=0;c<strlen(packets);c++)
                {
                    new_user[c]=packets[c+1];
                }
                printf("\t%s log out\n",new_user);
                erase_user(new_user);
                all_user--;
            }
        else if(packets[0]==GROUP_CHAT)
        {
            char *newmess = packets;
            newmess++;
            printf("\t"); printf("%s",newmess);
        }
        else if(packets[0]==PRIVATE_CHAT)
        {
            char *newkey=strstr(packets,"\t");
            newkey++;
            printf("\t"); printf("%s",newkey);
        }
        else if(packets[0]==SEND_FILE)
            {
                bzero(file_name,sizeof(file_name));
                char *keys = strstr(packets,"\t");
                keys++;
                strcpy(file_name,keys);
                printf("%s",file_name);
                
                FILE *file;
                char buff[1024];
                //bzero(buff,sizeof(buff));
                file = fopen(file_name,"w");
                if(file == NULL)
                {
                    printf("Copy file Error !!!");
                    exit(1);
                }
                fclose(file);
                
                char *run;
                //packets_file[0] = SEND_CONTENT_FILE;
                while(packets_file[0]!=END_SEND_FILE)
                {
                    
                    read(sockfd,packets_file,sizeof(packets_file));
                    printf("%s",packets_file);
                    if(packets_file[0]==END_SEND_FILE)break;
                    bzero(buff,sizeof(buff));
                    run = strstr(packets_file,"\t"); run++;
                    strcpy(buff,run); //printf("$ %s",buff);
                    file = fopen(file_name,"a");
                    fprintf(file,"%s",buff);
                    fclose(file);
                }
                
                printf("\nRecive file complete\n");
            }
        
        
        pthread_mutex_unlock(&lock_packets);
    }
    
}


int main()
{
    
    int len;
    struct sockaddr_in address;
    int result;
    pthread_t manager_thread_check;
    pthread_t manager_thread_recive_packets;
    //pthread_t manager_thread_send_packets;
    int thread_check;
    void *thread_retval;
    
    // create socket for client
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    address.sin_family=AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // address of client is 192.168.1.10
    address.sin_port = htons(PORT); // port connect to server is 8000

    len = sizeof(address);
    // request the connect to server
    
    result = connect(sockfd,(struct sockaddr *)&address,len);
    if(result == -1)
    {
        perror("oops: client problem with connection");
        exit(EXIT_FAILURE);
    }
    // Log in user or add user
    char request[5];
    reply[0] = LOG_IN;
    while(reply[0] != LOG_IN_SUCCESS)
    {
        printf("What do you want to do [log/add]:  "); scanf("%s",request);
        //memset(packets,'\0',sizeof(packets));
        bzero(packets,sizeof(packets));
        if(strcmp(request,"log") == 0)
        {
            printf("Login\n");
            printf("Username: "); scanf("%s",username);
            //fgets(username,sizeof(username),stdin);
            printf("Password: "); scanf("%s",password);
            //fgets(password,sizeof(password),stdin);
            
            packets[0] = LOG_IN;
            strcat(packets,username);
            strcat(packets,"\t");
            strcat(packets,password);
            
            write(sockfd,packets,sizeof(packets));
            read(sockfd,reply,sizeof(reply));
            if(reply[0]==LOG_IN_FAILURE) printf("!!! Login failure\n");
            else if(reply[0]== LOG_IN_SUCCESS) printf("=> Login success\n");
        }
        if(strcmp(request,"add")==0)
        {
            printf("Choose username: "); //fgets(username,sizeof(username),stdin);
            scanf("%s",username);
            printf("Type password: "); //fgets(password,sizeof(password),stdin);
            scanf("%s",password);
            
            packets[0] = ADD_USER;
            strcat(packets,username);
            strcat(packets,"\t");
            strcat(packets,password);
            write(sockfd,packets,sizeof(packets));
            read(sockfd,reply,sizeof(reply));
            if(reply[0] == ADD_ERROR) printf("!!! Registration user failure\n");
            else if(reply[0]==ADD_SUCCESS) {printf(">> Registration user success\n"); strcpy(request,"log");}
        }
    }
    strcpy(listuser[all_user].user,username);
    //strcpy(listuser[0], username);
    thread_check=pthread_mutex_init(&lock_packets,NULL);
    if(thread_check!=0)
    {
        perror("Thread mutex created error");
        exit(EXIT_FAILURE);
    }
    thread_check=pthread_create(&manager_thread_check,NULL,thread_check_socket,NULL);
    if(thread_check!=0)
    {
        perror("Thread read created error");
        exit(EXIT_FAILURE);
    }
    thread_check=pthread_create(&manager_thread_recive_packets,NULL,thread_recive_packets,NULL);
    if(thread_check!=0)
    {
        perror("Thread read created error");
        exit(EXIT_FAILURE);
    }
    
    while(alive)
    {   
        //pthread_mutex_lock(&lock_packets);
        bzero(message,sizeof(message));
        //memset(message,'\0',sizeof(message));
        //memset(packets,'\0',sizeof(packets));
        
        //printf("message: ");
        fgets(message,sizeof(message),stdin);
        //char *mess_temp = message;
        //scanf("%s",mess_temp);
        bzero(packets,sizeof(packets));

        if(strncmp(message,"exit",4)==0)
        {
            packets[0] = LOG_OUT;
            //pthread_mutex_lock(&lock_packets);
            strcat(packets,username);
            int k = write(sockfd,packets,sizeof(packets));
            //pthread_mutex_unlock(&lock_packets);
            if(k<0) {perror("Write error"); break;}
            close(sockfd);
            return 0;
            break;
        }
        else if(strncmp(message,"lu",2)==0)
        {
            printf("Active: ");
            for(int l=0;l<=all_user;l++)
            {
                
                printf("%s; ",listuser[l].user);
            }
            printf("\n");
        }
        // private chat: @user_recive/Hello friend
        else if(message[0] == '@')
        {
            //memset(user_recive,'\0',sizeof(user_recive));
            bzero(user_recive,sizeof(user_recive));
            for(int i=0;i<strlen(message) && message[i+1]!='/';i++)
            {
                user_recive[i] = message[i+1];
            }
            //memset(private_mess,'\0',sizeof(private_mess));
            bzero(private_mess,sizeof(private_mess));
            char* key = strstr(message,"/");
            key++;
            strcpy(private_mess,key);

            packets[0] = PRIVATE_CHAT;
            strcat(packets,user_recive);
            strcat(packets,"\t");
            strcat(packets,username);
            strcat(packets,": ");
            strcat(packets,private_mess);
            //pthread_mutex_lock(&lock_packets);
            int k = write(sockfd,packets,sizeof(packets));
            //pthread_mutex_unlock(&lock_packets);
            if(k<0) {perror("Write error"); break;}
        }
        else if(message[0]=='<')
        {
            bzero(file_name,sizeof(file_name));
            char *key = strstr(message,">");
            key++;
            strcpy(file_name,key);
            bzero(user_recive_file,sizeof(user_recive_file));
            for(int i=0;i<strlen(message) && message[i+1]!='>';i++)
            {
                user_recive_file[i] = message[i+1];
            }
            packets[0]=SEND_FILE;
            strcat(packets,user_recive_file);
            strcat(packets,"\t");
            strcat(packets,file_name);
            int k = write(sockfd,packets,sizeof(packets));
            //pthread_mutex_unlock(&lock_packets);
            if(k<0) {perror("Write error"); break;}
            printf("Senting file %s...\n",file_name);
            FILE *file;
	
            //bzero(buff,sizeof(buff));
            file = fopen(file_name,"r");
            if(file == NULL){perror("Open file error");exit(1);}
            while(!feof(file))
            {
                bzero(buff,sizeof(buff));
                fread(buff,sizeof(buff),100,file);
                //printf("%s",buff);
                packets_file[0]=SEND_CONTENT_FILE;
                strcat(packets_file,user_recive_file);
                strcat(packets_file,"\t");
                strcat(packets_file,buff);
                write(sockfd,packets_file,sizeof(packets_file));
            }
            fclose(file);
            bzero(packets_file,sizeof(packets_file));
            packets_file[0]=END_SEND_FILE;
            strcat(packets_file,user_recive_file);
            write(sockfd,packets_file,sizeof(packets_file));
            printf("Sent file complete\n");
            /*int check_thread =pthread_create(&manager_thread_send_file,NULL,thread_send_file,NULL);
            if(check_thread!=0)
            {
                perror("Thread read created error");
                exit(EXIT_FAILURE);
            }*/
            
        }
        else if(message[0]!=10)
        {
            // group chat if message[0] != @
            packets[0] = GROUP_CHAT;
            strcat(packets,username);
            strcat(packets,": ");
            strcat(packets,message);
            //pthread_mutex_lock(&lock_packets);
            int k = write(sockfd,packets,sizeof(packets));
            //pthread_mutex_unlock(&lock_packets);
            if(k<0) {perror("Write error"); break;}
        }
        //pthread_mutex_unlock(&lock_packets);
        //bzero(packets,sizeof(packets));
        //int k = read(sockfd,packets,sizeof(packets));
        //if(k<0) {perror("Read error"); break;}
        //printf("%s\n",packets);
        //printf("message[0] is:%d\n",message[0]);
        //if(packets[7]==message[0]) printf("Right!!\n");
        //else printf("not =\n");
           
    }
    
    pthread_join(manager_thread_check,&thread_retval);
    pthread_join(manager_thread_recive_packets,&thread_retval);
    //pthread_join(manager_thread_send_packets,&thread_retval);
    pthread_mutex_destroy(&lock_packets);
    //printf("Server not found\n");
    return 0;
    
}