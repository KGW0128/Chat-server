/*
 * echo_selserv_win.c
 * Written by SW. YOON
 */

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define BUFSIZE 4096			//���� ������
#define IDSIZE 255				//���̵� ������
#define PWSIZE 255				//��� ������
#define NICKNAMESIZE 255		//�г��� ������
#define ERROR_DISCONNECTED -2	//�������� Ȯ��
#define DISCONNECTED -1			//���� Ȯ��
#define SOC_TRUE 1				//��
#define SOC_FALSE 0				//����


//��� ���ڵ�
#define ID_ERROR_MSG "���� ���̵��Դϴ�.\n"
#define PW_ERROR_MSG "��й�ȣ�� Ʋ�Ƚ��ϴ�.\n"
#define LOGIN_SUCCESS_MSG "�α��ο� �����߽��ϴ�.\n"
#define ID_EXIST_MSG "�̹� �ִ� ���̵� �Դϴ�.\n"
#define JOIN_SUCCESS_MSG "���Կ� �����߽��ϴ�.\n"
#define LOGOUT_MSG "�α׾ƿ��Ǿ����ϴ�.\n"
#define NICKNAME_CHANGE_SUCCESS_MSG "�г����� ����Ǿ����ϴ�.\n"
#define ROOM_CHISE_MSG "ä�ù��� �����ϼ���.\r\n1����\r\n2����\r\n3����"
#define ROOM_IN_MSG "���� �����߽��ϴ�.\r\n"
#define ROOM_OUT_MSG "���� �����߽��ϴ�.\r\n"
#define CNT_ACOOUNT_MSG "�̹� ���ӵ� �����Դϴ�.\n"


enum STATE //�������� state���� ��������
{
	NO_STATE = -1,
	INIT_STATE=1, 
	MENU_SELECT_STATE, 	
	LOGIN_STATE,	
	SEND_DELAY_STATE,
	DISCONNECTED_STATE
};



enum PROTOCOL//Ŭ�󿡰Լ� ���� ��û �������� ����
{ 
	JOIN_INFO,				//ȸ������ ��û
	LOGIN_INFO,				//�α��� ��û
	JOIN_RESULT,			//ȸ������ ���
	LOGIN_RESULT,			//�α��� ���
	LOGOUT,					//���� ��û
	LOGOUT_RESULT,			//���� ���
	ROOM_INFO,				//�� ��� ��û
	NICKNAME_CHANGE_INFO,	//�г��� ���� ��û
	NICKNAME_CHANGE_RESULT,	//�г��� ���� ���
	ROOM_NUM,				//�� ���� ��û
	ROOM_OUT_ME				//�� ������ ��û
};


enum RESULT//��� ������ ���� �������� ����
{
	NODATA = -1,				//��
	ID_EXIST = 1,				//���̵� ��ħ
	ID_ERROR,					//���̵� ����
	PW_ERROR,					//��� ����
	JOIN_SUCCESS,				//ȸ������ ����
	LOGIN_SUCCESS,				//�α��� ����
	LOGOUT_SUCCESS,				//���� ����
	NICKNAME_CHANGE_SUCCESS		//�г��� ���� ����
	
	
	,CNT_ACOOUNT				//�̹� ����� ����(����)
};


struct _User_Info//���� ���� ����ü
{
	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];
};

struct ROOM_IP//������ ������ ���� ���� ����ü
{
	char port[30];				// ä�ù� ��Ʈ��ȣ
	char chat_ip[30];			// ä�ù� ip�ּ�
	char name[NICKNAMESIZE];	//�г���
	char chat_msg[BUFSIZE + 1];	//ä�ù� �޼���
};


struct _ClientInfo//Ŭ�� ���� ����ü
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

	char		recvbuf[BUFSIZE];	//������� ����
	char		sendbuf[BUFSIZE];	//�ޱ�� ����

	char num[255];					//���� ������ ���ȣ
	ROOM_IP room_ip;

	int retval;
	//gui�� sand�� ���� ����
	SOCKET catroom_sock;
	//gui�� sand�� abbr ����
	SOCKADDR_IN catroom_addr;


};


struct _Chat_Room_Info//ä�� ���� ����ü
{
	char port[30];				//�޾ƿ� ä�ù� ��Ʈ��ȣ
	char chat_ip[30];			//�޾ƿ� ä�ù� ip�ּ�
	int num;					//���� ������ ���ȣ


	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];//ä�ù� �г���
	char chat_msg[BUFSIZE + 1];	//ä�ù� �޼���

	SOCKET exit_sock;			//���� ��Ĺ ���п�

	bool room_in = false;		//�� ���� Ȯ�� bool��
	bool room_out = false;		//�� ������ Ȯ�� bool��
	bool exit = false;			//Ŭ�� ���� Ȯ�� bool ��

	bool whis = false;			//����
	bool exp = false;			//�Ӹ�

	char  others_nickname[NICKNAMESIZE];

}c_room_info;



_ClientInfo* User_List[100];//���� ����ü �迭
int Count = 0;//���� ī��Ʈ

_User_Info* Join_List[100];//������ ����ü �迭
int Join_Count = 0;//�������� ī��Ʈ

FD_SET Rset, Wset;//�ޱ�, ���� ����

//���� ������ ���� Ȯ�ο�
char connected_ids[100][IDSIZE]; // ���� ���̵� �迭
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



//����
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

	if (!FileDataLoad())//������������ �ҷ�����
	{
		err_quit("file read error!");
	}

	while (1)
	{
		FD_ZERO(&Rset);
		FD_ZERO(&Wset);

		FD_SET(hServSock, &Rset);

		for (int i = 0; i < Count; i++)//���� ������ �ޱ����� �������� ��� ����
		{
			FD_SET(User_List[i]->sock, &Rset);

			if (User_List[i]->state == SEND_DELAY_STATE)
			{
				FD_SET(User_List[i]->sock, &Wset);
			}					
		}

		//���� ���� üũ
		if (select(0, &Rset, &Wset, 0, NULL) == SOCKET_ERROR)
		{
			err_quit("select() error");
		}			

		//ó�� ���Դٸ� �������� ����
		if (FD_ISSET(hServSock, &Rset)) 
		{
			int clntLen = sizeof(clntAddr);
			hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &clntLen);
			_ClientInfo* ptr = AddClient(hClntSock, clntAddr);
			ptr->state = MENU_SELECT_STATE;
			continue;
		}		



		//��� ���� ȸ��
		for (int i = 0; i < Count; i++)
		{
			_ClientInfo* ptr = User_List[i];

			if (FD_ISSET(ptr->sock, &Rset))//�ޱ�
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

			if (FD_ISSET(ptr->sock, &Wset))//������
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

			//������ ��
			if (ptr->state == DISCONNECTED_STATE)
			{
				RemoveClient(ptr);
				i--;
				continue;
			}

		}
	}	


	//���� ����� ������������ ����
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

// ���� �Լ� ���� ���
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

//Ŭ�󿡼� �޾ƿ� �������� ����
void GetProtocol(const char* _ptr, PROTOCOL& _protocol)
{
	memcpy(&_protocol, _ptr, sizeof(PROTOCOL));

}

//��� ������� ��Ŷ
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

//�� �޴� ��Ŷ
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

//������ ������� ��Ŷ
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

//ȸ������&�к� ���� ���� ��Ŷ
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

//�α��� ���� ���� ��Ŷ
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

//�泪���� ���� ���� ��Ŷ
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

//�������� �˻�
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

//���� �ҷ�����
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

//���� ���� �߰�
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

//���� ����
void FileSave()
{
	FILE* fp = fopen("UserInfo.info", "wb");
	if (fp == NULL)
	{
		err_quit("�����������");
	}

	for (int i = 0; i < Join_Count; i++)
	{
		int retval = fwrite(Join_List[i], 1, sizeof(_User_Info), fp);
		if (retval != sizeof(_User_Info))
		{
			err_quit("�����������");
		}
	}
}


//���� �ޱ�
int MessageRecv(_ClientInfo* _info)
{
	int retval = recv(_info->sock, _info->recvbuf + _info->comp_recvbytes, _info->recvbytes - _info->comp_recvbytes, 0);
	if (retval == SOCKET_ERROR) //�������������û�� ���
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

//���� ������
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

//���� Ȯ��
_ClientInfo* AddClient(SOCKET _sock, SOCKADDR_IN _clientaddr)
{
	printf("\nClient ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(_clientaddr.sin_addr),
		ntohs(_clientaddr.sin_port));

	//���� ����ü �迭�� ���ο� ���� ���� ����ü ����
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

//���� Ȯ��
void RemoveClient(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("\nClient ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

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


//�������� ���� �������� ����
void RecvPacketProcess(_ClientInfo* _ptr)
{
	PROTOCOL protocol;

	//�������� ����
	GetProtocol(_ptr->recvbuf, protocol);


	switch (_ptr->state)
	{
	case MENU_SELECT_STATE:	//�α��� �� �κ�
		switch (protocol)
		{
		case JOIN_INFO://ȸ������
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw, _ptr->userinfo.nickname);
			JoinProcess(_ptr);			
			break;
		case LOGIN_INFO://�α���
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw);
			LoginProcess(_ptr);
			break;	
		case NICKNAME_CHANGE_INFO://�г��� ����
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->userinfo.id, _ptr->userinfo.pw, _ptr->userinfo.nickname);
			Nickname_change_process(_ptr);
			break;
		}
		break;
	case LOGIN_STATE:	//�α��� �� �κ�
		switch (protocol)
		{		

		case ROOM_INFO://�� ���� �޴�
			Room_chiseprocess(_ptr);
			break;


		case ROOM_NUM://�� ��ȣ ����
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, _ptr->num, _ptr->userinfo.id, _ptr->userinfo.pw);
			Room_num_process(_ptr);
			break;


		case ROOM_OUT_ME://�� ������
			memset(&_ptr->userinfo, 0, sizeof(_User_Info));
			UnPackPacket(_ptr->recvbuf, c_room_info.exit_sock,c_room_info.nickname);
			Room_out_process(_ptr);
			break;


		case LOGOUT://Ŭ���̾�Ʈ ����	
			Logoutprocess(_ptr);			
			break;
		}
	}
}


//Ŭ�� ���� ���� �� ���μ���
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

	//���� ���ӵ� ����id ����
	for (int i = 0; i < connected_count; i++)
	{
		if (!strcmp(connected_ids[i], _ptr->userinfo.id))
		{
			for (int j = i; j < connected_count - 1; j++)
			{
				memmove(connected_ids[j], connected_ids[j + 1], sizeof(connected_ids[j + 1]));
			}
			printf("����[%s]�� ê ���α׷��� �����մϴ�.\n", _ptr->userinfo.id);
			memset(connected_ids[connected_count - 1], 0, sizeof(connected_ids[connected_count - 1]));

			connected_count--;
			break;
		}
	}


	_ptr->state = MENU_SELECT_STATE;
}

//������ ���μ���
void SendPacketProcess(_ClientInfo* _ptr)
{
	_ptr->state = _ptr->next_state;
	_ptr->next_state = NO_STATE;
}

//ȸ�����Կ� ���μ���
void JoinProcess(_ClientInfo* _ptr)
{
	RESULT join_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//���� �ִ� ������ŭ ����Ŭ
	for (int i = 0; i < Join_Count; i++)
	{
		//���� ���̵� �����ϸ�
		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))
		{
			//���̵� �ߺ�
			join_result = ID_EXIST;
			strcpy(msg, ID_EXIST_MSG);
			break;
		}
	}

	//��ġ�� �ʴ� �������
	if (join_result == NODATA)
	{
		//�ش� �������� ����
		_User_Info* user = new _User_Info;
		memset(user, 0, sizeof(_User_Info));
		strcpy(user->id, _ptr->userinfo.id);
		strcpy(user->pw, _ptr->userinfo.pw);
		strcpy(user->nickname, _ptr->userinfo.nickname);

		//���� �߰�
		FileDataAdd(user);

		//���� �߰�
		Join_List[Join_Count++] = user;
		
		//ȸ������ ����
		join_result = JOIN_SUCCESS;
		strcpy(msg, JOIN_SUCCESS_MSG);
	}

	//ȸ������ ���
	protocol = JOIN_RESULT;

	//���� ����
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, join_result, msg);

	//��� ������
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


//�α��� ���μ���
void LoginProcess(_ClientInfo* _ptr)
{
	RESULT login_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//���� ��������ŭ ����Ŭ
	for (int i = 0; i < Join_Count; i++)
	{

		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))//���̵� ����
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userinfo.pw))//����� ���ٸ�
			{
				if (connected_count > 0)//������ ������ �Ѹ� �̻��� ��
				{
					for (int j = 0; j < connected_count; j++)//���� ������ ������ŭ �ݺ�
					{
						if (!strcmp(connected_ids[j], _ptr->userinfo.id))//������ ������ �ִٸ� ����
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


				//���̵� + ��� + ���Ӱ��� �ƴ� ��
				//�α��� ����
				login_result = LOGIN_SUCCESS;
				strcpy(msg, LOGIN_SUCCESS_MSG);

				//���ӱ�� ����
				strncpy(connected_ids[connected_count], _ptr->userinfo.id, IDSIZE - 1);
				connected_ids[connected_count][IDSIZE - 1] = '\0';
				connected_count++;

				printf("%s�� ����Ȯ�� �� ����\n", _ptr->userinfo.id);

			}
			else//����� �ٸ��ٸ�
			{
				//��� ����
				login_result = PW_ERROR;
				strcpy(msg, PW_ERROR_MSG);
			}
			break;
		}
	}

	//���̵� ���� �ʴٸ�
	if (login_result == NODATA)
	{
		//���̵� ����
		login_result = ID_ERROR;
		strcpy(msg, ID_ERROR_MSG);
	}

	//�α����� �������� ���ߴٸ�
	if (login_result != LOGIN_SUCCESS)
	{
		//���� ���� �������� ����
		memset(&(_ptr->userinfo), 0, sizeof(_User_Info));
	}

	//�α��� ���
	protocol = LOGIN_RESULT;

	//�α��� ��� ���� ����
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, login_result, msg);
	
	//���� ������
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


//�г��� ���� ���μ���
void Nickname_change_process(_ClientInfo* _ptr)
{
	RESULT Nickname_change_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;

	//���� ���� �� ��ŭ ����Ŭ
	for (int i = 0; i < Join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userinfo.id))//���̵� ����
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userinfo.pw))//����� ���ٸ�
			{
				//�������� ������
				_User_Info* user = new _User_Info;
				memset(user, 0, sizeof(_User_Info));
				strcpy(user->id, Join_List[i]->id);
				strcpy(user->pw, Join_List[i]->pw);
				strcpy(Join_List[i]->nickname, _ptr->userinfo.nickname);
				strcpy(user->nickname, Join_List[i]->nickname);

				Join_List[i] = user;
	
				FileSave(); //�ٲ� ������ ����

				//���漺��
				Nickname_change_result = NICKNAME_CHANGE_SUCCESS;
				strcpy(msg, NICKNAME_CHANGE_SUCCESS_MSG);
			}
			else//����� �ٸ��ٸ�
			{
				//��� ����
				Nickname_change_result = PW_ERROR;
				strcpy(msg, PW_ERROR_MSG);
			}
			break;
		}
	}

	//���̵� �ٸ��ٸ�
	if (Nickname_change_result == NODATA)
	{
		//���̵� ����
		Nickname_change_result = ID_ERROR;
		strcpy(msg, ID_ERROR_MSG);
	}


	if (Nickname_change_result != NICKNAME_CHANGE_SUCCESS)//������ �ȵǾ��ٸ�
	{
		memset(&(_ptr->userinfo), 0, sizeof(_User_Info));//���� ������ �����
	}


	//�к� ���
	protocol = NICKNAME_CHANGE_RESULT;
	
	//�к� ��� ���� ����
	_ptr->sendbytes = PackPacket(_ptr->sendbuf, protocol, Nickname_change_result, msg);

	//��� ������
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


//�� �޴� ���μ���
void Room_chiseprocess(_ClientInfo* _ptr)
{
	char msg[BUFSIZE];
	strcpy(msg, ROOM_CHISE_MSG);

	RESULT room_result = NODATA;

	PROTOCOL protocol= ROOM_INFO;

	//�� �޴� ���� ����
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
 

//�� ���� ���μ���
void Room_num_process(_ClientInfo* _ptr)
{
	//���� �漱�� ���ڿ��� ������ ��ȯ
	int num = atoi(_ptr->num);
	
	char name[NICKNAMESIZE];//�г���
	char msg[BUFSIZE];//���ڿ� ��ġ��� �Լ�
	int pot;//���ڿ� ��Ʈ��ȣ ��ȯ�� int


	//�ش� ������ �г��� ã�Ƽ� ����
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

	//Ȯ�� ���� ���
	printf("%s�� ���ȣ: %d��\n", name,num);


	if (num == 1)//������ ���� 1���϶�
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//����ü ����
		memcpy(_ptr->room_ip.port, "9001", strlen("9001"));//��Ʈ��ȣ ����
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.1", strlen("235.7.8.1"));//�ּ� ����
		memcpy(_ptr->room_ip.name, name, strlen(name));//�г��� ����
		pot = 9001;
	}
	else if (num == 2)//������ ���� 2���϶�
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//����ü ����
		memcpy(_ptr->room_ip.port, "9002", strlen("9002"));//��Ʈ��ȣ ����
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.2", strlen("235.7.8.2"));//�ּ�����
		memcpy(_ptr->room_ip.name, name, strlen(name));//�г��� ����
		pot = 9002;
	}
	else if (num == 3)//������ ���� 3���϶�
	{
		ZeroMemory(&_ptr->room_ip, sizeof(_ptr->room_ip));//����ü ����
		memcpy(_ptr->room_ip.port, "9003", strlen("9003"));//��Ʈ��ȣ ����
		memcpy(_ptr->room_ip.chat_ip, "235.7.8.3", strlen("235.7.8.3"));//�ּ�����
		memcpy(_ptr->room_ip.name, name, strlen(name));//�г��� ����
		pot = 9003;
	}


	
	//gui���� ����
	_ptr->catroom_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (_ptr->catroom_sock == INVALID_SOCKET) err_quit("socket()");

	
	// gul�� addr �ʱ�ȭ		
	ZeroMemory(&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	_ptr->catroom_addr.sin_family = AF_INET;
	_ptr->catroom_addr.sin_addr.s_addr = inet_addr(_ptr->room_ip.chat_ip);
	_ptr->catroom_addr.sin_port = htons(pot);


	//�г���+���幮 ���ڿ� ���ļ� ����
	strcpy(msg, _ptr->room_ip.name);
	strcat(msg, ROOM_IN_MSG);
	strcpy(_ptr->room_ip.chat_msg, msg);

	//�ش� �� ���� ����
	strcpy(c_room_info.chat_msg, _ptr->room_ip.chat_msg);
	strcpy(c_room_info.port, _ptr->room_ip.port);
	strcpy(c_room_info.chat_ip, _ptr->room_ip.chat_ip);
	strcpy(c_room_info.nickname, _ptr->room_ip.name);

	c_room_info.room_in = true;


	//������ �����ϴ� �ش� �濡 ���幮 ����
	_ptr->retval = sendto(_ptr->catroom_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
		(SOCKADDR*)&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	if (_ptr->retval == SOCKET_ERROR)
	{
		err_display("sendto()");	
	}


	//Ŭ���̾�Ʈ���� �� ���� ����
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


//�� ������ ���μ���
void Room_out_process(_ClientInfo* _ptr)
{

	char msg[BUFSIZE];//���ڿ� ��ġ��� �Լ�


	//�г���+���幮 ���ڿ� ���ļ� ����
	strcpy(msg, _ptr->userinfo.nickname);
	strcat(msg, ROOM_OUT_MSG);
	strcpy(c_room_info.chat_msg, msg);

	c_room_info.room_out = true;//�� �����⸦ true

	//������ �����ϴ� �ش� �濡 ���幮 ����
	_ptr->retval = sendto(_ptr->catroom_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
		(SOCKADDR*)&_ptr->catroom_addr, sizeof(_ptr->catroom_addr));
	if (_ptr->retval == SOCKET_ERROR)
	{
		err_display("sendto()");
	}

	c_room_info.room_out = false;
}

