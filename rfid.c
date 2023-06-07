#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "RC522.h"

#define LCD_ADDR 0x27
#define ONE_DAY	86400
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
	char memberId[10];
	int borrowed_book;
	time_t borrowed_time;
} Member;

int fd;
int BLEN=1;

void write_word(int data){
    int temp = data;
    if ( BLEN == 1 )
        temp |= 0x08;
    else
        temp &= 0xF7;
    wiringPiI2CWrite(fd, temp);
}

void send_command(int comm){
    int buf;
    // Send bit7-4 firstly
    buf = comm & 0xF0;
    buf |= 0x04;                    // RS = 0, RW = 0, EN = 1
    write_word(buf);
    delay(2);
    buf &= 0xFB;                    // Make EN = 0
    write_word(buf);

    // Send bit3-0 secondly
    buf = (comm & 0x0F) << 4;
    buf |= 0x04;                    // RS = 0, RW = 0, EN = 1
    write_word(buf);
    delay(2);
    buf &= 0xFB;                    // Make EN = 0
    write_word(buf);
}

void send_data(int data){
    int buf;
    // Send bit7-4 firstly
    buf = data & 0xF0;
    buf |= 0x05;                    // RS = 1, RW = 0, EN = 1
    write_word(buf);
    delay(2);
    buf &= 0xFB;                    // Make EN = 0
    write_word(buf);

    // Send bit3-0 secondly
    buf = (data & 0x0F) << 4;
    buf |= 0x05;                    // RS = 1, RW = 0, EN = 1
    write_word(buf);
    delay(2);
    buf &= 0xFB;                    // Make EN = 0
    write_word(buf);
}

void init(){
    send_command(0x33);     // Must initialize to 8-line mode at first
    delay(5);
    send_command(0x32);     // Then initialize to 4-line mode
    delay(5);
    send_command(0x28);     // 2 Lines & 5*7 dots
    delay(5);
    send_command(0x0C);     // Enable display without cursor
    delay(5);
    send_command(0x01);     // Clear Screen
    wiringPiI2CWrite(fd, 0x08);
}

void clear(){
    send_command(0x01);     //clear Screen
}

void write_lcd(int x, int y, char data[]){
    int addr, i;
    int tmp;
    if (x < 0)  x = 0;
    if (x > 15) x = 15;
    if (y < 0)  y = 0;
    if (y > 1)  y = 1;

    // Move cursor
    addr = 0x80 + 0x40 * y + x;
    send_command(addr);

    tmp = strlen(data);
    for (i = 0; i < tmp; i++){
        send_data(data[i]);
    }
}

int find_member(Member members[], size_t n_members, const char *id) {
	for (int i = 0; i < n_members; i++) {
		if (strcmp(members[i].memberId, id) == 0) {
			return (int)i;
		}
	}
	return -1;
}

const char* readRFID ( void )
{
	char cStr [ 30 ];
	static uint8_t ucArray_ID [ 4 ]; // 카드 ID
	uint8_t ucStatusReturn; // 카드 검색 상태

	while ( 1 ) { // 무한 카드 검색
		/* PICC_REQALL : 모든 종류의 카드 검색 */
		if (( ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID )) != MI_OK ) { // 실패할 경우 다시 카드 검색
			ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID ); 
		}

		if (ucStatusReturn == MI_OK) { // 카드 검색 성공
			if (PcdAnticoll ( ucArray_ID ) == MI_OK) { // 카드 인식 성공
				return ucArray_ID;
			}
		}
	}
}

int main(void) {
	char server_address[] = "192.168.0.7";
	int port = 8888;

	Member members[10];
	size_t n_members = 0;
	int available_books[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
	int n_available_books = ARRAY_SIZE(available_books);
	int select_num;
	char block_num[2];

	int sock;
	struct sockaddr_in serv_addr;

	RC522_setup(7); // Pin : 7
	PcdReset(); // RC522 초기화
	M500PcdConfigISOType('A'); // ISO14443A 타입으로 설정

	fd = wiringPiI2CSetup(LCD_ADDR);
	init();

	// sock = socket(PF_INET, SOCK_STREAM, 0);
	// if (sock == -1) {
	// 	printf("🚫 소켓 생성 실패\r\n");
	// 	exit(1);
	// }

	// memset(&serv_addr, 0, sizeof(serv_addr));
	// serv_addr.sin_family = AF_INET;
	// serv_addr.sin_addr.s_addr = inet_addr(server_address);
	// serv_addr.sin_port = htons(port);

	// if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
	// 	printf("🚫 서버 연결 실패\r\n");
	// 	exit(1);
	// }
	
	while(1) {
		printf("\n");
		printf("📚 스마트 도서관에 오신 것을 환영합니다 !\n");
		printf("😃 회원증을 인식해주세요. . .\r\n");
		clear();
		write_lcd(0, 0, "Welcome to      ");
		write_lcd(0, 1, "Smart Library ! ");
		delay(2000);
		clear();
		write_lcd(0, 0, "Please scan your");
		write_lcd(0, 1, "membership card");
		// 회원번호 출력
		const char* id = readRFID();
		
		int member_index = find_member(members, n_members, id);
		if (member_index == -1) {
			printf("🚫 등록되지 않은 회원입니다.\r\n");
			clear();
			write_lcd(0, 0, "You are not a   ");
			write_lcd(0, 1, "member.         ");
			delay(1000);			
			write_lcd(0, 0, "Do you wanna   ");
			write_lcd(0, 1, "sign up? Y:1 N:2");
			printf("📝 회원 가입을 하시겠습니까?.\r\n");
			printf("1. 예 2. 아니오\r\n");
			scanf("%d", &select_num);
			if (select_num == 1) {			
				printf("✅ 회원 가입이 완료되었습니다.\r\n");
				clear();
				write_lcd(0, 0, "Sign up complete");
				strcpy(members[n_members].memberId, id); // 회원번호 저장
				members[n_members].borrowed_book = 0;
				members[n_members].borrowed_time = 0;
				n_members++;
				delay(3000);
				clear();
			} else {
				printf("😃 이용해주셔서 감사합니다.\r\n");
				clear();
				write_lcd(0, 0, "Thank you for   ");
				write_lcd(0, 1, "using.          ");
				delay(3000);
				clear();
			}
			continue;
		}
		else if (member_index >= 0) {
			printf("😃 환영합니다 !!\r\n");
			clear();
			write_lcd(0, 0, "Welcome !!      ");
			delay(1000);
			clear();
			printf("--------------------------------\r\n");
			printf("ℹ️   회원번호 : %02X%02X%02X%02X\r\n", id[0], id[1], id[2], id[3]);
			if (members[member_index].borrowed_book != 0) {
				printf("ℹ️   대여 중인 도서는 %d번 도서입니다.\r\n", members[member_index].borrowed_book);
			} else {
				printf("ℹ️   대여 중인 도서가 없습니다.\r\n");
			}
			if (members[member_index].borrowed_time != 0) {
				printf("ℹ️   현재 대여 기간은 %d일입니다.\r\n", (int)(time(NULL) - members[member_index].borrowed_time) / ONE_DAY);
			}
	
			printf("--------------------------------\r\n");
			printf("ℹ️   대여는 1번, 반납은 2번을 눌러주세요.\r\n");
			write_lcd(0, 0, "1. Borrow       ");
			write_lcd(0, 1, "2. Return       ");
			scanf("%d", &select_num);
			if (select_num == 1) {
				if (n_available_books == 0) {
					printf("🚫 대여 가능한 도서가 없습니다.\r\n");
					continue;
				}
				printf("📝 대여할 도서 번호를 입력해주세요.\r\n");
				clear();
				write_lcd(0, 0, "Enter the number");
				write_lcd(0, 1, "of the book.    ");
				scanf("%d", &select_num);
				if (select_num < 1 || select_num > 8) {
					printf("🚫 잘못된 도서 번호입니다.\r\n");
					continue;
				}
				if (available_books[select_num - 1] == 0) {
					printf("🚫 이미 대출중인 도서입니다.\r\n");
					continue;
				}
				if (select_num >= 1 && select_num <= 4) {
					printf("❗️ 1️번 블록을 개방합니다.\r\n");
					// write(sock, "1", 1);
					clear();
					write_lcd(0, 0, "Block 1 is open ");
					delay(5000);
				}
				else if (select_num >= 5 && select_num <= 8) {
					printf("❗️ 2️번 블록을 개방합니다.\r\n");
					// write(sock, "2", 1);
					clear();
					write_lcd(0, 0, "Block 2 is open ");
					delay(5000);
				}
				available_books[select_num - 1] = 0; // 대출중으로 변경
				printf("✅ 대여가 완료되었습니다.\r\n");
				clear();
				write_lcd(0, 0, "Borrow complete ");
				members[member_index].borrowed_book = select_num;
				members[member_index].borrowed_time = time(NULL);
				delay(3000);
			} else if (select_num == 2) {
				if (members[member_index].borrowed_book == 0) {
					printf("🚫 대여 중인 도서가 없습니다.\r\n");
					continue;
				}
				int return_book_num = members[member_index].borrowed_book;
				printf("📝 반납할 도서 번호는 %d번 도서입니다.\r\n", return_book_num);
				delay(1000);
				if(return_book_num >= 1 && return_book_num <= 4) {
					printf("❗️ 1️번 블록을 개방합니다.\r\n");
					// write(sock, "1", 1);
					clear();
					write_lcd(0, 0, "Block 1 is open ");
					delay(5000);
				}
				else if(return_book_num >= 5 && return_book_num <= 8) {
					printf("❗️ 2️번 블록을 개방합니다.\r\n");
					// write(sock, "2", 1);
					clear();
					write_lcd(0, 0, "Block 2 is open ");
					delay(5000);
				}

				available_books[return_book_num - 1] = 1; // 반납으로 변경
				members[member_index].borrowed_book = 0;
				// 7일 연체 시 연체한 날짜만큼 제한
				if ((int)(time(NULL) - members[member_index].borrowed_time) / ONE_DAY > 7) {
					printf("🚫 연체로 인해 7일간 대출이 제한됩니다.\r\n");
					delay(3000);
				}
				members[member_index].borrowed_time = 0;
				printf("✅ 반납이 완료되었습니다.\r\n");
				clear();
				write_lcd(0, 0, "Return complete ");
				delay(3000);
			} else {
				printf("🚫 잘못된 입력입니다.\r\n");
				delay(3000);
				continue;
			}		
		}	
	}
}