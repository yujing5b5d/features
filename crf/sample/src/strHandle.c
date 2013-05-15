#include "strHandle.h"
#include <ctype.h>


int spilitContent(char *dest,int dlen,const char *src,int len,char *predeli,char *nextdeli)
{
	int i;
	int j = 0;
	int k;
	int flag = 0;
	*predeli = *nextdeli = ' ';
	int connect = 0;
	for(i = 0 ; i < len ; i ++)
	{
		if(isConnectCh(src[i])) {
			connect = 1;
			if(i>0)
			{
				//if(DIGITLIKE(src[i-1])&&DIGITLIKE(src[i+1]))
				if(DIGITLIKE(src[i+1]))
				{
					connect = 0;
					*nextdeli = '-';
					break;
				}
			}
		}
		if(!flag)
		{
			if(!isDelimiter(src[i]))
			{
				j = 0;
				flag = 1;
			}else if(!isBlank(src[i]))
			{
				*predeli = src[i];
			}
		}else if(isDelimiter(src[i]) && !connect)
		{
			for(k=i+1;k<len;k++)
			{
				if(isAsciiOrDigit(src[k])) break;
				if(isConnectCh(src[k]))
				{
					connect = 1;
					break;
				}else if(isDelimiter(src[k]) && !isBlank(src[k]))
					break;
				
			}
			if(!connect)
			{
				*nextdeli = src[i];
				break;
			}else
			{
				i = k;
			}
		}
		if(isDelimiter(src[i]) || isConnectCh(src[i]))
		{
			continue;
		}
		if(j>=dlen) return 0;
		dest[j] = src[i];
		j++;
		connect = 0;
	}
	if(!flag) return 0;
	dest[j] = '\0';
	j++;
	if(isBlank(*nextdeli))
	{
		for(;i<len;i++)
		{
			if(isAsciiOrDigit(src[i])){ break;}
			if(!isBlank(src[i]) && !isConnectCh(src[i]))
			{
				*nextdeli = src[i];
				break;
			}
		}
	}
	return i;
}


inline char* strCaseTransfer(char *s, int type)
{
	char *str = s;
	char *res = s;
	if (type == 0)
	{
		while(*s) *str++ = toupper(*s++);
	}else{
		while(*s) *str++ = tolower(*s++);
	}
	return res;
}


