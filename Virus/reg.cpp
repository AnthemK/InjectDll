#include<stdio.h>
#include<windows.h>
int main(void)
{
	//�������Ӽ����ƺ͵��Ӽ��ľ��
	HKEY hRoot=HKEY_LOCAL_MACHINE;
	char szSubKey[]="Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	HKEY hKey;//��ָ���Ӽ�
	DWORD dwDisposition=REG_OPENED_EXISTING_KEY;
	//��������ھʹ���
	LONG lRet=RegCreateKeyEx(
		hRoot,
		szSubKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,
		NULL,
		&hKey,
		&dwDisposition
		);
	if(lRet!=ERROR_SUCCESS)
		return 0;
	//�õ���ǰִ���ļ����ļ���������·����
	char szModule[MAX_PATH];
	GetModuleFileName(NULL,szModule,MAX_PATH);
	//����һ���µļ�ֵ�����ü�ֵ����Ϊ�ļ�
	lRet=RegSetValueEx(
		hKey,
		"SelfRunDemo",
		0,
		REG_SZ,
		(BYTE*)szModule,
		strlen(szModule)
		);
	if(lRet==ERROR_SUCCESS)
		printf("self run success\n");
	//�ر��Ӽ����
	RegCloseKey(hKey);
	
}
 
