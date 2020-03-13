#include "string.h"

int isspace(char ch) {
    if(ch == 10 || ch == 32 || ch == 9) 
        return 1;
    else 
        return 0;
}

int strcmp(char *str1, char *str2) 
{
    int i = 0;
    while(str1[i] != '\0' || str2[i] != '\0') {
        if(str1[i] > str2[i]) 
            return str1[i] - str2[i];           //str1이 str2보다 크면 양수
        else if(str1[i] < str2[i]) 
            return str1[i] - str2[i];           //str2가 str1보다 크면 음수 
        i++;        
    }
    return 0;                                   //str1과 str2가 같으면 0
}

int strlen(char *str)
{
    int i = 0;
    while(str[i] != '\0') {
        i++;
    }
    return i;
}

int strcpy(char *str1, char *str2) {
    int i = 0;
    while(str2[i] != '\0') {
        str1[i] = str2[i];
        i++;
    }
    str1[i] = '\0';
    return 0;
}

int atoi(char *str) {    
    int tot=0;
    while(*str){    // NULL까지
        tot = tot*10 + *str - '0';  // 자리수를 올려주며 수를 가져옴
        str++;    // 다음 문자 주소로 넘김
    }
    return tot;
}

