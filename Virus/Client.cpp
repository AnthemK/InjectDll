//��һ���ǵñ����32λ�汾!!!!!! 
//�������������м���  -lws2_32  ������� ws2_32.lib; ���������� 
#include<windows.h>
#include<stdio.h>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")  //���� ws2_32.dll

int main(void)
{
	//��ʼ��DLL
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	//�����׽���
	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//���������������
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));  //ÿ���ֽڶ���0���
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(1234);
	connect(sock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
	Sleep(500);
	//���շ��������ص�����
	char szBuffer[MAXBYTE] = { 0 };
	recv(sock, szBuffer, MAXBYTE, 0);
	//������յ�������
	printf("Message form server: %s\n", szBuffer);
	//�ر��׽���
	closesocket(sock);
	//��ֹʹ�� DLL
	WSACleanup();
	return 0;
}
