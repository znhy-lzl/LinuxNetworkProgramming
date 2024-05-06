# include <iostream>

/* C++程序判断方式 */
bool isBigEndian()
{
    int number = 1;
    /* 将整数1的地址强制转换为指向字符数组的指针 */
    char *ch = reinterpret_cast<char*>(&number);

    /* 如果首字节的内存地址存储的是0，即高地址存储低位字节，则为大端字节序 */
    return static_cast<int>(ch[0]) == 0;
}

/* C程序判断方式 */
void byterorder()
{
    union {
        short value;
        char union_bytes[sizeof(short)];
    } test;

    test.value = 0x0102;

    if ((test.union_bytes[0] ==1) && (test.union_bytes[1] == 2)) {
        std::cout << "This machine is Big-Endian byte order." << std::endl;
    } else if ((test.union_bytes[0] ==2) && (test.union_bytes[1] == 1)) {
        std::cout << "This machine is Little-Endian byte order." << std::endl;
    } else {
        std::cout << "unknown..." << std::endl;
    }
}


int main()
{
    if (isBigEndian()) {
        std::cout << "This machine is Big-Endian byte order." << std::endl;
    } else {
        std::cout << "This machine is Little-Endian byte order." << std::endl;
    }

    byterorder();

    return 0;
}
