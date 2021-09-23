#include <bits/stdc++.h>
#include <windows.h>
using namespace std;

int main()
{
	HANDLE qwer= HeapCreate(0, 1000, 10000);
	int x=10;
	for(int i=1;i<=100;++i)
	{
		x+=i;
	} 
	std::cout<<x;
	return 0;
} 
