#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

char buf[MAX_SIZE+1];

void recv_msg(int socket_fd, char* buf, char* error_msg)
{
    int r_size;
    if ((r_size = recv(socket_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "***@qq.com"; // TODO: Specify the user
    const char* pass = "***"; // TODO: Specify the password
    const char* from = "***"; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
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

    printf("dest_ip: %s\n", dest_ip);

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    const char* EHLO = "EHLO qq.com\r\n"; // TODO: Enter EHLO command here
    send(s_fd, EHLO, strlen(EHLO), 0);
    printf("\033[1;33m%s\033[0m", EHLO);

    // TODO: Print server response to EHLO command
    recv_msg(s_fd, buf, "recv EHLO");

    // TODO: Authentication. Server response should be printed out.
    const char* AUTH = "AUTH LOGIN\r\n";
    send(s_fd, AUTH, strlen(AUTH), 0);
    printf("\033[1;33m%s\033[0m", AUTH);
    recv_msg(s_fd, buf, "recv AUTH");

    // 输入用户名和密码
    char* user_encode = encode_str(user);
    strcat(user_encode, "\r\n");
    send(s_fd, user_encode, strlen(user_encode), 0);
    printf("\033[1;33mUser: %s\033[0m", user_encode);
    recv_msg(s_fd, buf, "recv user");
    free(user_encode);

    char* pass_encode = encode_str(pass);
    strcat(pass_encode, "\r\n");
    send(s_fd, pass_encode, strlen(pass_encode), 0);
    printf("\033[1;33mPass: %s\033[0m", pass_encode);
    recv_msg(s_fd, buf, "recv pass");
    free(pass_encode);

    // TODO: Send MAIL FROM command and print server response
    // 写入发件人
    sprintf(buf, "MAIL FROM:<%s>\r\n", from);
    send(s_fd, buf, strlen(buf), 0);
    printf("\033[1;33m%s\033[0m", buf);
    recv_msg(s_fd, buf, "recv MAIL FROM");
    // TODO: Send RCPT TO command and print server response
    // 写入收件人
    sprintf(buf, "RCPT TO:<%s>\r\n", receiver);
    send(s_fd, buf, strlen(buf), 0);
    printf("\033[1;33m%s\033[0m", buf);
    recv_msg(s_fd, buf, "recv RCPT TO");
    
    // TODO: Send DATA command and print server response
    // 发送DATA命令
    const char* DATA = "DATA\r\n";
    send(s_fd, DATA, strlen(DATA), 0);
    printf("\033[1;33m%s\033[0m", DATA);
    recv_msg(s_fd, buf, "recv DATA");

    // TODO: Send message data
    // 发送邮件内容
    sprintf(buf, "From: %s\r\nTo: %s\r\nMIME-Version: 1.0\r\nContent-Type: multipart/mixed; boundary=qwertyuiopasdfghjklzxcvbnm\r\n", 
                  from, receiver);
    send(s_fd, buf, strlen(buf), 0);
    printf("\033[1;33m%s\033[0m", buf);
    if (subject != NULL) {
        // 写入标题
        sprintf(buf, "Subject: %s\r\n\r\n", subject);
        send(s_fd, buf, strlen(buf), 0);
        printf("\033[1;33m%s\033[0m", buf);
    }
    if (msg != NULL) {
        // 指定消息头部的格式，MIME 类型为纯文本
        const char* msg_head = "--qwertyuiopasdfghjklzxcvbnm\r\nContent-Type: text/plain\r\n\r\n";
        send(s_fd, msg_head, strlen(msg_head), 0);
        printf("\033[1;33m%s\033[0m", msg_head);

        if (access(msg, F_OK) == 0) {
            // 如果msg是一个文件，读取文件内容并发送
            FILE* msg_file = fopen(msg, "r");
            if (msg_file == NULL) {
                perror("fopen msg_file");
                exit(EXIT_FAILURE);
            }
            while (fgets(buf, MAX_SIZE, msg_file) != NULL) {
                send(s_fd, buf, strlen(buf), 0);
                printf("%s", buf);
            }
            fclose(msg_file);
        } else {
            // 如果msg是一个字符串，直接发送
            send(s_fd, msg, strlen(msg), 0);
            printf("%s", msg);
        }
        // 添加回车换行
        send(s_fd, "\r\n", 2, 0);
        printf("\r\n");
    }
    // 添加附件
    if (att_path != NULL) {
        // 指定附件头部的格式，MIME 类型为二进制流
        sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\nContent-Type: application/octet-stream\r\nContent-Transfer-Encoding: Base64\r\nContent-Disposition: attachment;filename=%s\r\n\r\n", att_path);
        send(s_fd, buf, strlen(buf), 0);
        printf("\033[1;33m%s\033[0m", buf);

        // 对附件进行base64编码
        FILE* att = fopen(att_path, "r");
        if (att == NULL) {
            perror("fopen att");
            exit(EXIT_FAILURE);
        }
        // 将编码后的内容写入文件，不存在则创建
        FILE* output = fopen("output.zip", "w");
        if (output == NULL) {
            perror("fopen output");
            exit(EXIT_FAILURE);
        }
        encode_file(att, output);
        fclose(att);
        fclose(output);

        // 读取编码后的内容并发送
        FILE* file_encode = fopen("output.zip", "r");
        while (fgets(buf, MAX_SIZE, file_encode) != NULL) {
            send(s_fd, buf, strlen(buf), 0);
            printf("%s", buf);
        }
        fclose(file_encode);
        
        // 结束附件
        const char* att_end = "--qwertyuiopasdfghjklzxcvbnm\r\n";
        send(s_fd, att_end, strlen(att_end), 0);
        printf("\033[1;33m%s\033[0m", att_end);
    }

    // TODO: Message ends with a single period
    // 结束邮件
    send(s_fd, end_msg, strlen(end_msg), 0);
    printf("\033[1;33m%s\033[0m", end_msg);
    recv_msg(s_fd, buf, "recv end_msg");

    // TODO: Send QUIT command and print server response
    // 发送QUIT命令
    const char* QUIT = "QUIT\r\n";
    send(s_fd, QUIT, strlen(QUIT), 0);
    printf("\033[1;33m%s\033[0m", QUIT);
    recv_msg(s_fd, buf, "recv QUIT");

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
