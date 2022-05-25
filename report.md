# 设计EXT2文件系统

## 实验目的

（1）掌握文件系统的工作原理

（2）理解文件系统的主要数据结构

（3）学习较为复杂的 Linux 下的编程

（4）了解 EXT2 文件系统的结构

## 实验内容

设计并实现一个二级文件系统程序：

1. 提供以下操作：

+ 文件创建 / 删除接口命令 `touch` / `rm`

+ 目录创建 / 删除接口命令 `mkdir` / `rmdir`

+ 显示目录内容命令 `ls`
+ 切换目录命令 `cd`
+ 读 / 写文件命令 `read` / `write`
+ 更改文件权限命令 `chmod`

2. 创建的文件不要求格式和内容。

3. 具备提供用户登录的功能。

4. 文件、目录要有权限。

## 实验步骤

### 程序设计思想

要求设计一个简单的文件系统，它包含了格式化、显示（文件）目录、创建文件、登录等几个简单命令的实现，而且能完成超级块的读写、inode 的读写等过程，是一个比真正的文件系统简单得多，而又能基本体现文件系统理论的程序。在超级块的使用上，采用了操作系统关于这方面的经典理论；而在节点的使用上，主要是模仿 Linux 的 EXT2 文件系统。

#### 程序流程

这是一个面向过程的程序，它的流程图如下图：

![clip_image002](https://gitee.com/cmz2000/album/raw/master/image/clip_image002.png)

 

#### 文件系统的设计思想

真正的文件系统，在涉及文件读写、文件创建等操作时，会用到内外存之间通信的语句。这些与底层硬件有关的编程一方面会给完整完成实验的人员制造不小的麻烦，更为重要的是这些内容不属于操作系统原理的范畴。因此本实验所设计的文件系统程序与真正的文件系统在某些方面有着本质的不同。

下表是一个大致的比较：

| **实验设计的文件系统** | **真实文件系统**     |
| ---------------------- | -------------------- |
| 二进制格式             | 自定义系统文件格式   |
| 依赖于其他操作系统     | 不依赖于其他操作系统 |
| 调用库函数访问外村     | 调用中断访问外存     |

实验要求设计的文件系统使用一个二进制文件来模拟磁盘空间，该 “文件系统” 的所有用户信息、inode 信息、超级块信息、文件信息均以二进制方式保存在文件的特定地方，如下图。

![clip_image004.png](https://gitee.com/cmz2000/album/raw/master/image/clip_image004.png)

### 设计文件系统的数据结构

#### 1）一些常量，根据上一步的图设计

```c
#define VOLUME_NAME         "EXT2FS"    //卷名
#define BLOCK_SIZE          512         //块大小
#define DISK_SIZE           4612        //磁盘总块数

#define DISK_START          0           //磁盘开始地址
#define SB_SIZE             32          //超级块大小是32B

#define GD_SIZE             32          //块组描述符大小是32B
#define GDT_START           (0+512)     //块组描述符起始地址

#define BLOCK_BITMAP        (512+512)   //块位图起始地址 2*512
#define INODE_BITMAP        (1024+512)  //inode 位图起始地址 3*512

#define INODE_TABLE         (1536+512)  //索引节点表起始地址 4*512
#define INODE_SIZE          64          //每个inode的大小是 64B
#define INODE_TABLE_COUNTS  4096        //inode entry 数

#define DATA_BLOCK          264192      //数据块起始地址 4*512+4096*64
#define DATA_BLOCK_COUNTS   4096        //数据块数

#define BLOCKS_PER_GROUP    4612        //每组中的块数

#define USER_MAX            4           //用户个数
#define FOPEN_TABLE_MAX     16          //文件打开表大小
```

####  2）超级块的数据结构

```c
struct super_block { // 32 B
	char sb_volume_name[16]; //文件系统名
   	unsigned short sb_disk_size; //磁盘总大小
   	unsigned short sb_blocks_per_group; // 每组中的块数
   	unsigned short sb_size_per_block;  // 块大小
   	char sb_pad[10];  //填充
};
```

#### 3）组描述符的数据结构

```c
struct group_desc { // 32 B
    char bg_volume_name[16]; //文件系统名
    unsigned short bg_block_bitmap; //块位图的起始块号
    unsigned short bg_inode_bitmap; //索引结点位图的起始块号
    unsigned short bg_inode_table; //索引结点表的起始块号
    unsigned short bg_free_blocks_count; //本组空闲块的个数
    unsigned short bg_free_inodes_count; //本组空闲索引结点的个数
    unsigned short bg_used_dirs_count; //组中分配给目录的结点数
    char bg_pad[4]; //填充(0xff)
};
```

####  4）索引结点的数据结构

```c
struct inode { // 64 B = 6 * 2B + 5 * 4B + 16B + 16B
    unsigned short i_mode;   //文件类型及访问权限
    unsigned short i_blocks; //文件所占的数据块个数(0~7), 最大为7
    unsigned short i_uid;    //文件拥有者标识号
    unsigned short i_gid;    //文件的用户组标识符
    unsigned short i_links_count; //文件的硬链接计数
    unsigned short i_flags;  //打开文件的方式
    unsigned long i_size;    //文件或目录大小(单位 byte)
    unsigned long i_atime;   //访问时间
    unsigned long i_ctime;   //创建时间
    unsigned long i_mtime;   //修改时间
    unsigned long i_dtime;   //删除时间
    unsigned short i_block[8]; //直接索引方式 指向数据块号
    char i_pad[16];           //填充(0xff)
};
```

####  5）目录项入口的数据结构

```c
struct dir_entry { // 16 B
    unsigned short inode; //索引节点号
    unsigned short rec_len; //目录项长度
    unsigned short name_len; //文件名长度
    char file_type; //文件类型(1 普通文件 2 目录.. )
    char name[9]; //文件名
};
```

####  6）用户信息的数据结构

```c
struct user {
    char username[10];
    char password[10];
    unsigned short u_uid;   //用户标识号
    unsigned short u_gid;
}User[USER_MAX];
```

### 文件系统实现的功能函数

```c
extern void initialize_user();  //初始化用户
extern int login(char username[10], char password[10]); //用户登录
extern void initialize_memory(); //初始化内存
extern void format(); //格式化文件系统
extern void cd(char tmp[100]); //进入某个目录，实际上是改变当前路径
extern void mkdir(char tmp[100], int type); //创建目录
extern void cat(char tmp[100], int type); //创建文件
extern void rmdir(char tmp[100]); //删除一个空目录
extern void del(char tmp[100]); //删除文件
extern void open_file(char tmp[100]); //打开文件
extern void close_file(char tmp[100]); //关闭文件
extern void read_file(char tmp[100]); //读文件内容
extern void write_file(char tmp[100]); //文件以覆盖方式写入
extern void ls(); //查看目录下的内容
extern void check_disk(); //检查磁盘状态
extern void help(); //查看指令
extern void chmod(char tmp[100], unsigned short mode); //修改文件权限
```

### 一些读写方法的处理

进行超级块、组描述符、位图、inode 节点、数据块的读写时，需要用到一些缓冲区，定义如下：

```c
static struct super_block sb_block[1];  // 超级块缓冲区
static struct group_desc gdt[1];  // 组描述符缓冲区
static struct inode inode_area[1]; // inode缓冲区
static unsigned char bitbuf[512] = {0}; // block位图缓冲区
static unsigned char ibuf[512] = {0}; // inode位图缓冲区
static struct dir_entry dir[32];  // 目录项缓冲区 32*16=512
static char Buffer[BLOCK_SIZE]; // 针对数据块的缓冲区
```

####  1）读写超级块

```c
// 写超级块
static void update_super_block() {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, DISK_START, SEEK_SET);
    fwrite(sb_block, SB_SIZE, 1, fp);
    fflush(fp); //立刻将缓冲区的内容输出，保证磁盘内存数据的一致性
}

// 读超级块
static void reload_super_block() {
    fseek(fp, DISK_START, SEEK_SET);
    fread(sb_block, SB_SIZE, 1, fp);//读取内容到超级块缓冲区中
}
```

先定位文件内部位置指针的位置，超级块的起始位置在前面步骤的分析图中已给出，再进行块的读写，其中定位文件内部位置指针用到的库函数为 `int fseek(FILE *stream, long offset, int fromwhere)`。同时在写完后用库函数 `int fflush(FILE *stream)` 清除读写缓冲区。

####  2）读写组描述符

```c
// 写组描述符
static void update_group_desc() {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, GDT_START, SEEK_SET);
    fwrite(gdt, GD_SIZE, 1, fp);
    fflush(fp);
}

// 读组描述符
static void reload_group_desc() {
    fseek(fp, GDT_START, SEEK_SET);
    fread(gdt, GD_SIZE, 1, fp);
}
```

组描述符的起始位置同样在前面步骤的分析图中已给出。

####  3）读写块位图

```c
// 写 block 位图
static void update_block_bitmap() {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, BLOCK_BITMAP, SEEK_SET);
    fwrite(bitbuf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读 block 位图
static void reload_block_bitmap() {
    fseek(fp, BLOCK_BITMAP, SEEK_SET);
    fread(bitbuf, BLOCK_SIZE, 1, fp);
}
```

####  4）读写 inode 位图

```c
// 写 inode 位图
static void update_inode_bitmap() {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, INODE_BITMAP, SEEK_SET);
    fwrite(ibuf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读 inode 位图
static void reload_inode_bitmap() {
    fseek(fp, INODE_BITMAP, SEEK_SET);
    fread(ibuf, BLOCK_SIZE, 1, fp);
}
```

####  5）读写第 i 个 inode

```c
// 写第 i 个 inode
static void update_inode_entry(unsigned short i) {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, INODE_TABLE + (i - 1) * INODE_SIZE, SEEK_SET);
    fwrite(inode_area, INODE_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个 inode
static void reload_inode_entry(unsigned short i) {
    fseek(fp, INODE_TABLE + (i - 1) * INODE_SIZE, SEEK_SET);
    fread(inode_area, INODE_SIZE, 1, fp);
}
```

####  6）读写第 i 个数据块

```c
// 写第 i 个数据块
static void update_dir(unsigned short i) {
    fp = fopen("./Ext2", "rb+");
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fwrite(dir, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

// 读第 i 个数据块
static void reload_dir(unsigned short i) {
    fseek(fp, DATA_BLOCK + i * BLOCK_SIZE, SEEK_SET);
    fread(dir, BLOCK_SIZE, 1, fp);
} 
```

### 内存的查找、分配、删除等方法

#### 1）分配数据块

```c
// 分配一个数据块,返回数据块号
static int alloc_block() {
    //bitbuf共有512个字节，表示4096个数据块。根据last_alloc_block/8计算它在bitbuf的哪一个字节
    unsigned short cur = last_alloc_block;
    unsigned char con = 128; // 1000 0000b
    int flag = 0;
    if (gdt[0].bg_free_blocks_count == 0) {
        printf("There is no block to be allocated!\n");
        return (0);
    }
    reload_block_bitmap();
    cur /= 8;
    while (bitbuf[cur] == 255) { //该字节的8个bit都已有数据
        if (cur == 511)
            cur = 0; //最后一个字节也已经满，从头开始寻找
        else
            cur++;
    }
    while (bitbuf[cur] & con) { //在一个字节中找具体的某一个bit
        con = con / 2;
        flag++;
    }
    bitbuf[cur] = bitbuf[cur] + con;
    last_alloc_block = cur * 8 + flag;

    update_block_bitmap();
    gdt[0].bg_free_blocks_count--;
    update_group_desc();
    return last_alloc_block;
}
```

####  2）分配 inode

```c
// 分配一个 inode
static int get_inode() {
    unsigned short cur = last_alloc_inode;
    unsigned char con = 128;
    int flag = 0;
    if (gdt[0].bg_free_inodes_count == 0) {
        printf("There is no Inode to be allocated!\n");
        return 0;
    }
    reload_inode_bitmap();

    cur = (cur - 1) / 8;   //第一个标号是1，但是存储是从0开始的
    while (ibuf[cur] == 255) { //先看该字节的8个位是否已经填满
        if (cur == 511)
            cur = 0;
        else
            cur++;
    }
    while (ibuf[cur] & con) { //再看某个字节的具体哪一位没有被占用
        con = con / 2;
        flag++;
    }
    ibuf[cur] = ibuf[cur] + con;
    last_alloc_inode = cur * 8 + flag + 1;
    update_inode_bitmap();
    gdt[0].bg_free_inodes_count--;
    update_group_desc();
    return last_alloc_inode;
}
```

#### 3）查找文件或目录

```c
// 查找当前目录中名为tmp的文件或目录，并得到该文件的inode号，它在上级目录中的数据块号以及数据块中目录的项号
static unsigned short research_file(char tmp[100], int file_type, unsigned short *inode_num,
                                   unsigned short *block_num, unsigned short *dir_num) {
    unsigned short j, k;
    reload_inode_entry(current_dir); //进入当前目录
    j = 0;
    while (j < inode_area[0].i_blocks) {
        reload_dir(inode_area[0].i_block[j]);
        k = 0;
        while (k < 32) {
            if (!dir[k].inode || dir[k].file_type != file_type || strcmp(dir[k].name, tmp)) {
                k++;
            }
            else {
                *inode_num = dir[k].inode;
                *block_num = j;
                *dir_num = k;
                return 1;
            }
        }
        j++;
    }
    return 0;
}
```

#### 4）为新增目录或文件分配 dir_entry

```c
// 为新增目录或文件分配 dir_entry
// 对于新增文件，只需分配一个 inode 号
// 对于新增目录，除了 inode 号外，还需要分配数据区存储.和..两个目录项
static void dir_prepare(unsigned short tmp, unsigned short len, int type) {
    reload_inode_entry(tmp);

    if (type == 2) { // 目录
        inode_area[0].i_size = 32;
        inode_area[0].i_blocks = 1;
        inode_area[0].i_block[0] = alloc_block();
        dir[0].inode = tmp;
        dir[1].inode = current_dir;
        dir[0].name_len = len;
        dir[1].name_len = current_dirlen;
        dir[0].file_type = dir[1].file_type = 2;

        for (type = 2; type < 32; type++)
            dir[type].inode = 0;
        strcpy(dir[0].name, ".");
        strcpy(dir[1].name, "..");
        update_dir(inode_area[0].i_block[0]);

        inode_area[0].i_mode = 01006;
    }
    else {
        inode_area[0].i_size = 0;
        inode_area[0].i_blocks = 0;
        inode_area[0].i_mode = 0407;
    }
    update_inode_entry(tmp);
}
```

#### 5）删除一个块

```c
// 删除一个块
static void remove_block(unsigned short del_num) {
    unsigned short tmp;
    tmp = del_num / 8;
    reload_block_bitmap();
    switch (del_num % 8) { // 更新block位图 将具体的位置为0
        case 0:
            bitbuf[tmp] = bitbuf[tmp] & 127;
            break; // bitbuf[tmp] & 0111 1111b
        case 1:
            bitbuf[tmp] = bitbuf[tmp] & 191;
            break; //bitbuf[tmp]  & 1011 1111b
        case 2:
            bitbuf[tmp] = bitbuf[tmp] & 223;
            break; //bitbuf[tmp]  & 1101 1111b
        case 3:
            bitbuf[tmp] = bitbuf[tmp] & 239;
            break; //bitbuf[tmp]  & 1110 1111b
        case 4:
            bitbuf[tmp] = bitbuf[tmp] & 247;
            break; //bitbuf[tmp]  & 1111 0111b
        case 5:
            bitbuf[tmp] = bitbuf[tmp] & 251;
            break; //bitbuf[tmp]  & 1111 1011b
        case 6:
            bitbuf[tmp] = bitbuf[tmp] & 253;
            break; //bitbuf[tmp]  & 1111 1101b
        case 7:
            bitbuf[tmp] = bitbuf[tmp] & 254;
            break; // bitbuf[tmp] & 1111 1110b
    }
    update_block_bitmap();
    gdt[0].bg_free_blocks_count++;
    update_group_desc();
}
```

#### 6）删除一个 inode

```c
// 删除一个 inode
static void remove_inode(unsigned short del_num) {
    unsigned short tmp;
    tmp = (del_num - 1) / 8;
    reload_inode_bitmap();
    switch ((del_num - 1) % 8) { //更改block位图
        case 0:
            bitbuf[tmp] = bitbuf[tmp] & 127;
            break;
        case 1:
            bitbuf[tmp] = bitbuf[tmp] & 191;
            break;
        case 2:
            bitbuf[tmp] = bitbuf[tmp] & 223;
            break;
        case 3:
            bitbuf[tmp] = bitbuf[tmp] & 239;
            break;
        case 4:
            bitbuf[tmp] = bitbuf[tmp] & 247;
            break;
        case 5:
            bitbuf[tmp] = bitbuf[tmp] & 251;
            break;
        case 6:
            bitbuf[tmp] = bitbuf[tmp] & 253;
            break;
        case 7:
            bitbuf[tmp] = bitbuf[tmp] & 254;
            break;
    }
    update_inode_bitmap();
    gdt[0].bg_free_inodes_count++;
    update_group_desc();
}
```

#### 7）在打开文件表中查找是否已打开某个文件

```c
// 在打开文件表中查找是否已打开文件
static unsigned short search_file(unsigned short Inode) {
    unsigned short fopen_table_point = 0;
    while (fopen_table_point < 16 && fopen_table[fopen_table_point++] != Inode);
    if (fopen_table_point == 16) {
        return 0;
    }
    return 1;
}
```

### 程序编译运行

代码编写完成后，共有4个文件，源码见附件。

![image-20210611164555756](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164555756.png)

把代码文件上传到 `openEuler` 操作系统（本实验采用 openEuler，当然其他操作系统也可以）中，使用命令 `gcc main.c init.c –o ext2` 进行编译链接：

![image-20210611164717834](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164717834.png)

编译链接成功，得到可执行文件 ext2，下面开始进行运行测试：

![image-20210611164820654](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164820654.png)

键入 `./ext2` 运行可执行文件，效果图如下：

![image-20210611164843553](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164843553.png)

这是简易的登录界面，输入内置的用户名 test 和密码 test，登录成功，程序开始安装文件系统并执行初始化：

![image-20210611164921500](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164921500.png)

这时我们使用 `quit` 命令退出程序，然后查看当前目录，发现多了一个 Ext2 文件，这是被程序创建出来的二进制文件，用于模拟磁盘空间。

![image-20210611164942975](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164942975.png)

然后再次进入文件系统，登录，我们发现这次程序没有再进行文件系统的安装和初始化等工作，这是因为每次进入文件系统时，程序会检查文件系统是否存在，不存在时才会创建。

![image-20210611164955629](https://gitee.com/cmz2000/album/raw/master/image/image-20210611164955629.png)

下面我们开始测试程序功能：

1）`ls` 功能，成功

![image-20210611165032414](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165032414.png)

2）`mkdir` 创建目录，成功

![image-20210611165053651](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165053651.png)

3）`touch` 创建文件，成功

![image-20210611165121913](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165121913.png)

4）`cd` 功能进入目录 dir，创建文件，也成功

![image-20210611165148118](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165148118.png)

可以发现使用 cd 切换目录时，目录的变化在 @ 字符后显示了。

5）`rm` 删除 dir 目录下的文件 file，成功

![image-20210611165233850](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165233850.png)

6）返回主目录中，删除 dir 文件夹，成功

![image-20210611165316083](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165316083.png)

7）输入 `help` 显示指令帮助信息，或输入一个错误指令，文件系统自动输出指令帮助，成功

![image-20210611165325733](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165325733.png)

8）`write` 向file文件中写一些信息，系统会提示还未打开文件

![image-20210611165343186](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165343186.png)

打开文件 file 后，再次执行写命令向 file 写入信息，以 `#` 结束，成功，我们发现向 file 文件写入信息后，file 文件的大小变为了 33bytes

![image-20210611165431539](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165431539.png)

9）`read` ，下面读取 file 文件中的信息，上一步写入的信息都被读出来了，然后关闭 file 文件，成功

![image-20210611165459261](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165459261.png)

10）改变 file 文件的权限，权限的表示方式为二进制 111b，最高位表示读权限，第二位表示写权限，最低位表示执行权限，1 表示有权限，0 表示无权限。先查看当前 file 文件的权限，为 111b

![image-20210611165523149](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165523149.png)

然后我们把 file 的权限改为 011b（十进制为3），即没有读权限

![image-20210611165539911](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165539911.png)

可以看到现在 file 文件已经没有读权限了，再次尝试读取 file 文件中的内容，被文件系统阻止，并提示 file 文件不能读。

![image-20210611165556525](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165556525.png)

11）测试检查磁盘状态功能，成功

![image-20210611165611690](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165611690.png)

12）测试格式化文件系统功能，发现文件系统中所有文件已被格式化，成功

![image-20210611165619357](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165619357.png)

13）退出文件系统，成功

![image-20210611165627230](https://gitee.com/cmz2000/album/raw/master/image/image-20210611165627230.png)

## 心得体会

通过本次设计并实现 ext2 文件系统，我对 ext2 文件系统的思想、结构、设计方式有了深入的了解，在实现的过程中，也对一些用到的函数掌握程度更深入，比如 `fopen`，`fseek`，`fwrite`，`fread`，`fflush` 等函数，总的来说，这次实验让我收获颇丰。