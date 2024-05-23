// main 함수 채운 후 테스트 시작하면 됨

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "student.h"

// readPage() 함수는 레코드 파일에서 주어진 페이지번호 pagenum(=0, 1, 2, ...)에 해당하는 page를
// 읽어서 pagebuf가 가리키는 곳에 저장하는 역할을 수행한다. 정상적인 수행을 한 경우
// '1'을, 그렇지 않은 경우는 '0'을 리턴한다.
int readPage(FILE *fp, char *pagebuf, int pagenum) {
    fseek(fp, FILE_HEADER_SIZE + pagenum * PAGE_SIZE, SEEK_SET);
    return fread(pagebuf, PAGE_SIZE, 1, fp) == 1 ? 1 : 0;
}

// getRecFromPagebuf() 함수는 readPage()를 통해 읽어온 page에서 주어진 레코드번호 recordnum(=0, 1, 2, ...)에
// 해당하는 레코드를 recordbuf에 전달하는 일을 수행한다.
// 만약 page에서 전달할 record가 존재하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
int getRecFromPagebuf(const char *pagebuf, char *recordbuf, int recordnum) {
    // 레코드의 개수가 맞는지 확인
    short num_records;
    memcpy(&num_records, pagebuf, sizeof(short));
    if (recordnum >= num_records) return 0;

    // 현재 레코드의 offset가져오기
    short offset;
    memcpy(&offset, pagebuf + 8 + recordnum * sizeof(short), sizeof(short));

    short start_ptr;
    int len;
    if (recordnum == 0) {
        len = offset + 1; 
        memcpy(recordbuf, pagebuf + PAGE_HEADER_SIZE, len);
    }
    else if (recordnum >= 1) {
        short before_offset; 
        memcpy(&before_offset, pagebuf + 8 + (recordnum - 1) * sizeof(short), sizeof(short));
        len = offset - before_offset;     
        memcpy(recordbuf, pagebuf + PAGE_HEADER_SIZE + before_offset + 1, len); 

    }
    recordbuf[len] = '\0';
    return 1;
}

// unpack() 함수는 recordbuf에 저장되어 있는 record에서 각 field를 추출하여 학생 객체에 저장하는 일을 한다.
void unpack(const char *recordbuf, STUDENT *s) {
    sscanf(recordbuf, "%8[^#]#%13[^#]#%16[^#]#%1[^#]#%20[^#]#%15[^#]#%20[^#]#", s->id, s->name, s->dept, s->year, s->addr, s->phone, s->email);
}

// 주어진 학생 객체를 레코드 파일에 저장할 때 사용되는 함수들이며, 그 시나리오는 다음과 같다.
// 1. 학생 객체를 실제 레코드 형태의 recordbuf에 저장한다(pack() 함수 이용).

// 2. 레코드 파일의 맨마지막 page를 pagebuf에 읽어온다(readPage() 이용). 만약 새로운 레코드를 저장할 공간이
//    부족하면 pagebuf에 empty page를 하나 생성한다.

// 3. recordbuf에 저장되어 있는 레코드를 pagebuf의 page에 저장한다(writeRecToPagebuf() 함수 이용).

// 4. 수정된 page를 레코드 파일에 저장한다(writePage() 함수 이용)
// writePage()는 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
int writePage(FILE *fp, const char *pagebuf, int pagenum) {
    fseek(fp, FILE_HEADER_SIZE + pagenum * PAGE_SIZE, SEEK_SET);
    return fwrite(pagebuf, PAGE_SIZE, 1, fp) == 1 ? 1 : 0;
}

void writeRecToPagebuf(char *pagebuf, const char *recordbuf) {

    int rec_length = strlen(recordbuf);

    int num_records;
    memcpy(&num_records, pagebuf, sizeof(short));

    int free_space;
    memcpy(&free_space, pagebuf + 2, sizeof(short));

    int reserved_space;
    memcpy(&reserved_space, pagebuf+ 4, 4);

    short offset;
    if (num_records == 0) {
        offset = rec_length - 1;
        memcpy(pagebuf + 8 + num_records * sizeof(short), &offset, sizeof(short));
        memcpy(pagebuf + PAGE_HEADER_SIZE, recordbuf, rec_length);
    } 
    else {
        short before_offset; 
        memcpy(&before_offset, pagebuf + 8 + (num_records - 1) * sizeof(short), sizeof(short));
        offset = rec_length + before_offset;
        memcpy(pagebuf + 8 + num_records * sizeof(short), &offset, sizeof(short));
        int before_length = before_offset + 1;
        memcpy(pagebuf + PAGE_HEADER_SIZE + before_length, recordbuf, rec_length);
    }

    num_records++;
    memcpy(pagebuf, &num_records, sizeof(short));

    free_space -= rec_length;
    memcpy(pagebuf + 2, &free_space, sizeof(short));

    reserved_space = PAGE_SIZE - PAGE_HEADER_SIZE - free_space; 
    memcpy(pagebuf + 4, &reserved_space, 4);
}

void pack(char *recordbuf, const STUDENT *s) {
    sprintf(recordbuf, "%s#%s#%s#%s#%s#%s#%s#", s->id, s->name, s->dept, s->year, s->addr, s->phone, s->email);
}

// 레코드 파일에서 field의 키값(keyval)을 갖는 레코드를 검색하고 그 결과를 출력한다.
// 검색된 레코드를 출력할 때 반드시 printSearchResult() 함수를 사용한다.
void search(FILE *fp, FIELD f, char *keyval) {
    char pagebuf[PAGE_SIZE];
    char recordbuf[MAX_RECORD_SIZE];
    STUDENT s;
    char headerbuf[FILE_HEADER_SIZE];
    int num_pages;
    readFileHeader(fp, headerbuf);
    memcpy(&num_pages, headerbuf, sizeof(short));

    int found = 0;
    // 이거 오류나면 다른 걸로 해도 됨
    STUDENT *stu = malloc((found + 1) * sizeof(STUDENT));
    //
    for (int pagenum = 0; pagenum < num_pages; pagenum++) {
        if (!readPage(fp, pagebuf, pagenum)) {
            continue;
        }
        int num_records;
        memcpy(&num_records, pagebuf, sizeof(short));
        for (int recnum = 0; recnum < num_records; recnum++) {
            if (!getRecFromPagebuf(pagebuf, recordbuf, recnum)) {
                continue;
            }
            unpack(recordbuf, &s);

            if ((f == ID && strcmp(s.id, keyval) == 0) ||
                (f == NAME && strcmp(s.name, keyval) == 0) ||
                (f == DEPT && strcmp(s.dept, keyval) == 0) ||
                (f == YEAR && strcmp(s.year, keyval) == 0) ||
                (f == ADDR && strcmp(s.addr, keyval) == 0) ||
                (f == PHONE && strcmp(s.phone, keyval) == 0) ||
                (f == EMAIL && strcmp(s.email, keyval) == 0)) {
                strcpy(stu[found].id, s.id);
                strcpy(stu[found].name, s.name);
                strcpy(stu[found].dept, s.dept);
                strcpy(stu[found].year, s.year);
                strcpy(stu[found].addr, s.addr);
                strcpy(stu[found].phone, s.phone);
                strcpy(stu[found].email, s.email);
                found++;
            }
        }
    }
    printSearchResult(stu, found);
}

void printSearchResult(const STUDENT *s, int n)
{
	int i;

	printf("#Records = %d\n", n);
	
	for(i=0; i<n; i++)
	{
		printf("%s#%s#%s#%s#%s#%s#%s\n", s[i].id, s[i].name, s[i].dept, s[i].year, s[i].addr, s[i].phone, s[i].email);
	}
}

// 레코드 파일에 새로운 학생 레코드를 추가할 때 사용한다. 표준 입력으로 받은 필드값들을 이용하여
// 학생 객체로 바꾼 후 insert() 함수를 호출한다.
void insert(FILE *fp, const STUDENT *s) {
    char recordbuf[MAX_RECORD_SIZE];
    char pagebuf[PAGE_SIZE];
    char headerbuf[FILE_HEADER_SIZE];

    // recordebuf에 학생 레코드 저장
    pack(recordbuf, s);

    int num_pages;
    readFileHeader(fp, headerbuf);
    memcpy(&num_pages, headerbuf, sizeof(short));

    readPage(fp, pagebuf, num_pages - 1);

    int num_records;
    memcpy(&num_records, pagebuf, sizeof(short));
        
    int free_space;
    memcpy(&free_space, pagebuf + 2, sizeof(short));

    if (free_space>=strlen(recordbuf)) {
        writeRecToPagebuf(pagebuf, recordbuf);
        writePage(fp, pagebuf, num_pages - 1);
    }
    else {
        num_pages++;
        memset(pagebuf, 0, PAGE_SIZE);

        num_records = 0;
        memcpy(pagebuf, &num_records, sizeof(short));
        free_space = PAGE_SIZE-PAGE_HEADER_SIZE;
        memcpy(pagebuf + 2, &free_space, sizeof(short));
        int reserved_space = PAGE_SIZE - PAGE_HEADER_SIZE - free_space; 
        memcpy(pagebuf + 4, &reserved_space, 4);
       
        writeRecToPagebuf(pagebuf, recordbuf);
        writePage(fp, pagebuf, num_pages - 1);

        memcpy(headerbuf, &num_pages, sizeof(short));
        writeFileHeader(fp, headerbuf);
    }
}

// 레코드의 필드명을 enum FIELD 타입의 값으로 변환시켜 준다.
// 예를 들면, 사용자가 수행한 명령어의 인자로 "NAME"이라는 필드명을 사용하였다면 
// 프로그램 내부에서 이를 NAME(=1)으로 변환할 필요성이 있으며, 이때 이 함수를 이용한다.
FIELD getFieldID(char *fieldname) {
    if (strcmp(fieldname, "ID") == 0) return ID;
    if (strcmp(fieldname, "NAME") == 0) return NAME;
    if (strcmp(fieldname, "DEPT") == 0) return DEPT;
    if (strcmp(fieldname, "YEAR") == 0) return YEAR;
    if (strcmp(fieldname, "ADDR") == 0) return ADDR;
    if (strcmp(fieldname, "PHONE") == 0) return PHONE;
    if (strcmp(fieldname, "EMAIL") == 0) return EMAIL;
    return 1;
}

// 레코드 파일의 헤더를 읽어 오거나 수정할 때 사용한다.
// 예를 들어, 새로운 #pages를 수정하기 위해서는 먼저 readFileHeader()를 호출하여 헤더의 내용을
// headerbuf에 저장한 후 여기에 #pages를 수정하고 writeFileHeader()를 호출하여 변경된 헤더를 
// 저장한다. 두 함수 모두 성공하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
int readFileHeader(FILE *fp, char *headerbuf) {
    fseek(fp, 0, SEEK_SET);
    return fread(headerbuf, FILE_HEADER_SIZE, 1, fp) == 1 ? 1 : 0;
}

int writeFileHeader(FILE *fp, const char *headerbuf) {
    fseek(fp, 0, SEEK_SET);
    return fwrite(headerbuf, FILE_HEADER_SIZE, 1, fp) == 1 ? 1 : 0;
}

// 모든 file processing operation은 C library를 사용할 것
int main(int argc, char *argv[]) {
    
	FILE *fp = fopen(argv[2], "r+b"); 
    if (!fp) {
        fp = fopen(argv[2], "w+b");
        // 새로운 파일 초기화 작업
        char headerbuf[FILE_HEADER_SIZE] = {0};
        short num_page = 1;
        memcpy(headerbuf, &num_page, sizeof(short));
        writeFileHeader(fp, headerbuf);

        // 첫 페이지 초기화 작업
        char pagebuf[PAGE_SIZE] = {0};
        writePage(fp, pagebuf, 0);
        int num_records = 0;
        memcpy(pagebuf, &num_records, sizeof(short));
        int free_space = PAGE_SIZE-PAGE_HEADER_SIZE;
        memcpy(pagebuf + 2, &free_space, sizeof(short));
        int reserved_space = PAGE_SIZE - PAGE_HEADER_SIZE - free_space; 
        memcpy(pagebuf + 4, &reserved_space, 4);
    }

    if (strcmp(argv[1], "-i") == 0) {
        STUDENT s;
        for (int i = 3; i < argc; i++) {
            char *field_name = strtok(argv[i], "=");
            char *field_value = strtok(NULL, "=");
            FIELD field = getFieldID(field_name);
            switch (field) {
                case ID: strncpy(s.id, field_value, sizeof(s.id) - 1); s.id[sizeof(s.id) - 1] = '\0'; break;
                case NAME: strncpy(s.name, field_value, sizeof(s.name) - 1); s.name[sizeof(s.name) - 1] = '\0'; break;
                case DEPT: strncpy(s.dept, field_value, sizeof(s.dept) - 1); s.dept[sizeof(s.dept) - 1] = '\0'; break;
                case YEAR: strncpy(s.year, field_value, sizeof(s.year) - 1); s.year[sizeof(s.year) - 1] = '\0'; break;
                case ADDR: strncpy(s.addr, field_value, sizeof(s.addr) - 1); s.addr[sizeof(s.addr) - 1] = '\0'; break;
                case PHONE: strncpy(s.phone, field_value, sizeof(s.phone) - 1); s.phone[sizeof(s.phone) - 1] = '\0'; break;
                case EMAIL: strncpy(s.email, field_value, sizeof(s.email) - 1); s.email[sizeof(s.email) - 1] = '\0'; break;
                default: break;
            }
        }
        insert(fp,&s);
    }
    else if (strcmp(argv[1], "-s") == 0) {
        char *field_name = strtok(argv[3], "=");
        char *field_value = strtok(NULL, "=");
        FIELD field = getFieldID(field_name);
        search(fp, field, field_value);

    } 
    else {
    }

    fclose(fp);
	return 1;
}