#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];
char buf_out[MAX_SIZE+1];

/* 返回读取到的数据长度 */
int recv_msg(int socket_fd, char* buf, char* error_msg)
{
    int r_size;
    if ((r_size = recv(socket_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    return r_size;
}

void recv_mail()
{
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "***@qq.com"; // TODO: Specify the user
    const char* pass = "***"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    // 创建套接字文件描述符
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // 建立TCP连接
    struct sockaddr_in *s_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    s_addr->sin_family = AF_INET;
    s_addr->sin_port = htons(port);
    s_addr->sin_addr = (struct in_addr){inet_addr(dest_ip)};
    bzero(s_addr->sin_zero, 8);
    if (connect(s_fd, (struct sockaddr*)s_addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // TODO: Send user and password and print server response
    sprintf(buf, "USER %s\r\n", user);
    send(s_fd, buf, strlen(buf), 0);
    printf("\033[1;32m%s\033[0m", buf);
    recv_msg(s_fd, buf, "recv_user error");

    sprintf(buf, "PASS %s\r\n", pass);
    send(s_fd, buf, strlen(buf), 0);
    printf("\033[1;32m%s\033[0m", buf);
    recv_msg(s_fd, buf, "recv_pass error");

    // TODO: Send STAT command and print server response
    const char* STAT = "STAT\r\n";
    send(s_fd, STAT, strlen(STAT), 0);
    printf("\033[1;32m%s\033[0m", STAT);
    recv_msg(s_fd, buf, "recv_stat error");

    // TODO: Send LIST command and print server response
    const char* LIST = "LIST\r\n";
    send(s_fd, LIST, strlen(LIST), 0);
    printf("\033[1;32m%s\033[0m", LIST);
    recv_msg(s_fd, buf, "recv_list error");

    // TODO: Retrieve the first mail and print its content
    const char* RETR = "RETR 1\r\n";
    send(s_fd, RETR, strlen(RETR), 0);
    printf("\033[1;32m%s\033[0m", RETR);
    int read_size = recv_msg(s_fd, buf, "recv_retr error");
    // 根据返回内容偏移获取第一个邮件大小
    int mail_size = atoi(buf + 4);  // +4是因为返回内容是+OK 1 len\r\n，所以从第5个开始是邮件大小
    mail_size -= read_size;
    while (mail_size > 0)
    {
        read_size = recv_msg(s_fd, buf, "recv_retr error");
        mail_size -= read_size;
    }

    // TODO: Send QUIT command and print server response
    const char* QUIT = "QUIT\r\n";
    send(s_fd, QUIT, strlen(QUIT), 0);
    printf("\033[1;32m%s\033[0m", QUIT);
    recv_msg(s_fd, buf, "recv_quit error");

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
