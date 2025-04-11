/*
 * echo_selserv_win.c
 * Written by SW. YOON
 */

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define BUFSIZE 4096			//버프 사이즈
#define IDSIZE 255				//아이디 사이즈
#define PWSIZE 255				//비번 사이즈
#define NICKNAMESIZE 255		//닉네임 사이즈
#define ERROR_DISCONNECTED -2	//에러끊김 확인
#define DISCONNECTED -1			//끓김 확인
#define SOC_TRUE 1				//참
#define SOC_FALSE 0				//거짓


//결과 문자들
#define ID_ERROR_MSG "없는 아이디입니다.\n"
#define PW_ERROR_MSG "비밀번호가 틀렸습니다.\n"
#define LOGIN_SUCCESS_MSG "로그인에 성공했습니다.\n"
#define ID_EXIST_MSG "이미 있는 아이디 입니다.\n"
#define JOIN_SUCCESS_MSG "가입에 성공했습니다.\n"
#define LOGOUT_MSG "로그아웃되었습니다.\n"
#define NICKNAME_CHANGE_SUCCESS_MSG "닉네임이 변경되었습니다.\n"
#define ROOM_CHISE_MSG "채팅방을 선택하세요.\r\n1번방\r\n2번방\r\n3번방"
#define ROOM_IN_MSG "님이 입장했습니다.\r\n"
#define ROOM_OUT_MSG "님이 퇴장했습니다.\r\n"
#define CNT_ACOOUNT_MSG "이미 접속된 계정입니다.\n"


enum STATE //서버전용 state구분 프로토콜
{
	NO_STATE = -1,
	INIT_STATE=1, 
	MENU_SELECT_STATE, 	
	LOGIN_STATE,	
	SEND_DELAY_STATE,
	DISCONNECTED_STATE
};



enum PROTOCOL//클라에게서 받은 요청 프로토콜 구분
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


enum RESULT//결과 전송을 위한 프로토콜 구분
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


struct _User_Info//유저 정보 구조체
{
	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];
};

struct ROOM_IP//방정보 전달을 위한 정보 구조체
{
	char port[30];				// 채팅방 포트번호
	char chat_ip[30];			// 채팅방 ip주소
	char name[NICKNAMESIZE];	//닉네임
	char chat_msg[BUFSIZE + 1];	//채팅방 메세지
};


struct _ClientInfo//클라 정보 구조체
{
	SOCKET		sock;
	SOCKADDR_IN addr;
	_User_Info  userinfo;
	STATE		next_state;
	STATE		state;
	bool		r_sizeflag;
	
	int			recvbytes;
	int			comp_recvbytes;
	int			sendbytes;
	int			comp_sendbytes;

	char		recvbuf[BUFSIZE];	//보내기용 버퍼
	char		sendbuf[BUFSIZE];	//받기용 버퍼

	char num[255];					//내가 선택한 방번호
	ROOM_IP room_ip;

	int retval;
	//gui의 sand용 소켓 생성
	SOCKET catroom_sock;
	//gui의 sand용 abbr 생성
	SOCKADDR_IN catroom_addr;


};


struct _Chat_Room_Info//채팅 전용 구조체
{
	char port[30];				//받아온 채팅방 포트번호
	char chat_ip[30];			//받아온 채팅방 ip주소
	int num;					//내가 선택한 방번호


	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];//채팅방 닉네임
	char chat_msg[BUFSIZE + 1];	//채팅방 메세지

	SOCKET exit_sock;			//유저 소캣 구분용

	bool room_in = false;		//방 입장 확인 bool값
	bool room_out = false;		//방 나가기 확인 bool값
	bool exit = false;			//클라 종료 확인 bool 값

	bool whis = false;			//강퇴
	bool exp = false;			//귓말

	char  others_nickname[NICKNAMESIZE];

}c_room_info;



_ClientInfo* User_List[100];//유저 구조체 배열
int Count = 0;//유저 카운트

_User_Info* Join_List[100];//가입자 구조체 배열
int Join_Count = 0;//가입유저 카운트

FD_SET Rset, Wset;//받기, 쓰기 구분

//현제 접속한 유저 확인용
char connected_ids[100][IDSIZE]; // 유저 아이디 배열
int connected_count = 0;

void err_quit(const char* msg);
void err_display(const char* msg);
int recvn(SOCKET s, char* buf, int len, int flags);
void GetProtocol(const char* _ptr, PROTOCOL& _protocol);
int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1);
void UnPackPacket(const char* _buf, char* _str1, char* _str2, char* _str3);
void UnPackPacket(const char* _buf, char* _str1, char* _str2);
BOOL SearchFile(const char* filename);
bool FileDataLoad();
bool FileDataAdd(_User_Info* _info);
void FileSave();
int MessageRecv(_ClientInfo* _info);
int MessageSend(_ClientInfo* _info);
int PacketRecv(_ClientInfo* _ptr);
_ClientInfo* AddClient(SOCKET _sock, SOCKADDR_IN _clientaddr);
void RemoveClient(_ClientInfo* _ptr);
void RecvPacketProcess(_ClientInfo* _ptr);
void SendPacketProcess(_ClientInfo* _ptr);
void JoinProcess(_ClientInfo* _ptr);
void LoginProcess(_ClientInfo* _ptr);
void Logoutprocess(_ClientInfo* _ptr);



void Room_chiseprocess(_ClientInfo* _ptr);
int Room_Menu_PackPacket(char* _buf, const char* _str1);
void Nickname_change_process(_ClientInfo* _ptr);
void Room_num_process(_ClientInfo* _ptr);
int IP_PackPacket(char* _buf, const  char* _str1, const char* _str2, const char* _str3);
void Room_out_process(_ClientInfo* _ptr);
void UnPackPacket(const char* _buf, SOCKET sock ,char* _str1);



//메인
int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET hServSock;
	SOCKADDR_IN servAddr;
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
	int retval;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
		err_quit("WSAStartup() error!");

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hServSock == INVALID_SOCKET)
		err_quit("socket() error");

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(9000);

	u_long on = 1;
	retval = ioctlsocket(hServSock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket()");


	if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
		err_quit("bind() error");
	if (listen(hServSock, 5) == SOCKET_ERROR)
		err_quit("listen() error");

	if (!FileDataLoad())//가입유저정보 불러오기
	{
		err_quit("file read error!");
	}

	while (1)
	{
		FD_ZERO(&Rset);
		FD_ZERO(&Wset);

		FD_SET(hServSock, &Rset);

		for (int i = 0; i < Count; i++)//현재 유저가 받기인지 쓰기인지 모두 구분
		{
			FD_SET(User_List[i]->sock, &Rset);

			if (User_List[i]->state == SEND_DELAY_STATE)
			{
				FD_SET(User_List[i]->sock, &Wset);
			}					
		}

		//소켓 에러 체크
		if (select(0, &Rset, &Wset, 0, NULL) == SOCKET_ERROR)
		{
			err_quit("select() error");
		}			

		//처음 들어왔다면 기초정보 셋팅
		if (FD_ISSET(hServSock, &Rset)) 
		{
			int clntLen = sizeof(clntAddr);
			hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &clntLen);
			_ClientInfo* ptr = AddClient(hClntSock, clntAddr);
			ptr->state = MENU_SELECT_STATE;
			continue;
		}		



		//모든 유저 회전
		for (int i = 0; i < Count; i++)
		{
			_ClientInfo* ptr = User_List[i];

			if (FD_ISSET(ptr->sock, &Rset))//받기
			{				
				int result = PacketRecv(ptr);

				switch (result)
				{
				case DISCONNECTED:
					ptr->state = DISCONNECTED_STATE;
					break;
				case SOC_FALSE:
					continue;
				case SOC_TRUE:
					break;
				}

				RecvPacketProcess(ptr);
			}					

			if (FD_ISSET(ptr->sock, &Wset))//보내기
			{
				int result = MessageSend(ptr);
				switch (result)
				{
				case ERROR_DISCONNECTED:
					err_display("connect end");
				case DISCONNECTED:
					ptr->state = DISCONNECTED_STATE;
					break;
				case SOC_FALSE:
					continue;
				case SOC_TRUE:
					break;
				}

				SendPacketProcess(ptr);				
			}

			//종료일 때
			if (ptr->state == DISCONNECTED_STATE)
			{
				RemoveClient(ptr);
				i--;
				continue;
			}

		}
	}	


	//서버 종료시 가입유저정보 저장
	FileSave();

	closesocket(hServSock);
	WSACleanup();
	return 0;
}


void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
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

//클라에서 받아온 프로토콜 분해
void GetProtocol(const char* _ptr, PROTOCOL& _protocol)
{
	memcpy(&_protocol, _ptr, sizeof(PROTOCOL));

}

//결과 보내기용 패킷
int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &_result, sizeof(_result));
	ptr = ptr + sizeof(_result);
	size = size + sizeof(_result);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

//방 메뉴 패킷
int Room_Menu_PackPacket(char* _buf, const char* _str1)
{

	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);


	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;

}

//방정보 보내기용 패킷
int IP_PackPacket(char* _buf, const char* _str1, const char* _str2, const char* _str3)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int strsize2 = strlen(_str2);
	int strsize3 = strlen(_str3);

	int size = 0;

	ptr = ptr + sizeof(size);


	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	memcpy(ptr, &strsize2, sizeof(strsize2));
	ptr = ptr + sizeof(strsize2);
	size = size + sizeof(strsize2);

	memcpy(ptr, _str2, strsize2);
	ptr = ptr + strsize2;
	size = size + strsize2;

	memcpy(ptr, &strsize3, sizeof(strsize3));
	ptr = ptr + sizeof(strsize3);
	size = size + sizeof(strsize3);

	memcpy(ptr, _str3, strsize3);
	ptr = ptr + strsize3;
	size = size + strsize3;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);
	return size;
}

//회원가입&닉변 정보 분해 패킷
void UnPackPacket(const char* _buf, char* _str1, char* _str2, char* _str3)
{
	int str1size, str2size, str3size;

	const char* ptr = _buf + sizeof(PROTOCOL);

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

//로그인 정보 분해 패킷
void UnPackPacket(const char* _buf, char* _str1, char* _str2)
{
	int str1size, str2size;

	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;

	memcpy(&str2size, ptr, sizeof(str2size));
	ptr = ptr + sizeof(str2size);

	memcpy(_str2, ptr, str2size);
	ptr = ptr + str2size;
}

//방나가기 정보 분해 패킷
void UnPackPacket(const char* _buf, SOCKET sock, char* _str1)
{
	int str1size;

	const char* ptr = _buf + sizeof(PROTOCOL);


	memcpy(&c_room_info.exit_sock, ptr, sizeof(sock));
	ptr = ptr + sizeof(sock);


	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;

}

//저장파일 검색
BOOL SearchFile(const char* filename)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);
	if (hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	else {
		FindClose(hFindFile);
		return TRUE;
	}
}

//파일 불러오기
bool FileDataLoad()
{
	if (!SearchFile("UserInfo.info"))
	{
		FILE* fp = fopen("UserInfo.info", "wb");
		fclose(fp);
		return true;
	}

	FILE* fp = fopen("UserInfo.info", "rb");
	if (fp == NULL)
	{
		return false;
	}

	_User_Info info;
	memset(&info, 0, sizeof(_User_Info));

	while (1)
	{
		fread(&info, sizeof(_User_Info), 1, fp);
		if (feof(fp))
		{
			break;
		}
		_User_Info* ptr = new _User_Info;
		memcpy(ptr, &info, sizeof(_User_Info));
		Join_List[Join_Count++] = ptr;
	}

	fclose(fp);
	return true;
}

//파일 정보 추가
bool FileDataAdd(_User_Info* _info)
{
	FILE* fp = fopen("UserInfo.info", "ab");
	if (fp == NULL)
	{
		return false;
	}

	int retval = fwrite(_info, 1, sizeof(_User_Info), fp);

	if (retval != sizeof(_User_Info))
	{
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}

//파일 저장
void FileSave()
{
	FILE* fp = fopen("UserInfo.info", "wb");
	if (fp == NULL)
	{
		err_quit("파일저장실패");
	}

	for (int i = 0; i < Join_Count; i++)
	{
		int retval = fwrite(Join_List[i], 1, sizeof(_User_Info), fp);
		if (retval != sizeof(_User_Info))
		{
			err_quit("파일저장실패");
		}
	}
}


//정보 받기
int MessageRecv(_ClientInfo* _info)
{
	int retval = recv(_info->sock, _info->recvbuf + _info->comp_recvbytes, _info->recvbytes - _info->comp_recvbytes, 0);
	if (retval == SOCKET_ERROR) //강제연결종료요청인 경우
	{
		return ERROR_DISCONNECTED;
	}
	else if (retval == 0)
	{
		return DISCONNECTED;
	}
	else
	{
		_info->comp_recvbytes += retval;
		if (_info->comp_recvbytes == _info->recvbytes)
		{
			_info->comp_recvbytes = 0;
			_info->recvbytes = 0;
			return SOC_TRUE;
		}
		return SOC_FALSE;
	}

}

//정보 보내기
int MessageSend(_ClientInfo* _info)
{
	int retval = send(_info->sock, _info->sendbuf + _info->comp_sendbytes,
		_info->sendbytes - _info->comp_sendbytes, 0);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			return SOC_FALSE;
		}
		return ERROR_DISCONNECTED;
	}	
	else
	{
		_info->comp_sendbytes = _info->comp_sendbytes + retval;

		if (_info->sendbytes == _info->comp_sendbytes)
		{
			_info->sendbytes = 0;
			_info->comp_sendbytes = 0;

			return SOC_TRUE;
		}
		else
		{
			return SOC_FALSE;
		}
	}
}


int PacketRecv(_ClientInfo* _ptr)
{
	if (!_ptr->r_sizeflag)
	{
		_ptr->recvbytes = sizeof(int);
		int retval = MessageRecv(_ptr);
		switch (retval)
		{
		case SOC_TRUE:
			memcpy(&_ptr->recvbytes, _ptr->recvbuf, sizeof(int));
			_ptr->r_sizeflag = true;
			return SOC_FALSE;
		case SOC_FALSE:
			return SOC_FALSE;
		case ERROR_DISCONNECTED:
			err_display("recv error()");
			return DISCONNECTED;
		case DISCONNECTED:
			return DISCONNECTED;
		}
	}

	int retval = MessageRecv(_ptr);
	switch (retval)
	{
	case SOC_TRUE:
		_ptr->r_sizeflag = false;
		return SOC_TRUE;
	case SOC_FALSE:
		return SOC_FALSE;
	case ERROR_DISCONNECTED:
		err_display("recv error()");
		return DISCONNECTED;
	case DISCONNECTED:
		return DISCONNECTED;
	}
}

//접속 확인
_ClientInfo* AddClient(SOCKET _sock, SOCKADDR_IN _clientaddr)
{
	printf("\nClient 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(_clientaddr.sin_addr),
		ntohs(_clientaddr.sin_port));

	//소켓 구조체 배열에 새로운 소켓 정보 구조체 저장
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_clientaddr, sizeof(SOCKADDR_IN));
	ptr->next_state = NO_STATE;
	ptr->state = INIT_STATE;
	ptr->r_sizeflag = false;
	User_List[Count++] = ptr;
	return ptr;
}

//종료 확인
void RemoveClient(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("\nClient 종료: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

	for (int i = 0; i < Count; i++)
	{
		if (User_List[i] == _ptr)
		{
			delete User_List[i];

			for (int j = i; j < Count - 1; j++)
			{
				User_List[j] = User_List[j + 1];
			}
			User_List[Count - 1] = nullptr;
			Count--;
			break;
		}
	}

}


//서버에서 받은 프로토콜 구분
void RecvPacketProcess(_ClientInfo* _ptr)
{
	PROTOCOL protocol;

	//프로토콜 추출
	GetProtocol(_ptr->recvbuf, protocol);


	switch (_ptr->state)
	{
	case MENU_SELECT_STATE:	//로그인 전 부분
		switch (protocol)
		{
		case JOIN_INFO://회원가입
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw, _ptr->userinfo.nickname);
			JoinProcess(_ptr);			
			break;
		case LOGIN_INFO://로그인
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw);
			LoginProcess(_ptr);
			break;	
		case NICKNAME_CHANGE_INFO://닉네임 변경
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw, _ptr->userinfo.nickname);
			Nickname_change_process(_ptr);
			break;
		}
		break;
	case LOGIN_STATE:	//로그인 후 부분
		switch (protocol)
		{		

		case ROOM_INFO://방 선택 메뉴
			Room_chiseprocess(_ptr);
			break;


		case ROOM_NUM://방 번호 선택
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->num, _ptr->userinfo.id, _ptr->userinfo.pw);
			Room_num_process(_ptr);
			break;


		case ROOM_OUT_ME://방 나가기
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, c_room_info.exit_sock,c_room_info.nickname);
			Room_out_process(_ptr);
			break;


		case LOGOUT://클라이언트 종료	
			Logoutprocess(_ptr);			
			break;
		}
	}
}


//클라가 종료 했을 때 프로세스
void Logoutprocess(_ClientInfo* _ptr)
{
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, LOGOUT_RESULT, LOGOUT_SUCCESS, LOGOUT_MSG);

	int result = MessageSend(_ptr);
	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		_ptr->next_state = MENU_SELECT_STATE;
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:
		break;
	}

	//현재 접속된 유저id 제거
	for (int i = 0; i < connected_count; i++)
	{
		if (!strcmp(connected_ids[i], _ptr->userinfo.id))
		{
			for (int j = i; j < connected_count - 1; j++)
			{
				memmove(connected_ids[j], connected_ids[j + 1], sizeof(connected_ids[j + 1]));
			}
			printf("유저[%s]가 챗 프로그램을 종료합니다.\n", _ptr->userinfo.id);
			memset(connected_ids[connected_count - 1], 0, sizeof(connected_ids[connected_count - 1]));

			connected_count--;
			break;
		}
	}


	_ptr->state = MENU_SELECT_STATE;
}

//보내기 프로세스
void SendPacketProcess(_ClientInfo* _ptr)
{
	_ptr->state = _ptr->next_state;
	_ptr->next_state = NO_STATE;
}

//회원가입용 프로세스
void JoinProcess(_ClientInfo* _ptr)
{
	RESULT join_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//현재 있는 유저만큼 사이클
	for (int i = 0; i < Join_Count; i++)
	{
		//같은 아이디가 존재하면
		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))
		{
			//아이디 중복
			join_result = ID_EXIST;
			strcpy(msg, ID_EXIST_MSG);
			break;
		}
	}

	//곂치지 않는 정보라면
	if (join_result == NODATA)
	{
		//해당 유저정보 저장
		_User_Info* user = new _User_Info;
		memset(user, 0, sizeof(_User_Info));
		strcpy(user->id, _ptr->userinfo.id);
		strcpy(user->pw, _ptr->userinfo.pw);
		strcpy(user->nickname, _ptr->userinfo.nickname);

		//정보 추가
		FileDataAdd(user);

		//유저 추가
		Join_List[Join_Count++] = user;
		
		//회원가입 성공
		join_result = JOIN_SUCCESS;
		strcpy(msg, JOIN_SUCCESS_MSG);
	}

	//회원가입 결과
	protocol = JOIN_RESULT;

	//정보 저장
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, join_result, msg);

	//결과 보내기
	int result = MessageSend(_ptr);
	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		_ptr->next_state = MENU_SELECT_STATE;
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:		
		break;
	}

}


//로그인 프로세스
void LoginProcess(_ClientInfo* _ptr)
{
	RESULT login_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//가입 유저수만큼 사이클
	for (int i = 0; i < Join_Count; i++)
	{

		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))//아이디가 같고
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userinfo.pw))//비번이 같다면
			{
				if (connected_count > 0)//접속한 유저가 한명 이상일 때
				{
					for (int j = 0; j < connected_count; j++)//현재 접속한 유저만큼 반복
					{
						if (!strcmp(connected_ids[j], _ptr->userinfo.id))//접속한 유저가 있다면 에러
						{
							login_result = CNT_ACOOUNT;
							strcpy(msg, CNT_ACOOUNT_MSG);
							break;
						}

					}
					if (login_result == CNT_ACOOUNT)
					{
						break;
					}


				}


				//아이디 + 비번 + 접속계정 아닐 때
				//로그인 성공
				login_result = LOGIN_SUCCESS;
				strcpy(msg, LOGIN_SUCCESS_MSG);

				//접속기록 저장
				strncpy(connected_ids[connected_count], _ptr->userinfo.id, IDSIZE - 1);
				connected_ids[connected_count][IDSIZE - 1] = '\0';
				connected_count++;

				printf("%s님 접속확인 및 저장\n", _ptr->userinfo.id);

			}
			else//비번이 다르다면
			{
				//비번 오류
				login_result = PW_ERROR;
				strcpy(msg, PW_ERROR_MSG);
			}
			break;
		}
	}

	//아이디가 같지 않다면
	if (login_result == NODATA)
	{
		//아이디 오류
		login_result = ID_ERROR;
		strcpy(msg, ID_ERROR_MSG);
	}

	//로그인을 성공하지 못했다면
	if (login_result != LOGIN_SUCCESS)
	{
		//현재 받은 유저정보 삭제
		memset(&(_ptr->userinfo), 0, sizeof(_User_Info));
	}

	//로그인 결과
	protocol = LOGIN_RESULT;

	//로그인 결과 정보 저장
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, login_result, msg);
	
	//정보 보내기
	int result = MessageSend(_ptr);
	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		if (login_result == LOGIN_SUCCESS)
		{
			_ptr->next_state = LOGIN_STATE;
		}
		else
		{
			_ptr->next_state = MENU_SELECT_STATE;
		}
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:
		break;
	}

	if (login_result == LOGIN_SUCCESS)
	{
		_ptr->state = LOGIN_STATE;
	}

}


//닉네임 변경 프로세스
void Nickname_change_process(_ClientInfo* _ptr)
{
	RESULT Nickname_change_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//가입 유저 수 만큼 사이클
	for (int i = 0; i < Join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))//아이디가 같고
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userinfo.pw))//비번이 같다면
			{
				//유저정보 재저장
				_User_Info* user = new _User_Info;
				memset(user, 0, sizeof(_User_Info));
				strcpy(user->id, Join_List[i]->id);
				strcpy(user->pw, Join_List[i]->pw);
				strcpy(Join_List[i]->nickname, _ptr->userinfo.nickname);
				strcpy(user->nickname, Join_List[i]->nickname);

				Join_List[i] = user;
	
				FileSave(); //바뀐 데이터 저장

				//변경성공
				Nickname_change_result = NICKNAME_CHANGE_SUCCESS;
				strcpy(msg, NICKNAME_CHANGE_SUCCESS_MSG);
			}
			else//비번이 다르다면
			{
				//비번 오류
				Nickname_change_result = PW_ERROR;
				strcpy(msg, PW_ERROR_MSG);
			}
			break;
		}
	}

	//아이디가 다르다면
	if (Nickname_change_result == NODATA)
	{
		//아이디 오류
		Nickname_change_result = ID_ERROR;
		strcpy(msg, ID_ERROR_MSG);
	}


	if (Nickname_change_result != NICKNAME_CHANGE_SUCCESS)//변경이 안되었다면
	{
		memset(&(_ptr->userinfo), 0, sizeof(_User_Info));//받은 데이터 지우기
	}


	//닉변 결과
	protocol = NICKNAME_CHANGE_RESULT;
	
	//닉변 결과 정보 저장
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, Nickname_change_result, msg);

	//결과 보내기
	int result = MessageSend(_ptr);
	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		_ptr->next_state = MENU_SELECT_STATE;
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:
		break;
	}

}


//방 메뉴 프로세스
void Room_chiseprocess(_ClientInfo* _ptr)
{
	char msg[BUFSIZE];
	strcpy(msg, ROOM_CHISE_MSG);

	RESULT room_result = NODATA;

	PROTOCOL protocol= ROOM_INFO;

	//방 메뉴 정보 전송
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, room_result, msg);
	int result = MessageSend(_ptr);

	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		_ptr->next_state = MENU_SELECT_STATE;
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:
		break;
	}

}
 

//방 선택 프로세스
void Room_num_process(_ClientInfo* _ptr)
{
	//받은 방선택 문자열을 정수로 변환
	int num = atoi(_ptr->num);
	
	char name[NICKNAMESIZE];//닉네임
	char msg[BUFSIZE];//문자열 합치기용 함수
	int pot;//문자열 포트번호 변환용 int


	//해당 유저의 닉네임 찾아서 저장
	for (int i = 0; i < Join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userinfo.pw))
			{
				strcpy(name, Join_List[i]->nickname);
				break;
			}

		}
	}

	//확인 문자 출력
	printf("%s님 방번호: %d번\n", name,num);


	if (num == 1)//선택한 방이 1번일때
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//구조체 비우기
		memcpy(_ptr->room_ip.port, "9001", strlen("9001"));//포트번호 저장
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.1", strlen("235.7.8.1"));//주소 저장
		memcpy(_ptr->room_ip.name, name, strlen(name));//닉네임 저장
		pot = 9001;
	}
	else if (num == 2)//선택한 방이 2번일때
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//구조체 비우기
		memcpy(_ptr->room_ip.port, "9002", strlen("9002"));//포트번호 저장
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.2", strlen("235.7.8.2"));//주소저장
		memcpy(_ptr->room_ip.name, name, strlen(name));//닉네임 저장
		pot = 9002;
	}
	else if (num == 3)//선택한 방이 3번일때
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//구조체 비우기
		memcpy(_ptr->room_ip.port, "9003", strlen("9003"));//포트번호 저장
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.3", strlen("235.7.8.3"));//주소저장
		memcpy(_ptr->room_ip.name, name, strlen(name));//닉네임 저장
		pot = 9003;
	}


	
	//gui소켓 설정
	_ptr->catroom_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (_ptr->catroom_sock == INVALID_SOCKET) err_quit("socket()");

	
	// gul용 addr 초기화		
	ZeroMemory(&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	_ptr->catroom_addr.sin_family = AF_INET;
	_ptr->catroom_addr.sin_addr.s_addr = inet_addr(_ptr->room_ip.chat_ip);
	_ptr->catroom_addr.sin_port = htons(pot);


	//닉네임+입장문 문자열 합쳐서 생성
	strcpy(msg, _ptr->room_ip.name);
	strcat(msg, ROOM_IN_MSG);
	strcpy(_ptr->room_ip.chat_msg, msg);

	//해당 방 정보 저장
	strcpy(c_room_info.chat_msg, _ptr->room_ip.chat_msg);
	strcpy(c_room_info.port, _ptr->room_ip.port);
	strcpy(c_room_info.chat_ip, _ptr->room_ip.chat_ip);
	strcpy(c_room_info.nickname, _ptr->room_ip.name);

	c_room_info.room_in = true;


	//유저가 입장하는 해당 방에 입장문 전송
	_ptr->retval = sendto(_ptr->catroom_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
		(SOCKADDR*)&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	if (_ptr->retval == SOCKET_ERROR)
	{
		err_display("sendto()");	
	}


	//클라이언트에게 방 정보 전달
	_ptr->sendbytes = IP_PackPacket(_ptr->sendbuf, _ptr->room_ip.port, _ptr->room_ip.chat_ip, _ptr->room_ip.name);
	int result = MessageSend(_ptr);

	switch (result)
	{
	case ERROR_DISCONNECTED:
		err_display("connect end");
	case DISCONNECTED:
		_ptr->state = DISCONNECTED_STATE;
		break;
	case SOC_FALSE:
		_ptr->next_state = MENU_SELECT_STATE;
		_ptr->state = SEND_DELAY_STATE;
		return;
	case SOC_TRUE:
		break;
	}

	c_room_info.room_in = false;
}


//방 나가기 프로세스
void Room_out_process(_ClientInfo* _ptr)
{

	char msg[BUFSIZE];//문자열 합치기용 함수


	//닉네임+입장문 문자열 합쳐서 생성
	strcpy(msg, _ptr->userinfo.nickname);
	strcat(msg, ROOM_OUT_MSG);
	strcpy(c_room_info.chat_msg, msg);

	c_room_info.room_out = true;//방 나가기를 true

	//유저가 입장하는 해당 방에 퇴장문 전송
	_ptr->retval = sendto(_ptr->catroom_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
		(SOCKADDR*)&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	if (_ptr->retval == SOCKET_ERROR)
	{
		err_display("sendto()");
	}

	c_room_info.room_out = false;
}

