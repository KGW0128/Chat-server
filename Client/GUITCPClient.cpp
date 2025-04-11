#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <ws2tcpip.h>

#define SERVERIP   "127.0.0.1"	//���� ip ����
#define SERVERPORT 9000			//���� ��Ʈ��ȣ ����

#define BUFSIZE 4096			//���� ������
#define IDSIZE 255				//���̵� ������
#define PWSIZE 255				//��� ������
#define NICKNAMESIZE 255		//�г��� ������


enum PROTOCOL//Ŭ���̾�Ʈ ���� ��������
{

	CHOOSE_MENU=-1,		//ó������ �޴�
	JOIN_MENU = 1,		//ȸ������ �޴�
	LOGIN_MENU,			//�α��� �޴�
	NICK_CHANGE_MENU,	//�г��� ���� �޴�


	CHOOSE_ROOM = 1,	//�α��� �� �漱�ø޴�
	RESULT,				//�漱��
	CATTING,			//ä��
	OUT_ROOM,			//�泪����
	EXIT,				//����
	WHISPER,			//�ӼӸ�
	EXPULSION,			//����

	ID,					//���̵� �Է� ����
	PW,					//��� �Է� ����
	NICK				//�г��� �Է� ����


};


struct _PlayerInfo//Ŭ���̾�Ʈ ���� ����ü
{
	SOCKET			sock;		//�������� ���� ��Ĺ
	SOCKADDR_IN		addr;		//�������� ���� addr

	PROTOCOL pro;				//�������� ��ȣ
	char port[30];				//�޾ƿ� ä�ù� ��Ʈ��ȣ
	char chat_ip[30];			//�޾ƿ� ä�ù� ip�ּ�
	int num;					//���� ������ ���ȣ


	char id[IDSIZE];			//�α��ο� ���̵�
	char pw[PWSIZE];			//�α��ο� ���
	char nickname[NICKNAMESIZE];//�α��ο� �г���
	char chat_msg[BUFSIZE + 1];	//�Է¹��� �����

	SOCKET exit_sock;			//�� ���� �� ���� ��Ĺ ���п�

	bool out = false;			//�� ������ Ȯ�� bool��

	int log_and_join;			//����,�α��� �޴� ���ð� ���� �Լ�

}info;


SOCKET sock; // ���� ��ſ� ����(tcp)
char buf[BUFSIZE + 1]; // ������ �ۼ��� ����
HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HWND hSendButton; // ������ ��ư
HWND roomoutButton; // ������ ��ư
HWND hEdit1, hEdit2; // ���� ��Ʈ��
SOCKET cat_sock; // ä�ÿ� ����(udp)
HWND you_out_Botton, whisper_Botten; // ����, �Ӹ� ��ư



struct _Chat_Room_Info//ä�ÿ� ����ü
{
	char port[30];				//�޾ƿ� ä�ù� ��Ʈ��ȣ
	char chat_ip[30];			//�޾ƿ� ä�ù� ip�ּ�
	int num;					//���� ������ ���ȣ


	char id[IDSIZE];			//���̵�
	char pw[PWSIZE];			//���
	char nickname[NICKNAMESIZE];//ä�ù� �г���
	char chat_msg[BUFSIZE + 1];	//ä�ù� �޼���

	SOCKET exit_sock;			//���� ��Ĺ ���п�

	bool room_in = false;		//�� ���� Ȯ�� bool��
	bool room_out = false;		//�� ������ Ȯ�� bool��
	bool exit = false;			//Ŭ�� ���� Ȯ�� bool ��

	bool whis = false;			//�Ӹ�
	bool exp = false;			//����

	char  others_nickname[NICKNAMESIZE];

}c_room_info;




enum SERVER_PROTOCOL//�������� ���� ��������
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


enum SERVER_RESULT//�������� �޾ƿ� ���� ���� ��������
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




// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);
//ä�þ�����
DWORD WINAPI GUI_Recv_Client(LPVOID arg);


bool PacketRecv(SOCKET _sock, char* _buf);
void Login_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, int& _size);
void Join_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _str1, char* _str2, char* _str3, int& _size);
void UnPackPacket(char* _buf, SERVER_RESULT& _result, char* _str1);

void ROOM_CHOOES_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, int& _size);
void ROOM_NUM_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, char* _num, char* _str1, char* _str2, int& _size);

void Room_UnPackPacket(char* _buf, char* _str1, char* _str2, char* _str3);

void ROOM_OUT_PackPacket(char* _buf, SERVER_PROTOCOL _protocol, SOCKET sock, char* _str1,int& _size);



//����
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL) return 1;


	// ���� ��� ������ ����
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch(uMsg)
	{
	case WM_INITDIALOG://��ȭ���ڸ� �����Ҷ�
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);//�Է�ĭ
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);//���ĭ
		hSendButton = GetDlgItem(hDlg, IDOK);//������ ��ư
		roomoutButton = GetDlgItem(hDlg, ROOM_OUT);//�� ������ ��ư

		you_out_Botton = GetDlgItem(hDlg, IDC_EXPULSION);//���� ��ư
		whisper_Botten = GetDlgItem(hDlg, IDC_WHISPER);//�ӼӸ� ��ư


		EnableWindow(roomoutButton, FALSE);//�� ������ ��ư ��Ȱ��ȭ

		EnableWindow(you_out_Botton, FALSE);//���� ��ư ��Ȱ��ȭ
		EnableWindow(whisper_Botten, FALSE);//�Ӹ� ��ư ��Ȱ��ȭ

		//���� �޴� ���
		DisplayText("<<�޴�>>\n");
		DisplayText("1. ȸ������\n");
		DisplayText("2. �α���\n");
		DisplayText("3. �г��� ����\n");
		DisplayText("����: ");


		info.pro = CHOOSE_MENU;//�޴� ���� enum���� ����
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;


	case WM_COMMAND://��ư �Է� Ȯ��
		switch(LOWORD(wParam))
		{
		
		case IDOK://������ �� ��
			if (info.pro == OUT_ROOM)//enum�� ������ ���ٸ�
			{
				info.pro = CHOOSE_ROOM;//�ٽ� �漱�� enum���� ����
			}

			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE+1);

			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			SetWindowText(hEdit1, "");//�Է�ĭ ����
			SetFocus(hEdit1);//�Է�ĭ ��Ŀ��
			SendMessage(hEdit1, EM_SETSEL, 0, -1);	
			return TRUE;


		case IDC_WHISPER://�ӼӸ�
			info.pro = WHISPER;
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;


		case IDC_EXPULSION://����
			info.pro = EXPULSION;
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;



		case ROOM_OUT://�泪����
			info.pro = OUT_ROOM;//enum�� �泪����� ����
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetWindowText(hSendButton, "������");//�������ư�� ���������� ����
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			info.out = false;//�� ������ Ȯ�� bool�� fasle		
			return TRUE;


		case IDCANCEL://����
			info.pro = EXIT;//enum�� ����� ����
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, info.chat_msg, BUFSIZE + 1);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			EndDialog(hDlg, IDCANCEL);//���ν��� �ݱ�
			return TRUE;


		}
		return FALSE;
	}
	return FALSE;
}




// Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	// ���� �ʱ�ȭ
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






//---------------��Ƽ ĳ��Ʈ ��
	
	SOCKADDR_IN baindaddr;// bind ����ü �ʱ�ȭ

	SOCKADDR_IN chataddr;// ���� �ּ� ����ü �ʱ�ȭ
	
	struct ip_mreq mreq;// ��Ƽĳ��Ʈ �׷� ����
	
	HANDLE hThread;// ä��recvfrom ������


	// ä�ÿ� ���� ���� ����
	cat_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (cat_sock == INVALID_SOCKET) err_quit("socket()");

	//���ù��� ������ �������
	BOOL optval = TRUE;
	retval = setsockopt(cat_sock, SOL_SOCKET,
		SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	int ttl = 2;//����͸� �Ѿ �� �ִ� ����(2== 1�� �Ѿ)
	retval = setsockopt(cat_sock, IPPROTO_IP, IP_MULTICAST_TTL,
		(char*)&ttl, sizeof(ttl));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");



	char buf[BUFSIZE];
	int size;
	SERVER_RESULT result;
	SERVER_PROTOCOL protocol;

	int pot;//���ڿ��� ���� ��Ʈ��ȣ ��ȯ�� int


	// ������ ������ ���
	while(1)
	{
		WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

		////����,�� ������, ������ �϶�
		//if (info.pro == OUT_ROOM || info.pro == EXIT|| info.pro == CHOOSE_ROOM||(info.pro == WHISPER&& c_room_info.exp==false )||(info.pro == EXPULSION && c_room_info.whis==false))
		//{
		//	//���̿��� ����� ���� ������ �Է� ����
		//	strcpy(info.chat_msg,"pass");
		//}


		//// ���ڿ� ���̰� 0�̸� ������ ����
		//if (strlen(info.chat_msg) == 0)
		//{
		//	EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		//	SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		//	continue;
		//}


		switch (info.pro)//��Ʈ��ȣ ����
		{

		case CHOOSE_MENU://ù ȭ�� �޴� ����

			info.log_and_join = atoi(info.chat_msg);//ȸ������, �α��� ���� ����
			DisplayText("%d\r\n", info.log_and_join);//������ ��ȣ ���

			if (info.log_and_join == JOIN_MENU)//ȸ������ ���ý�
			{
				DisplayText("\n");
				DisplayText("[ȸ������]\n");
				DisplayText("ID	: ");
				info.pro = ID;//�������� ���̵�� ����
			}
			else if(info.log_and_join == LOGIN_MENU)//�α��� ���ý�
			{
				DisplayText("\n");
				DisplayText("[�α���]\n");
				DisplayText("ID	: ");
				info.pro = ID;//�������� ���̵�� ����
			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//�к� ���ý�
			{
				DisplayText("\n");
				DisplayText("[�к�]\n");
				DisplayText("ID	: ");
				info.pro = ID;//�������� ���̵�� ����
			}
			else//�ٸ��� �Է����� ��
			{
				DisplayText("�߸��� �Է��Դϴ�. �ٽ� �Է��� �ּ���...\n");
				DisplayText("\n");
				DisplayText("<<�޴�>>\n");
				DisplayText("1. ȸ������\n");
				DisplayText("2. �α���\n");
				DisplayText("3. �г��� ����\n");
				DisplayText("����: ");

				EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
				SetEvent(hReadEvent); // �б� �Ϸ� �˸���
				continue;
			
			}

		break;



		case ID://���̵� �Է�

			strcpy(info.id, info.chat_msg);//���̵� ����
			DisplayText("%s\n", info.id);
			DisplayText("PW	: ");

			info.pro = PW;//�������� ������� ����


			break;



		case PW://��� �Է�

			strcpy(info.pw, info.chat_msg);//��� ����

			if (info.log_and_join == JOIN_MENU)//ȸ������ ���ý�
			{
				DisplayText("%s\n", info.pw);
				DisplayText("NickName	: ");

				info.pro = NICK;//�������� �г������� ����
			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//�г��� ���� ���ý�
			{
				DisplayText("%s\n", info.pw);
				DisplayText("NickName	: ");

				info.pro = NICK;//�������� �г������� ����
			}
			else if (info.log_and_join == LOGIN_MENU)//�α��� ���ý�
			{
				DisplayText("%s\n", info.pw);


				//�α��� ���� ����
				Login_PackPacket(buf, LOGIN_INFO, info.id, info.pw, size);

				//�α��� ���� ������
				retval = send(sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send()");
					break;
				}

				//�α��� ��� �ޱ�
				if (!PacketRecv(sock, buf))
				{
					break;
				}

				//�и�
				memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
				memset(info.chat_msg, 0, sizeof(info.chat_msg));
				UnPackPacket(buf, result, info.chat_msg);
				

				//��� ���
				DisplayText("%s\n", info.chat_msg);
				DisplayText("\n");

				switch (result)//enum ����
				{
				case ID_ERROR://���̵� ����
				case PW_ERROR://��� ����
				case CNT_ACOOUNT://�̹� ���Ӱ��� ����

					//�ʱ� �޴� �ٽ� ���
					DisplayText("<<�޴�>>\n");
					DisplayText("1. ȸ������\n");
					DisplayText("2. �α���\n");
					DisplayText("3. �г��� ����\n");
					DisplayText("����: ");

					info.pro = CHOOSE_MENU;//�޴� ���� enum���� ����
					break;


				case LOGIN_SUCCESS://�α��� ����
					
					//�� ��� �޴� �����޶�� �������� ��û
					ROOM_CHOOES_PackPacket(buf, ROOM_INFO, size);
					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
						break;
					}

					//�� ��� �޴� �޼��� �޾ƿ�
					if (!PacketRecv(sock, buf))
					{
						break;
					}

					//�и� �� ���
					memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
					memset(info.chat_msg, 0, sizeof(info.chat_msg));
					UnPackPacket(buf, result, info.chat_msg);
					DisplayText("%s\n", info.chat_msg);//�� �޴� ���


					info.pro = RESULT;//�� ��ȣ �������� �̵�
					break;

				}
			}


			break;



		case NICK://�г��� �Է�

			strcpy(info.nickname, info.chat_msg);//�г��� ����
			DisplayText("%s\n", info.nickname);


			if (info.log_and_join == JOIN_MENU)//ȸ������ ���ý�
			{
				//ȸ������ ���� ����
				Join_PackPacket(buf, JOIN_INFO, info.id, info.pw, info.nickname, size);

			}
			else if (info.log_and_join == NICK_CHANGE_MENU)//�г��� ���� ���ý�
			{
				//�к� ���� ����
				Join_PackPacket(buf, NICKNAME_CHANGE_INFO, info.id, info.pw, info.nickname, size);
			}

			//���� ������
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			//��� �޾ƿ���
			if (!PacketRecv(sock, buf))
			{
				break;
			}

			//����
			memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
			memset(info.chat_msg, 0, sizeof(info.chat_msg));
			UnPackPacket(buf, result, info.chat_msg);

			//��� ���
			DisplayText("%s\n", info.chat_msg);
			DisplayText("\n");

			//�ʱ�޴� ���
			DisplayText("<<�޴�>>\n");
			DisplayText("1. ȸ������\n");
			DisplayText("2. �α���\n");
			DisplayText("3. �г��� ����\n");
			DisplayText("����: ");

			info.pro = CHOOSE_MENU;//�޴� ���� enum���� ����

			break;



		case CHOOSE_ROOM://�޴�����

			SetWindowText(hSendButton, "������");//������ ��ư �ؽ�Ʈ ������� ����
			
			//�� ��� �޴� �����޶�� �������� ��û
			ROOM_CHOOES_PackPacket(buf, ROOM_INFO, size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			if (!PacketRecv(sock, buf))//�� ��� �޴� �޼��� �޾ƿ�
			{
				break;
			}

			//�и� �� ���
			memcpy(&protocol, buf, sizeof(SERVER_PROTOCOL));
			memset(info.chat_msg, 0, sizeof(info.chat_msg));
			UnPackPacket(buf, result, info.chat_msg);
			DisplayText("%s\n", info.chat_msg);//�� �޴� ���

			c_room_info.exp = false;

			info.pro = RESULT;//�� ��ȣ �������� �̵�
			break;


		case RESULT://�漱��
			
			//�Է¹��� ���ȣ �޾ƿ�
			info.num = atoi(info.chat_msg);

			//���� ���� �������� �ʾҴٸ�
			if (info.num < 1 || info.num>3)
			{
				DisplayText("���ȣ�� �ٽ� �Է��ϼ���.\n");
				break;
			}

			//�� ���� ���� ����
			ROOM_NUM_PackPacket(buf,  ROOM_NUM, info.chat_msg, info.id, info.pw, size);

			//���� ������
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			//�� ��� �޴� �޼��� �޾ƿ�
			if (!PacketRecv(sock, buf))
			{
				break;
			}

			//����
			Room_UnPackPacket(buf, info.port, info.chat_ip, info.nickname);

			//�� ������ ����
			strcpy(c_room_info.port, info.port);
			strcpy(c_room_info.chat_ip, info.chat_ip);
			strcpy(c_room_info.nickname, info.nickname);


			//���ڿ��� ���� ��Ʈ��ȣ�� ������ ��ȯ
			pot = atoi(info.port);
			

			//ä�ù濬��	
			ZeroMemory(&baindaddr, sizeof(baindaddr));
			baindaddr.sin_family = AF_INET;
			baindaddr.sin_addr.s_addr = htonl(INADDR_ANY);//���ּ�
			baindaddr.sin_port = htons(pot);//�޾ƿ� ä�ù� ����
			//binding
			retval = bind(cat_sock, (SOCKADDR*)&baindaddr, sizeof(baindaddr));
			if (retval == SOCKET_ERROR) err_quit("bind()");


			//ä�ÿ� addr ����
			ZeroMemory(&chataddr, sizeof(chataddr));
			chataddr.sin_family = AF_INET;
			chataddr.sin_addr.s_addr = inet_addr(info.chat_ip);//�޾ƿ� ä�ù� �ּ�
			chataddr.sin_port = htons(pot);//�޾ƿ� ä�ù� ����

			// ��Ƽĳ��Ʈ �׷� ����
			mreq.imr_multiaddr.s_addr = inet_addr(info.chat_ip);//������ �ּ�
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);//���ּ�
			retval = setsockopt(cat_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,//����
				(char*)&mreq, sizeof(mreq));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");

			//// ���幮 ���
			DisplayText("%s���濡 �����߽��ϴ�.\r\n", info.chat_msg);

			c_room_info.exit_sock = cat_sock; //�ڽ� ��Ĺ ����

			//gui recv�� ������ ����
			hThread = CreateThread(NULL, 0, GUI_Recv_Client, (LPVOID)cat_sock, 0, NULL);


			EnableWindow(roomoutButton, TRUE); // ������ ��ư Ȱ��ȭ
			EnableWindow(you_out_Botton, TRUE);//���� ��ư Ȱ��ȭ
			EnableWindow(whisper_Botten, TRUE);//�Ӹ� ��ư Ȱ��ȭ

			info.pro = CATTING;//ä���� enum���� ����

			break;


		case CATTING://ä����
			
			//ä�ó��� ����
			strcpy(c_room_info.chat_msg, info.chat_msg);



			// ä�� ���� ������
			retval = sendto(cat_sock, (char*)&c_room_info, sizeof(_Chat_Room_Info), 0,
				(SOCKADDR*)&chataddr, sizeof(chataddr));
			if (retval == SOCKET_ERROR) {
				err_display("sendto()");
				break;
			}


			c_room_info.whis = false;
			c_room_info.exp == false;

			break;



		case WHISPER://�Ӹ�

			if (c_room_info.whis == false)
			{
				DisplayText("�ӼӸ� ����� �г����� ��������.\n");
				c_room_info.whis = true;
				info.pro = WHISPER;
				break;
				
			}
			else if (c_room_info.whis == true)
			{
				strcpy(c_room_info.others_nickname, info.chat_msg);//�г��� ����
				DisplayText("������ ������ �Է��ϼ���.\n");
				info.pro = CATTING;
				break;
			}

			break;



		case EXPULSION: //����
			if (c_room_info.exp == false)
			{
				DisplayText("������ ����� �г����� ��������.\n");
				c_room_info.exp = true;
				info.pro = EXPULSION;
				break;
				
			}
			if (c_room_info.exp == true)
			{
				strcpy(c_room_info.others_nickname, info.chat_msg);//�г��� ����

				retval = sendto(cat_sock, (char*)&c_room_info, sizeof(_PlayerInfo), 0,
					(SOCKADDR*)&chataddr, sizeof(chataddr));
				if (retval == SOCKET_ERROR) {
					err_display("sendto()");
					break;
				}

				c_room_info.exp = false;
				info.pro = CATTING;//ä���� enum���� ����			
				break;
			}

			break;




		case OUT_ROOM://�泪����

	
			// �������� �����ٰ� ������
			ROOM_OUT_PackPacket(buf, ROOM_OUT_ME, c_room_info.exit_sock ,info.nickname,size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}


			//gui recv�� �����尡 ������ ���� ���
			WaitForSingleObject(hThread, INFINITE); 
			CloseHandle(hThread);//gui recv�� ������ ����

			//���� Ȯ�� ���� ���
			DisplayText("\r\nä�þ����带 �ݽ��ϴ�...\r\n\r\n");

			//ä�� ���� ����
			closesocket(cat_sock);

			//ä�� ���� �����
			cat_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (cat_sock == INVALID_SOCKET) err_quit("socket()");
			
			//���ù��� ������ �� �缳��
			retval = setsockopt(cat_sock, SOL_SOCKET,
				SO_REUSEADDR, (char*)&optval, sizeof(optval));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");

			//����� �Ѿ�� �缳��
			retval = setsockopt(cat_sock, IPPROTO_IP, IP_MULTICAST_TTL,
				(char*)&ttl, sizeof(ttl));
			if (retval == SOCKET_ERROR) err_quit("setsockopt()");


			// ������ ��ư ��Ȱ��ȭ
			EnableWindow(roomoutButton, FALSE); 
			
			EnableWindow(you_out_Botton, FALSE);//�� ������ ��ư ��Ȱ��ȭ
			EnableWindow(whisper_Botten, FALSE);//�� ������ ��ư ��Ȱ��ȭ

			SetWindowText(roomoutButton, "�泪����");//�������ư�� ���������� ����

			
			//�������� ���� �ȳ� ���ڿ� ���
			DisplayText("�������� ���Ͻø� Ȯ�� ��ư�� ��������.\r\n");
			


			break;


		case EXIT://����


			//���� ���� ������
			ROOM_CHOOES_PackPacket(buf, LOGOUT, size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}


			//������ ����� �����ߴ��� Ȯ���� ���� recv
			if (!PacketRecv(sock, buf))
			{
				break;
			}


			break;


		default:
			break;

		}

		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
	}

	return 0;
}

DWORD WINAPI GUI_Recv_Client(LPVOID arg)//ä�þ�����
{
	//������ ���� �Լ�
	SOCKET client_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	_Chat_Room_Info chat_info;//recv��

	// Ŭ���̾�Ʈ ���� ���
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);//�� ���ϰ� ����� ���� �ּҸ� ����

	while (1)
	{
		//�������Լ� �� ���� �޼��� �޾ƿ�
		int len = sizeof(clientaddr);
		retval = recvfrom(client_sock, (char*)&chat_info, sizeof(_Chat_Room_Info), 0,
			(SOCKADDR*)&clientaddr, &len);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvfrom()");

		}

	

		if (chat_info.room_out == true)//���� �޼����� �����̸�
		{

			if (client_sock == chat_info.exit_sock)//���� ��Ĺ�� ���� ��Ĺ�̶� ���ٸ�
			{
				//������ ������� ���� ����
				break;
			}
			else//������ ���� �ٸ��ٸ�
			{
				// ���� ������ ���
				DisplayText("%s%s\r\n", chat_info.nickname, chat_info.chat_msg);

			}
		}
		else if (chat_info.exp == true)//�����̸�
		{
			if (strcmp(chat_info.others_nickname, info.nickname) == 0)
			{
				DisplayText("�������κ��� ������ϼ̽��ϴ�.\r\n");


				EnableWindow(you_out_Botton, FALSE);//�� ������ ��ư ��Ȱ��ȭ
				EnableWindow(whisper_Botten, FALSE);//�� ������ ��ư ��Ȱ��ȭ
				EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ

				SetWindowText(roomoutButton, "Ȯ��");//�������ư�� ���������� ����


				DisplayText("Ȯ�� ��ư�� ��������.\r\n");

			}
			else//������ ���� �ٸ��ٸ�
			{
				// ���� ������ ���
				DisplayText("%s���� ���忡 ���� ������ϼ̽��ϴ�.\r\n", chat_info.others_nickname);
			}

		}
		else//���� �޼����� ���ᰡ �ƴϸ�
		{

			if (chat_info.whis == true)
			{

				if (strcmp(chat_info.others_nickname, info.nickname) == 0)
				{
					DisplayText("[%s������ ������ �ӼӸ�]: %s\r\n", chat_info.nickname, chat_info.chat_msg);
					chat_info.whis = false;
				}


			}
			else//�Ϲ� ä��
			{
				if (chat_info.room_in == true)//���� �޼����� ���幮�̸�
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





// ���� ��Ʈ�� ��� �Լ�
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
		SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);//true�϶� ���� ���� �����,false�� ����

	}
	else
	{
		SendMessage(hEdit2, EM_REPLACESEL, TRUE, (LPARAM)cbuf);//true�϶� ���� ���� �����,false�� ����

	}
	va_end(arg);
}

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ����� ���� ������ ���� �Լ�
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



//�α��� ��Ŷ
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



//ȸ������ ��Ŷ
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


//��� �޴� ��Ŷ
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


//������ ������ �޴� ��Ŷ
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



//�� ���� �޴� ��û ��Ŷ
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


//�� ������ ��ȣ ������ ��Ŷ
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

//�� ������ ��Ŷ
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