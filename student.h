#ifndef _STUDENT_H_
#define _STUDENT_H_

#define MAX_RECORD_SIZE	100		//id(8) + name(13) + department(16) + year(1) + address(20) + phone(15) +  email(20) + 7 delimeters
#define PAGE_SIZE		512		// 512 Bytes
#define PAGE_HEADER_SIZE	64		// 64 Bytes
#define FILE_HEADER_SIZE	16		// 16 Bytes
// 필요한 경우 'define'을 추가할 수 있음.

typedef enum {ID=0, NAME, DEPT, YEAR,  ADDR, PHONE, EMAIL} FIELD;

typedef struct _STUDENT
{
	char id[9];			// 학번
	char name[14];		// 이름
	char dept[17];		// 학과
	char year[2];		// 학년
	char addr[21];		// 주소
	char phone[16];		// 전화번호
	char email[21];		// 이메일 주소
} STUDENT;

// void add_student_record(const char *filename, STUDENT student);
// void search_student_record(const char *filename, FIELD field, const char *value);
FIELD getFieldID(char *fieldname);
void printSearchResult(const STUDENT *s, int n);

int readPage(FILE *fp, char *pagebuf, int pagenum);
int getRecFromPagebuf(const char *pagebuf, char *recordbuf, int recordnum);
void unpack(const char *recordbuf, STUDENT *s);
int writePage(FILE *fp, const char *pagebuf, int pagenum);
void writeRecToPagebuf(char *pagebuf, const char *recordbuf);
void pack(char *recordbuf, const STUDENT *s);
void search(FILE *fp, FIELD f, char *keyval);
void insert(FILE *fp, const STUDENT *s);
int readFileHeader(FILE *fp, char *headerbuf);
int writeFileHeader(FILE *fp, const char *headerbuf);

#endif
