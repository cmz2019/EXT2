#include <stdio.h>
#include <string.h>
#include "main.h"
#include "init.h"

int main() {
    char command[10], temp[100];
    char username[10], password[10];
    initialize_user();
    while (1) { // 登录
        printf("please login:\n");
        printf("user->");
        scanf("%s", username);
        if (!strcmp(username, "quit") || !strcmp(username, "exit"))
            return 0;
        printf("password->");
        scanf("%s", password);
        if (login(username, password)) {
            strcpy(current_user, username);
            strcpy(current_path, "[");
            strcat(current_path, current_user);
            strcat(current_path, "@ext2 /");
            printf("User %s sign in!\n", username);
            break;
        }
        else {
            printf("User name or password wrong, please enter again!\n");
            printf("If want to exit, please enter \"quit\" or \"exit\"!\n");
        }
    }
    initialize_memory();
    while (1) {
        printf("%s]#", current_path);
        scanf("%s", command);
        if (!strcmp(command, "cd")) { //进入当前目录下
            scanf("%s", temp);
            cd(temp);
        }
        else if (!strcmp(command, "mkdir")) { //创建目录
            scanf("%s", temp);
            mkdir(temp, 2);
        }
        else if (!strcmp(command, "touch")) {   //创建文件
            scanf("%s", temp);
            cat(temp, 1);
        }
        else if (!strcmp(command, "rmdir")) { //删除空目录
            scanf("%s", temp);
            rmdir(temp);
        }
        else if (!strcmp(command, "rm")) {    //删除文件或目录，不提示
            scanf("%s", temp);
            del(temp);
        }
        else if (!strcmp(command, "open")) {   //打开一个文件
            scanf("%s", temp);
            open_file(temp);
        }
        else if (!strcmp(command, "close")) {   //关闭一个文件
            scanf("%s", temp);
            close_file(temp);
        }
        else if (!strcmp(command, "read")) {   //读一个文件
            scanf("%s", temp);
            read_file(temp);
        }
        else if (!strcmp(command, "write")) {  //写一个文件
            scanf("%s", temp);
            write_file(temp);
        }
        else if (!strcmp(command, "ls")) {   //显示当前目录
            ls();
        }
        else if (!strcmp(command, "format")) {  //格式化硬盘
            char tempch;
            printf("Format will erase all the data in the Disk\n");
            printf("Are you sure? (y/n):\n");
            fflush(stdin);
            scanf(" %c", &tempch);
            if (tempch == 'Y' || tempch == 'y') {
                printf("Formatting...\n");
                format();
            }
            else {
                printf("Format Disk canceled\n");
            }
        }
        else if (!strcmp(command, "ckdisk")) {  //检查硬盘
            check_disk();
        }
        else if (!strcmp(command, "help") || !strcmp(command, "h")) { //查看帮助
            help();
        }
        else if (!strcmp(command, "quit") || !strcmp(command, "exit")) { //退出系统
            printf("Good Bye!\n");
            break;
        }
        else if (!strcmp(command, "chmod")) { //修改权限
            scanf("%s", temp);
            unsigned short mode;
            scanf("%hd", &mode);
            chmod(temp, mode);
        }
        else {
            printf("No this Command,Please check!\n");
            help();
        }
        getchar();
    }
    return 0;
}