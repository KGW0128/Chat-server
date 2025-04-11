#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <ws2tcpip.h>

#define SERVERIP   "127.0.0.1"	//서버 ip 고정
#define SERVERPORT 9000			//서버 포트번호 고정

#define BUFSIZE 4096			//버프 사이즈
#define IDSIZE 255				//아이디 사이즈
#define PWSIZE 255				//비번 사이즈
#define NICKNAMESIZE 255		//닉네임 사이즈


enum PROTOCOL//클라이언트 전용 프로토콜
{

	CHOOSE_MENU=-1,		//처음시작 메뉴
	JOIN_MENU = 1,		//회원가입 메뉴
	LOGIN_MENU,			//로그인 메뉴
	NICK_CHANGE_MENU,	//닉네임 변경 메뉴


	CHOOSE_ROOM = 1,	//로그인 후 방선택메뉴
	RESULT,				//방선택
	CATTING,			//채팅
	OUT_ROOM,			//방나가기
	EXIT,				//종료
	WHISPER,			//귓속말
	EXPULSION,			//강퇴

	ID,					//아이디 입력 구분
	PW,					//비번 입력 구분
	NICK				//닉네임 입력 구분


};


struct _PlayerInfo//클라이언트 전용 구조체
{
	SOCKET			sock;		//서버와의 개인 소캣
	SOCKADDR_IN		addr;		//서버와의 개인 addr

	PROTOCOL pro;				//프로토콜 번호
	char port[30];				//받아온 채팅방 포트번호
	char chat_ip[30];			//받아온 채팅방 ip주소
	int num;					//내가 선택한 방번호


	char id[IDSIZE];			//로그인용 아이디
	char pw[PWSIZE];			//로그인용 비번
	char nickname[NICKNAMESIZE];//로그인용 닉네임
	char chat_msg[BUFSIZE + 1];	//입력문자 저장용

	SOCKET exit_sock;			//방 나갈 때 유저 소캣 구분용

	bool out = false;			//방 나가기 확인 bool값

	int log_and_join;			//가입,로그인 메뉴 선택값 저장 함수

}info;


SOCKET sock; // 서버 통신용 소켓(tcp)
char buf[BUFSIZE + 1]; // 데이터 송수신 버퍼
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSendButton; // 보내기 버튼
HWND roomoutButton; // 나가기 버튼
HWND hEdit1, hEdit2; // 편집 컨트롤
SOCKET cat_sock; // 채팅용 소켓(udp)
HWND you_out_Botton, whisper_Botten; // 강퇴, 귓말 버튼



struct _Chat_Room_Info//채팅용 구조체
{
	char port[30];				//받아온 채팅방 포트번호
	char chat_ip[30];			//받아온 채팅방 ip주소
	int num;					//내가 선택한 방번호


	char id[IDSIZE];			//아이디
	char pw[PWSIZE];			//비번
	char nickname[NICKNAMESIZE];//채팅방 닉네임
	char chat_msg[BUFSIZE + 1];	//채팅방 메세지

	SOCKET exit_sock;			//유저 소캣 구분용

	bool room_in = false;		//방 입장 확인 bool값
	bool room_out = false;		//방 나가기 확인 bool값
	bool exit = false;			//클라 종료 확인 bool 값

	bool whis = false;			//귓말
	bool exp = false;			//강퇴

	char  others_nickname[NICKNAMESIZE];

}c_room_info;




enum SERVER_PROTOCOL//서버에게 보낼 프로토콜
{
	JOIN_INFO,				//회원가입 요청
	LOGIN_INFO,				//로그인 요청
	JOIN_RESULT,			//회원가입 결과
	LOGIN_RESULT,			//로그인 결과
	LOGOUT,					//종료 요청
	LOGOUT_RESULT,			//종료 결과
	ROOM_INFO,				//방 목록 요청
	NICKNAME_CHANGE_INFO,	//닉네임 변경 요청
	NICKNAME_CHANGE_RESULT,	//닉네임 변경 결과
	ROOM_NUM,				//방 정보 요청
	ROOM_OUT_ME				//방 나가기 요청
};


enum SERVER_RESULT//서버에게 받아온 정보 구분 프로토콜
{
	NODATA = -1,				//빈값
	ID_EXIST = 1,				//아이디 곂침
	ID_ERROR,					//아이디 에러
	PW_ERROR,					//비번 에러
	JOIN_SUCCESS,				//회원가입 성공
	LOGIN_SUCCESS,				//로그인 성공
	LOGOUT_SUCCESS,				//종료 성공
	NICKNAME_CHANGE_SUCCESS		//닉네임 변경 성공


	,CNT_ACOOUNT				//이미 연결된 계정(에러)
};




// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);
//채팅쓰레드
DWORD WINAPI GUI_Recv_Client(LPVOID arg);


bool PacketRecv(SOCKET _sock, char* _buf);
void Login_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, int& _size);
void Join_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, char* _str3, int& _size);
void UnPackPacket(char* _buf, SERVER_RESULT& _result, char* _str1);

void ROOM_CHOOES_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, int& _size);
void ROOM_NUM_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _num, char* _str1, char* _str2, int& _size);

void Room_UnPackPacket(char* _buf, char* _str1, char* _str2, char* _str3);

void ROOM_OUT_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, SOCKET sock, char* _str1,int& _size);



//메인
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) return 1;


	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch(uMsg)
	{
	case WM_INITDIALOG://대화상자를 생성할때
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);//입력칸
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);//출력칸
		hSendButton = GetDlgItem(hDlg, IDOK);//보내기 버튼
		roomoutButton = GetDlgItem(hDlg, ROOM_OUT);//방 나가기 버튼

		you_out_Botton = GetDlgItem(hDlg, IDC_EXPULSION);//강퇴 버튼
		whisper_Botten = GetDlgItem(hDlg, IDC_WHISPER);//귓속말 버튼


		EnableWindow(roomoutButton, FALSE);//방 나가기 버튼 비활성화

		EnableWindow(you_out_Botton, FALSE);//강퇴 버튼 비활성화
		EnableWindow(whisper_Botten, FALSE);//귓말 버튼 비활성화

		//시작 메뉴 출력
		DisplayText("<<메뉴>>\n");
		DisplayText("1. 회원가입\n");
		DisplayText("2. 로그인\n");
		DisplayText("3. 닉네임 변경\n");
		DisplayText("선택: ");


		info.pro = CHOOSE_MENU;//메뉴 선택 enum으로 설정
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;


	case WM_COMMAND://버튼 입력 확인
		switch(LOWORD(wParam))
		{
		
		case IDOK://보내기 일 때
			if (info.pro == OUT_ROOM)//enum이 나가기 였다면
			{
				info.pro = CHOOSE_ROOM;//다시 방선택 enum으로 설정
			}

			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE+1);

			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			SetWindowText(hEdit1, "");//입력칸 비우기
			SetFocus(hEdit1);//입력칸 포커스
			SendMessage(hEdit1, EM_SETSEL, 0, -1);	
			return TRUE;


		case IDC_WHISPER://귓속말
			info.pro = WHISPER;
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;


		case IDC_EXPULSION://강퇴
			info.pro = EXPULSION;
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;



		case ROOM_OUT://방나가기
			info.pro = OUT_ROOM;//enum을 방나가기로 설정
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetWindowText(hSendButton, "재입장");//보내기버튼을 재입장으로 변경
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			info.out = false;//방 나가기 확인 bool값 fasle		
			return TRUE;


		case IDCANCEL://종료
			info.pro = EXIT;//enum을 종료로 설정
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			EndDialog(hDlg, IDCANCEL);//프로시저 닫기
			return TRUE;


		}
		return FALSE;
	}
	return FALSE;
}




// 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");






//---------------멀티 캐스트 용
	
	SOCKADDR_IN baindaddr;// bind 구조체 초기화

	SOCKADDR_IN chataddr;// 소켓 주소 구조체 초기화
	
	struct ip_mreq mreq;// 멀티캐스트 그룹 가입
	
	HANDLE hThread;// 채팅recvfrom 쓰레드


	// 채팅용 소켓 새로 생성
	cat_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (cat_sock == INVALID_SOCKET) err_quit("socket()");

	//리시버를 여러개 열어놓음
	BOOL optval = TRUE;
	retval = setsockopt(cat_sock, SOL_SOCKET,
		SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	int ttl = 2;//라우터를 넘어갈 수 있는 갯수(2== 1개 넘어감)
	retval = setsockopt(cat_sock, IPPROTO_IP, IP_MULTICAST_TTL,
		(char*)&ttl, sizeof(ttl));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");



	char buf[BUFSIZE];
	int size;
	SERVER_RESULT result;
	SERVER_PROTOCOL protocol;

	int pot;//문자열로 받은 포트번호 변환용 int


	// 서버와 데이터 통신
	while(1)
	{
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

		////종료,방 나가기, 재입장 일때
		//if (info.pro == OUT_ROOM || info.pro == EXIT|| info.pro == CHOOSE_ROOM||(info.pro == WHISPER&& c_room_info.exp==false )||(info.pro == EXPULSION && c_room_info.whis==false))
		//{
		//	//빈값이여도 통과를 위한 데이터 입력 저장
		//	strcpy(info.chat_msg,"pass");
		//}


		//// 문자열 길이가 0이면 보내지 않음
		//if (strlen(info.chat_msg) == 0)
		//{
		//	EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		//	SetEvent(hReadEvent); // 읽기 완료 알리기
		//	continue;
		//}


		switch (info.pro)//포트번호 구분
		{

		case CHOOSE_MENU://첫 화면 메뉴 선택

			info.log_and_join = atoi(info.chat_msg);//회원가입, 로그인 선택 저장
			DisplayText("%d\r\n", info.log_and_join);//선택한 번호 출력

			if (info.log_and_join == JOIN_MENU)//회원가입 선택시
			{
				DisplayText("\n");
				DisplayText("[회원가입]\n");
				DisplayText("ID	: ");
				info.pro = ID;//프로토콜 아이디로 변경
			}
			else if(info.log_and_join == LOGIN_MENU)//로그인 선택시
			{
				DisplayText("\n");
				DisplayText("[로그인]\n");
				DisplayText("ID	: ");
				info.pro = ID;//프로토콜 아이디로 변경
			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//닉변 선택시
			{
				DisplayText("\n");
				DisplayText("[닉변]\n");
				DisplayText("ID	: ");
				info.pro = ID;//프로토콜 아이디로 변경
			}
			else//다른걸 입력했을 때
			{
				DisplayText("잘못된 입력입니다. 다시 입력해 주세요...\n");
				DisplayText("\n");
				DisplayText("<<메뉴>>\n");
				DisplayText("1. 회원가입\n");
				DisplayText("2. 로그인\n");
				DisplayText("3. 닉네임 변경\n");
				DisplayText("선택: ");

				EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
				SetEvent(hReadEvent); // 읽기 완료 알리기
				continue;
			
			}

		break;



		case ID://아이디 입력

			strcpy(info.id, info.chat_msg);//아이디 저장
			DisplayText("%s\n", info.id);
			DisplayText("PW	: ");

			info.pro = PW;//프로토콜 비번으로 변경


			break;



		case PW://비번 입력

			strcpy(info.pw, info.chat_msg);//비번 저장

			if (info.log_and_join == JOIN_MENU)//회원가입 선택시
			{
				DisplayText("%s\n", info.pw);
				DisplayText("NickName	: ");

				info.pro = NICK;//프로토콜 닉네임으로 변경
			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//닉네임 변경 선택시
			{
				DisplayText("%s\n", info.pw);
				DisplayText("NickName	: ");

				info.pro = NICK;//프로토콜 닉네임으로 변경
			}
			else if (info.log_and_join == LOGIN_MENU)//로그인 선택시
			{
				DisplayText("%s\n", info.pw);


				//로그인 정보 저장
				Login_PackPacket(buf, LOGIN_INFO, info.id, info.pw, size);

				//로그인 정보 보내기
				retval = send(sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send()");
					break;
				}

				//로그인 결과 받기
				if (!PacketRecv(sock, buf))
				{
					break;
				}

				//분리
				memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
				memset(info.chat_msg, 0, sizeof(info.chat_msg));
				UnPackPacket(buf, result, info.chat_msg);
				

				//결과 출력
				DisplayText("%s\n", info.chat_msg);
				DisplayText("\n");

				switch (result)//enum 구분
				{
				case ID_ERROR://아이디 에러
				case PW_ERROR://비번 에러
				case CNT_ACOOUNT://이미 접속계정 에러

					//초기 메뉴 다시 출력
					DisplayText("<<메뉴>>\n");
					DisplayText("1. 회원가입\n");
					DisplayText("2. 로그인\n");
					DisplayText("3. 닉네임 변경\n");
					DisplayText("선택: ");

					info.pro = CHOOSE_MENU;//메뉴 선택 enum으로 설정
					break;


				case LOGIN_SUCCESS://로그인 성공
					
					//방 목록 메뉴 보내달라고 서버에게 요청
					ROOM_CHOOES_PackPacket(buf, ROOM_INFO, size);
					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
						break;
					}

					//방 목록 메뉴 메세지 받아옴
					if (!PacketRecv(sock, buf))
					{
						break;
					}

					//분리 후 출력
					memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
					memset(info.chat_msg, 0, sizeof(info.chat_msg));
					UnPackPacket(buf, result, info.chat_msg);
					DisplayText("%s\n", info.chat_msg);//방 메뉴 출력


					info.pro = RESULT;//방 번호 선택으로 이동
					break;

				}
			}


			break;



		case NICK://닉네임 입력

			strcpy(info.nickname, info.chat_msg);//닉네임 저장
			DisplayText("%s\n", info.nickname);


			if (info.log_and_join == JOIN_MENU)//회원가입 선택시
			{
				//회원가입 정보 저장
				Join_PackPacket(buf, JOIN_INFO, info.id, info.pw, info.nickname, size);

			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//닉네임 변경 선택시
			{
				//닉변 정보 저장
				Join_PackPacket(buf, NICKNAME_CHANGE_INFO, info.id, info.pw, info.nickname, size);
			}

			//정보 보내기
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			//결과 받아오기
			if (!PacketRecv(sock, buf))
			{
				break;
			}

			//분해
			memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
			memset(info.chat_msg, 0, sizeof(info.chat_msg));
			UnPackPacket(buf, result, info.chat_msg);

			//결과 출력
			DisplayText("%s\n", info.chat_msg);
			DisplayText("\n");

			//초기메뉴 출력
			DisplayText("<<메뉴>>\n");
			DisplayText("1. 회원가입\n");
			DisplayText("2. 로그인\n");
			DisplayText("3. 닉네임 변경\n");
			DisplayText("선택: ");

			info.pro = CHOOSE_MENU;//메뉴 선택 enum으로 설정

			break;



		case CHOOSE_ROOM://메뉴선택

			SetWindowText(hSendButton, "보내기");//보내기 버튼 텍스트 보내기로 변경
			
			//방 목록 메뉴 보내달라고 서버에게 요청
			ROOM_CHOOES_PackPacket(buf, ROOM_INFO, size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			if (!PacketRecv(sock, buf))//방 목록 메뉴 메세지 받아옴
			{
				break;
			}

			//분리 후 출력
			memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
			memset(info.chat_msg, 0, sizeof(info.chat_msg));
			UnPackPacket(buf, result, info.chat_msg);
			DisplayText("%s\n", info.chat_msg);//방 메뉴 출력

			c_room_info.exp = false;

			info.pro = RESULT;//방 번호 선택으로 이동
			break;


		case RESULT://방선택
			
			//입력받은 방번호 받아옴
			info.num = atoi(info.chat_msg);

			//재대로 방을 선택하지 않았다면
			if (info.num < 1 || info.num>3)
			{
				DisplayText("방번호를 다시 입력하세요.\n");
				break;
			}

			//방 선택 정보 저장
			ROOM_NUM_PackPacket(buf,  ROOM_NUM, info.chat_msg, info.id, info.pw, size);

			//정보 보내기
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			//방 목록 메뉴 메세지 받아옴
			if (!PacketRecv(sock, buf))
			{
				break;
			}

			//분해
			Room_UnPackPacket(buf, info.port, info.chat_ip, info.nickname);

			//각 정보들 저장
			strcpy(c_room_info.port, info.port);
			strcpy(c_room_info.chat_ip, info.chat_ip);
			strcpy(c_room_info.nickname, info.nickname);


			//문자열로 받은 포트번호를 정수로 전환
			pot = atoi(info.port);
			

			//채팅방연결	
			ZeroMemory(&baindaddr, sizeof(baindaddr));
			baindaddr.sin_family = AF_INET;
			baindaddr.sin_addr.s_addr = htonl(INADDR_ANY);//내주소
			baindaddr.sin_port = htons(pot);//받아온 채팅방 소켓
			//binding
			retval = bind(cat_sock, (SOCKADDR*)&baindaddr, sizeof(baindaddr));
			if (retval == SOCKET_ERROR) err_quit("bind()");


			//채팅용 addr 생성
			ZeroMemory(&chataddr, sizeof(chataddr));
			chataddr.sin_family = AF_INET;
			chataddr.sin_addr.s_addr = inet_addr(info.chat_ip);//받아온 채팅방 주소
			chataddr.sin_port = htons(pot);//받아온 채팅방 소켓

			// 멀티캐스트 그룹 가입
			mreq.imr_multiaddr.s_addr = inet_addr(info.chat_ip);//가입할 주소
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);//내주소
			retval = setsockopt(cat_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,//가입
				(char*)&mreq, sizeof(mreq));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");

			//// 입장문 출력
			DisplayText("%s번방에 입장했습니다.\r\n", info.chat_msg);

			c_room_info.exit_sock = cat_sock; //자신 소캣 저장

			//gui recv용 쓰레드 생성
			hThread = CreateThread(NULL, 0, GUI_Recv_Client, (LPVOID)cat_sock, 0, NULL);


			EnableWindow(roomoutButton, TRUE); // 보내기 버튼 활성화
			EnableWindow(you_out_Botton, TRUE);//강퇴 버튼 활성화
			EnableWindow(whisper_Botten, TRUE);//귓말 버튼 활성화

			info.pro = CATTING;//채팅중 enum으로 변경

			break;


		case CATTING://채팅중
			
			//채팅내용 저장
			strcpy(c_room_info.chat_msg, info.chat_msg);



			// 채팅 내용 보내기
			retval = sendto(cat_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
				(SOCKADDR*)&chataddr, sizeof(chataddr));
			if (retval == SOCKET_ERROR) {
				err_display("sendto()");
				break;
			}


			c_room_info.whis = false;
			c_room_info.exp == false;

			break;



		case WHISPER://귓말

			if (c_room_info.whis == false)
			{
				DisplayText("귓속말 대상의 닉네임을 적으세요.\n");
				c_room_info.whis = true;
				info.pro = WHISPER;
				break;
				
			}
			else if (c_room_info.whis == true)
			{
				strcpy(c_room_info.others_nickname, info.chat_msg);//닉네임 저장
				DisplayText("전달할 내용을 입력하세요.\n");
				info.pro = CATTING;
				break;
			}

			break;



		case EXPULSION: //강퇴
			if (c_room_info.exp == false)
			{
				DisplayText("강퇴할 대상의 닉네임을 적으세요.\n");
				c_room_info.exp = true;
				info.pro = EXPULSION;
				break;
				
			}
			if (c_room_info.exp == true)
			{
				strcpy(c_room_info.others_nickname, info.chat_msg);//닉네임 저장

				retval = sendto(cat_sock, (char*)&c_room_info, sizeof(_PlayerInfo), 0,
					(SOCKADDR*)&chataddr, sizeof(chataddr));
				if (retval == SOCKET_ERROR) {
					err_display("sendto()");
					break;
				}

				c_room_info.exp = false;
				info.pro = CATTING;//채팅중 enum으로 변경			
				break;
			}

			break;




		case OUT_ROOM://방나가기

	
			// 서버한테 나간다고 보내기
			ROOM_OUT_PackPacket(buf, ROOM_OUT_ME, c_room_info.exit_sock ,info.nickname,size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}


			//gui recv용 쓰레드가 닫힐때 까지 대기
			WaitForSingleObject(hThread, INFINITE); 
			CloseHandle(hThread);//gui recv용 쓰레드 종료

			//닫힘 확인 문자 출력
			DisplayText("\r\n채팅쓰레드를 닫습니다...\r\n\r\n");

			//채팅 소켓 종료
			closesocket(cat_sock);

			//채팅 소켓 재생성
			cat_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (cat_sock == INVALID_SOCKET) err_quit("socket()");
			
			//리시버를 여려개 엶 재설정
			retval = setsockopt(cat_sock, SOL_SOCKET,
				SO_REUSEADDR, (char*)&optval, sizeof(optval));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");

			//라우터 넘어가기 재설정
			retval = setsockopt(cat_sock, IPPROTO_IP, IP_MULTICAST_TTL,
				(char*)&ttl, sizeof(ttl));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");


			// 나가기 버튼 비활성화
			EnableWindow(roomoutButton, FALSE); 
			
			EnableWindow(you_out_Botton, FALSE);//방 나가기 버튼 비활성화
			EnableWindow(whisper_Botten, FALSE);//방 나가기 버튼 비활성화

			SetWindowText(roomoutButton, "방나가기");//보내기버튼을 재입장으로 변경

			
			//재입장을 위한 안내 문자열 출력
			DisplayText("재입장을 원하시면 확인 버튼을 누르세요.\r\n");
			


			break;


		case EXIT://종료


			//종료 정보 보내기
			ROOM_CHOOES_PackPacket(buf, LOGOUT, size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}


			//서버가 종료룰 인지했는지 확인을 위한 recv
			if (!PacketRecv(sock, buf))
			{
				break;
			}


			break;


		default:
			break;

		}

		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		
		SetEvent(hReadEvent); // 읽기 완료 알리기
	}

	return 0;
}

DWORD WINAPI GUI_Recv_Client(LPVOID arg)//채팅쓰레드
{
	//쓰레드 전용 함수
	SOCKET client_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	_Chat_Room_Info chat_info;//recv용

	// 클라이언트 정보 얻기
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);//이 소켓과 연결된 상대방 주소를 연결

	while (1)
	{
		//서버에게서 방 선택 메세지 받아옴
		int len = sizeof(clientaddr);
		retval = recvfrom(client_sock, (char*)&chat_info, sizeof(_Chat_Room_Info), 0,
			(SOCKADDR*)&clientaddr, &len);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvfrom()");

		}

	

		if (chat_info.room_out == true)//받은 메세지가 종료이면
		{

			if (client_sock == chat_info.exit_sock)//받은 소캣이 보낸 소캣이랑 같다면
			{
				//본인이 나갈경우 받지 않음
				break;
			}
			else//소켓이 서로 다르다면
			{
				// 받은 데이터 출력
				DisplayText("%s%s\r\n", chat_info.nickname, chat_info.chat_msg);

			}
		}
		else if (chat_info.exp == true)//강퇴이면
		{
			if (strcmp(chat_info.others_nickname, info.nickname) == 0)
			{
				DisplayText("방장으로부터 강퇴당하셨습니다.\r\n");


				EnableWindow(you_out_Botton, FALSE);//방 나가기 버튼 비활성화
				EnableWindow(whisper_Botten, FALSE);//방 나가기 버튼 비활성화
				EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화

				SetWindowText(roomoutButton, "확인");//보내기버튼을 재입장으로 변경


				DisplayText("확인 버튼을 누르세요.\r\n");

			}
			else//소켓이 서로 다르다면
			{
				// 받은 데이터 출력
				DisplayText("%s님이 방장에 의해 강퇴당하셨습니다.\r\n", chat_info.others_nickname);
			}

		}
		else//받은 메세지가 종료가 아니면
		{

			if (chat_info.whis == true)
			{

				if (strcmp(chat_info.others_nickname, info.nickname) == 0)
				{
					DisplayText("[%s님으로 부터의 귓속말]: %s\r\n", chat_info.nickname, chat_info.chat_msg);
					chat_info.whis = false;
				}


			}
			else//일반 채팅
			{
				if (chat_info.room_in == true)//받은 메세지가 입장문이면
				{
					DisplayText("%s\r\n", chat_info.chat_msg);
				}
				else
				{
					DisplayText("[%s]: %s\r\n", chat_info.nickname, chat_info.chat_msg);
				}
			}

		}
	}

	return 0;
}





// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	if (info.pro == CATTING)
	{
		SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);//true일때 기존 내용 지우기,false는 보존

	}
	else
	{
		SendMessage(hEdit2, EM_REPLACESEL, TRUE, (LPARAM)cbuf);//true일때 기존 내용 지우기,false는 보존

	}
	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}


//tcp recv
bool PacketRecv(SOCKET _sock, char* _buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size, sizeof(size), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, _buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;

	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}



//로그인 패킷
void Login_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, int& _size)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int strsize2 = strlen(_str2);

	_size = 0;

	ptr = ptr + sizeof(_size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	_size = _size + sizeof(_protocol);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	_size = _size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	_size = _size + strsize1;

	memcpy(ptr, &strsize2, sizeof(strsize2));
	ptr = ptr + sizeof(strsize2);
	_size = _size + sizeof(strsize2);

	memcpy(ptr, _str2, strsize2);
	ptr = ptr + strsize2;
	_size = _size + strsize2;

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(_size));

	_size = _size + sizeof(_size);
}



//회원가입 패킷
void Join_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, char* _str3, int& _size)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int strsize2 = strlen(_str2);
	int strsize3 = strlen(_str3);

	_size = 0;

	ptr = ptr + sizeof(_size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	_size = _size + sizeof(_protocol);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	_size = _size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	_size = _size + strsize1;

	memcpy(ptr, &strsize2, sizeof(strsize2));
	ptr = ptr + sizeof(strsize2);
	_size = _size + sizeof(strsize2);

	memcpy(ptr, _str2, strsize2);
	ptr = ptr + strsize2;
	_size = _size + strsize2;

	memcpy(ptr, &strsize3, sizeof(strsize3));
	ptr = ptr + sizeof(strsize3);
	_size = _size + sizeof(strsize3);

	memcpy(ptr, _str3, strsize3);
	ptr = ptr + strsize3;
	_size = _size + strsize3;

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(_size));

	_size = _size + sizeof(_size);
}


//결과 받는 패킷
void UnPackPacket(char* _buf, SERVER_RESULT& _result, char* _str1)
{
	int strsize1;

	char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_result, ptr, sizeof(_result));
	ptr = ptr + sizeof(_result);

	memcpy(&strsize1, ptr, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);

	memcpy(_str1, ptr, strsize1);
	ptr = ptr + strsize1;
}


//선택한 방정보 받는 패킷
void Room_UnPackPacket(char* _buf, char* _str1, char* _str2, char* _str3)
{
	int str1size, str2size, str3size;
	char* ptr = _buf;
	
	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;

	memcpy(&str2size, ptr, sizeof(str2size));
	ptr = ptr + sizeof(str2size);

	memcpy(_str2, ptr, str2size);
	ptr = ptr + str2size;

	memcpy(&str3size, ptr, sizeof(str3size));
	ptr = ptr + sizeof(str3size);

	memcpy(_str3, ptr, str3size);
	ptr = ptr + str3size;
}



//방 선택 메뉴 요청 패킷
void ROOM_CHOOES_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, int& _size)
{
	char* ptr = _buf;
	_size = 0;

	ptr = ptr + sizeof(_size);
	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	_size = _size + sizeof(_protocol);

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(_size));

	_size = _size + sizeof(_size);
}


//방 선택한 번호 보내는 패킷
void ROOM_NUM_PackPacket(char* _buf,SERVER_PROTOCOL _protocol, char* _num, char* _str1, char* _str2, int& _size)
{

	int strsize1 = strlen(_num);

	int strsize2 = strlen(_str1);
	int strsize3 = strlen(_str2);

	char* ptr = _buf;
	_size = 0;

	ptr = ptr + sizeof(_size);
	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	_size = _size + sizeof(_protocol);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	_size = _size + sizeof(strsize1);

	memcpy(ptr, _num, strsize1);
	ptr = ptr + strsize1;
	_size = _size + strsize1;



	memcpy(ptr, &strsize2, sizeof(strsize2));
	ptr = ptr + sizeof(strsize2);
	_size = _size + sizeof(strsize2);

	memcpy(ptr, _str1, strsize2);
	ptr = ptr + strsize2;
	_size = _size + strsize2;

	memcpy(ptr, &strsize3, sizeof(strsize3));
	ptr = ptr + sizeof(strsize3);
	_size = _size + sizeof(strsize3);

	memcpy(ptr, _str2, strsize3);
	ptr = ptr + strsize3;
	_size = _size + strsize3;


	ptr = _buf;
	memcpy(ptr, &_size, sizeof(_size));

	_size = _size + sizeof(_size);

}

//방 나가기 패킷
void ROOM_OUT_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, SOCKET sock, char* _str1, int& _size)
{
	char* ptr = _buf;
	_size = 0;
	int strsize1 = strlen(_str1);


	ptr = ptr + sizeof(_size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	_size = _size + sizeof(_protocol);

	memcpy(ptr, &sock, sizeof(sock));
	ptr = ptr + sizeof(sock);
	_size = _size + sizeof(sock);


	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	_size = _size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	_size = _size + strsize1;

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(_size));

	_size = _size + sizeof(_size);


}