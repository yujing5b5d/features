#include "eftfun.h"
#include "virtualcontent.h"
#include "persistence.h"
#include "tokens.h"
#include "minEditDistance.h"
#include "myMath.h"
#include "hftnpse.h"
#include "hftctl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//TODO DEBUG
#include "debuginfo.h"

//#define INDEBUG 0

#define thresholdForDifferneces  10

#define thresholdForGetOffsetSuggestion(x)  x*0.3
#define T4GOS(x) thresholdForGetOffsetSuggestion(x)
#define INLMT(x) (editDistanceT(\
			x,\
			strlen(x),\
			content+i,\
			strlen(x)>strlen(content+i)?\
				strlen(content+i):\
				strlen(x),\
			T4GOS(strlen(x))\
			)!= -1)

#define MINANDNZ(x,y) (x!=0?(y!=0?(x>y?y:x):(x)):y)
/*
#define SHOWLMT(x) printf("%s(%d) %d <=> %d <%d>\n",\
			x,strlen(x),\
			editDistanceS(x,strlen(x),content+realoffset,\
				strlen(x)>strlen(content+realoffset)?strlen(content+realoffset):strlen(x)),\
			T4GOS(strlen(x)),\
			INLMT(x)\
			);
*/
//#define INLMT(x) (editDistanceS(x,strlen(x),content+i,strlen(x)) <= T4GOS(strlen(x)))

//#define IN(x) {printf("in:%s\n",x);fflush(NULL);}
//#define OT(x) {printf("out:%s\n",x);fflush(NULL);}
//#define NX(x) {printf("next:%s\n",x);fflush(NULL);}

endFeatureData mfd;
endFeatureDataContainer mfdc;


OffsetCallback endFunctionList[ENDCALLBACKLEN]={hasPPafterTheOffset,
						hasPPafterTheOffset2,
						hasYearafterTheOffset,
						hasNameafterTheOffset0,
						hasNameafterTheOffset1,//hasNameafterTheOffset2,
						hasSeqOfTheOffset,
						hasSeqOfTheOffset2,
						hasSpecialKeyWords,hasLocationafterTheOffset};

struct endKWD
{
	unsigned int len;
	char key[200];
};

typedef struct theEKContainer
{
	int top;
	struct endKWD data[200];
}ekContainer;

ekContainer myEc;

inline int cleanEndKWDContainer()
{
	myEc.top = 0;
	return 1;
}

inline int insertEndKWD(const char *key)
{
	if(myEc.top<0) myEc.top = 0;
	if(myEc.top>=200) return 0;
	sprintf(myEc.data[myEc.top].key,"%s",key);
	myEc.data[myEc.top].len = strlen(key);
	myEc.top++;
	return 1;
}

int getLastYearOffset(unsigned int startOffset,int limit)
{
	int offset = 0;
	while((startOffset = hasYearafterTheOffset(startOffset,limit)) != 0)
	{
		offset = startOffset;
	}
	return offset;
}

//TODO DEBUGING
int getLastPageOffset(unsigned int startOffset,int limit)
{
	int offset = 0;
	while((startOffset = hasPPafterTheOffset(startOffset,limit)) != 0)
	{
		offset = startOffset;
	}
	return offset;
}

int getLastPage2Offset(unsigned int startOffset,int limit)
{
	int offset = 0;
	while((startOffset = hasPPafterTheOffset2(startOffset,limit)) != 0)
	{
		offset = startOffset;
	}
	return offset;
}

//int maxLen = 0;

int basicFilter(endFeatureDataContainer *container,unsigned int startOffset)
{
	char *content = getPcontent();
	int cLen = getPclen();	
	int hasContent = 0;
	container->top = 0;
	//introduce of settings
	// 0 empty for valued -- index of all offsets
	// 1 acknowledgements etc.
	// 2 table , he is figure ... (a list)
	// 3 end of year // before end of content
	int lastYearOffset = getLastYearOffset(startOffset,cLen);
	int isMarkedEOY = 0;
	
	// 4 end of page // before end of content
	int lastPageOffset = getLastPageOffset(startOffset,cLen);
	int isMarkedEOP = 0;
	
	// 5 end of page2 // before end of content
	int lastPageOffset2 = getLastPage2Offset(startOffset,cLen);
	int isMarkedEOP2 = 0;
	// 6 end of article
	// nothing to init
	
	// 7 end year before ack
	int markedAck=0;
	int markedTable=0;
	
	int endYearBeforeAck = 0;
	int endYearBeforeTable = 0;
	
	int endPageBeforeAck = 0;
	int endPageBeforeTable = 0;
	
	int endPage2BeforeAck = 0;
	int endPage2BeforeTable = 0;
	
	for(int i=startOffset;i<cLen;i++)
	{
		if(i!=0) if(fitPattern('d',content[i-1])) continue;
		if(INLMT("APPENDIX")
			|| INLMT("ACKNOWLEDGEMENT")
			|| INLMT("AUTHOR BIBLIOGRAPHIES")
			|| INLMT("AUTHOR BIBLIOGRAPHY")
			|| INLMT("AUTHOR BIOGRAPHIES")
			|| INLMT("AUTHOR BIOGRAPHY"))
		{
			if(!markedAck)
			{
				endYearBeforeAck = i;
				endPageBeforeAck = i;
				endPage2BeforeAck = i;
				markedAck = 1;
			}
			
		}
		for(int x = 0;x < myEc.top;x++)
		{
			if(INLMT(myEc.data[x].key))
			{
				if(!markedTable)
				{
					endYearBeforeTable = i;
					endPageBeforeTable = i;
					endPage2BeforeTable = i;
					markedTable = 1;
				}
				
			}
		}
		if(markedAck && markedTable) break;
	}
	
	
	
	endYearBeforeAck = endYearBeforeAck == 0 ? 0 : getLastPageOffset(startOffset,endYearBeforeAck);
	//int isMarkedEYBA = 0;
	
	// 8 end year before table
	endYearBeforeTable = endYearBeforeTable == 0 ? 0 : getLastPageOffset(startOffset,endYearBeforeTable);
	//int isMarkedEYBT = 0;
	
	// 9 end year before ack or table
	int endYearBeforeAckOrTable = MINANDNZ(endYearBeforeAck,endYearBeforeTable);
	//int isMarkedEYBAOT = 0;
	
	
	// 10 end page before ack
	endPageBeforeAck = endPageBeforeAck == 0 ? 0 : getLastPageOffset(startOffset,endPageBeforeAck);
	//int isMarkedEPBA = 0;
	
	
	// 11 end page before table
	endPageBeforeTable = endPageBeforeTable == 0 ? 0 : getLastPageOffset(startOffset,endPageBeforeTable);
	//int isMarkedEPBT = 0;
	
	// 12 end page before ack or table
	int endPageBeforeAckOrTable = MINANDNZ(endPageBeforeAck,endPageBeforeTable);
	//int isMarkedEPBAOT = 0;
	
	// 13 end page2 before ack
	endPage2BeforeAck = endPage2BeforeAck == 0 ? 0 : getLastPage2Offset(startOffset,endPage2BeforeAck);
	//int isMarkedEP2BA = 0;
	
	// 14 end page2 before table
	endPage2BeforeTable = endPage2BeforeTable == 0 ? 0 : getLastPage2Offset(startOffset,endPage2BeforeTable);
	//int isMarkedEP2BT = 0;
	
	// 15 end page2 before ack or table
	int endPage2BeforeAckOrTable = MINANDNZ(endPage2BeforeAck,endPage2BeforeTable);
	//int isMarkedEP2BAOT = 0;
	
	// 16 index of year
	int nextYearOffset = hasYearafterTheOffset(startOffset,cLen);;
	int indexOfYear = 0;
	// 17 index of page
	int nextPageOffset = hasPPafterTheOffset(startOffset,cLen);
	int indexOfPage = 0;
	
	// 18 index of page2
	int nextPage2Offset = hasPPafterTheOffset2(startOffset,cLen);
	int indexOfPage2 = 0;
	
	for(int i=startOffset;i<cLen;i++)
	{//endYearBeforeAck
		// init
		hasContent = 0;
		
		// 16 index of year
		if(i==nextYearOffset && nextYearOffset != -1)
		{
			indexOfYear ++;
			container->data[container->top].t[16] = indexOfYear;
			nextYearOffset = hasYearafterTheOffset(nextYearOffset,cLen);
			if(nextYearOffset == 0) nextYearOffset = -1;
			//printf("[Y]");
			hasContent = 1;
		}
		// 17 index of page
		if(i==nextPageOffset && nextPageOffset != -1)
		{
			indexOfPage ++ ;
			container->data[container->top].t[17] = indexOfPage;
			nextPageOffset = hasPPafterTheOffset(nextPageOffset,cLen);
			//printf("#################################RECEIVE:%d\n",nextPageOffset);
			if(nextPageOffset == 0) nextPageOffset = -1;
			//printf("[P]");
			//printfContextS(i,"P1");
			hasContent = 1;
		}
		// 18 index of page2
		if(i==nextPage2Offset && nextPage2Offset != -1)
		{
			indexOfPage2 ++ ;
			container->data[container->top].t[17] = indexOfPage2;
			nextPage2Offset = hasPPafterTheOffset(nextPage2Offset,cLen);
			if(nextPage2Offset == 0) nextPage2Offset = -1;
			//printf("[PP]");
			//printfContextS(i,"P2");
			hasContent = 1;
		}
		
		
		if(i!=0)
		{
			// 7 ~ 15
			// 7 end year before ack
			if(endYearBeforeAck == i)
			{
				container->data[container->top].t[7] = 1;
				hasContent = 1;
				//isMarkedEYBA = 1;
			}
			// 8 end year before table
			if(endYearBeforeTable == i)
			{
				container->data[container->top].t[8] = 1;
				hasContent = 1;
				//isMarkedEYBT = 1;
			}
			// 9 end year before ack or table
			if(endYearBeforeAckOrTable == i)
			{
				container->data[container->top].t[9] = 1;
				hasContent = 1;
				//isMarkedEYBAOT = 1;
			}
			// 10 end page before ack
			if(endPageBeforeAck == i)
			{
				container->data[container->top].t[10] = 1;
				hasContent = 1;
				//isMarkedEPBA = 1;
			}
			// 11 end page before table
			if(endPageBeforeTable == i)
			{
				container->data[container->top].t[11] = 1;
				hasContent = 1;
				//isMarkedEPBT = 1;
			}
			// 12 end page before ack or table
			if(endPageBeforeAckOrTable == i)
			{
				container->data[container->top].t[12] = 1;
				hasContent = 1;
				//isMarkedEPBAOT = 1;
			}
			// 13 end page2 before ack
			if(endPage2BeforeAck == i)
			{
				container->data[container->top].t[13] = 1;
				hasContent = 1;
				//isMarkedEP2BA = 1;
			}
			// 14 end page2 before table
			if(endPage2BeforeTable == i)
			{
				container->data[container->top].t[14] = 1;
				hasContent = 1;
				//isMarkedEP2BT = 1;
			}
			// 15 end page2 before ack or table
			if(endPage2BeforeAckOrTable == i)
			{
				container->data[container->top].t[15] = 1;
				hasContent = 1;
				//isMarkedEP2BAOT = 1;
			}
		
			// ingore useless i == 0 or past is not data(data:[0-9][a-z][A-Z])
			if(fitPattern('d',content[i-1]) && !hasContent) continue;
		}
		
		
		
		
		//////////////////////////////////////////////////////////////////////////
		// 0 empty for valued -- index of all offsets
		// leave empty
		
		// 1 acknowledgements etc.
		//
		if(INLMT("APPENDIX")
			|| INLMT("ACKNOWLEDGEMENT")
			|| INLMT("AUTHOR BIBLIOGRAPHIES") 
			|| INLMT("AUTHOR BIBLIOGRAPHY")
			|| INLMT("AUTHOR BIOGRAPHIES")
			|| INLMT("AUTHOR BIOGRAPHY"))
		{
			container->data[container->top].t[1] = 1;
			hasContent = 1;
		}
		
		// 2 table , he is figure ... (a list)
		for(int x = 0;x < myEc.top;x++)
		{
			if(INLMT(myEc.data[x].key))
			{
				//printf("[+]");
				container->data[container->top].t[2] = 1;
				hasContent = 1;
			}
		}
		
		// 3 end of year // before end of article
		if(!hasDifferneces(lastYearOffset,i))
		{
			if(!isMarkedEOY)
			{
				isMarkedEOY = 1;
				hasContent = 1;
			}
			container->data[container->top].t[3] = 1;
		}else if(i > lastYearOffset)
		{
			container->data[container->top].t[3] = 2;
		}
		
		// 4 end of page // before end of article
		if(!hasDifferneces(lastPageOffset,i))
		{
			if(!isMarkedEOP)
			{
				isMarkedEOP = 1;
				hasContent = 1;
			}
			container->data[container->top].t[4] = 1;
		}else if(i > lastPageOffset)
		{
			container->data[container->top].t[4] = 2;
		}
		
		// 5 end of page2 // before end of article
		if(!hasDifferneces(lastPageOffset2,i))
		{
			if(!isMarkedEOP2)
			{
				isMarkedEOP2 = 1;
				hasContent = 1;
			}
			container->data[container->top].t[5] = 1;
		}else if(i > lastPageOffset2)
		{
			container->data[container->top].t[5] = 2;
		}
		
		// 6 end of article (move down)

		// 7 end year before ack
		if(i>endYearBeforeAck && endYearBeforeAck)
		{
			container->data[container->top].t[7] = 2;
		}
		
		// 8 end year before table
		if(i>endYearBeforeTable && endYearBeforeTable)
		{
			container->data[container->top].t[8] = 2;
		}
		// 9 end year before ack or table
		if(i>endYearBeforeAckOrTable && endYearBeforeAckOrTable)
		{
			container->data[container->top].t[9] = 2;
		}

		// 10 end page before ack
		if(i>endPageBeforeAck && endPageBeforeAck)
		{
			container->data[container->top].t[10] = 2;
		}
		
		// 11 end page before table
		if(i>endPageBeforeTable && endPageBeforeTable)
		{
			container->data[container->top].t[11] = 2;
		}
		
		// 12 end page before ack or table
		if(i>endPageBeforeAckOrTable && endPageBeforeAckOrTable)
		{
			container->data[container->top].t[12] = 2;
		}
		// 13 end page2 before ack
		if(i>endPage2BeforeAck && endPage2BeforeAck)
		{
			container->data[container->top].t[13] = 2;
		}
		// 14 end page2 before table
		
		if(i>endPage2BeforeTable && endPage2BeforeTable)
		{
			container->data[container->top].t[14] = 2;
		}
		// 15 end page2 before ack or table
		if(i>endPage2BeforeAckOrTable && endPage2BeforeAckOrTable)
		{
			container->data[container->top].t[15] = 2;
		}

		// move up
		
		//////////////////////////////////////////////////////////////////////////

		// 6 end of article
		if(!hasDifferneces(cLen,i))
		{
			if(ENDCTNMAX <= container->top) // 
			{
				fprintf(stderr,"[ERROR] pool is full! %s %d",__FILE__,__LINE__);
			}
			container->data[container->top].t[6] = 1;
			container->data[container->top].offset = i;
			container->top++;
			if(ENDCTNMAX < container->top)
			{
				fprintf(stderr,"[WARNING] pool is full! %s %d",__FILE__,__LINE__);
			}
			return 1;
		}
		
		if(hasContent)
		{
			if(ENDCTNMAX <= container->top)
			{
				fprintf(stderr,"[ERROR] pool is full! %s %d",__FILE__,__LINE__);
			}
			container->data[container->top].offset = i;
			container->top++;
			if(ENDCTNMAX <= container->top)
			{
				fprintf(stderr,"[WARNING] pool is full! %s %d",__FILE__,__LINE__);
			}
			//if(container->top >= 200) printf("found!");
			//maxLen = container->top > maxLen ? container->top : maxLen;
		}
	}
	return 0;
}

//int getMaxLen(){return maxLen;}


int combineOffsets(endFeatureDataContainer *container)//combine nearly offsets and make sure 
{
	int j = 0;
	int lastOffset = container->data[0].offset;
	//introduce of settings
	// 0 empty for valued -- index of all offsets
	// 1 acknowledgements etc.
	// 2 table , he is figure ... (a list)
	// 3 end of year // before end of content
	// 4 end of page // before end of content
	// 5 end of page2 // before end of content
	// 6 end of article
	// 7 end year before ack
	// 8 end year before table
	// 9 end year before ack or table
	// 10 end page before ack
	// 11 end page before table
	// 12 end page before ack or table
	// 13 end page2 before ack
	// 14 end page2 before table
	// 15 end page2 before ack or table
	// 16 index of year
	// 17 index of page
	// 18 index of page2
	//for(int i=1;i<container->top-1;i++)
	for(int i=1;i<container->top;i++)
	{
		//container->data[j].offset = container->data[i].offset;
		//hasDifferneces(int dest,int src)
		if(!hasDifferneces(lastOffset,container->data[i].offset))
		{
			for(int k=1;k<ENDLEN;k++)
			{
				
				if(container->data[j].t[k] != container->data[i].t[k])
				{
					j++;
					container->data[j].offset = container->data[i].offset;
					container->data[j].t[0] = j;
					for(int z=1;z<ENDLEN;z++)
					{
						container->data[j].t[z] = container->data[i].t[z];
					}
				}
			}
		}else
		{
			j++;
			container->data[j].offset = container->data[i].offset;
			container->data[j].t[0] = j;
			for(int k=1;k<ENDLEN;k++)
			{
				container->data[j].t[k] = container->data[i].t[k];
			}
			lastOffset = container->data[i].offset;
		}
	}
	/*
	if(!hasDifferneces(lastOffset,getPclen()))
	{
		for(int k=0;k<ENDLEN;k++)
		{
			container->data[j].t[k] = container->data[container->top-1].t[k] || container->data[j].t[k];
		}
	}else
	{
		j++;
		container->data[j].offset = container->data[container->top-1].offset;
		for(int k=0;k<ENDLEN;k++)
		{
			container->data[j].t[k] = container->data[container->top-1].t[k];
		}
	}
	*/
	/*
	j++;
	container->data[j].offset = container->data[container->top-1].offset;
	for(int k=0;k<ENDLEN;k++)
	{
		container->data[j].t[k] = container->data[container->top-1].t[k];
	}*/
	
	
	container->top = j+1;
	return 1;
}

int makeSequenceForCombinedOffsets(endFeatureDataContainer *container)
{
	unsigned int max[ENDLEN];
	//unsigned int before[ENDLEN];
	unsigned int hasData[ENDLEN];
	int top = container->top;
	for(int i=0;i<ENDLEN;i++)
	{
		max[i] = 1;
		hasData[i] = 0;
	}
	for(int i=0;i<top;i++)
		for(int j=1;j<ENDLEN;j++) 
			if(container->data[i].t[j] != 0 ) hasData[j] = 1;
	
	for(int i=0;i<top;i++)
	{
		for(int j=1;j<ENDLEN;j++)
		{
			if(hasData[j])
			{
				if(container->data[i].t[j] == 0)
				{
					container->data[i].t[j] = max[j];
				}else
				{
					max[j]++;
					container->data[i].t[j] = max[j];
				}
			
			}
		}
		container->data[i].t[0] = i+1;
	}
	/*
	printf("X:\n");
	for(int i=0;i<top;i++)
	{
		for(int j=0;j<ENDLEN;j++)
		{
			printf("%3d ",container->data[i].t[j]);
		}
		printf("\n");
	}*/
	return 1;
}


int genNextDataForEndfeature(FILE *fp,endFeatureData fd,int start)
{
	int offset = fd.offset;
	int lmt = 200;
	
//	char *content = getPcontent();
//	int offend = getPclen();	
//	int finac = 1;// :1 is not ascii code  :0 is ascii code
	/*
	// follow is not ascii code for instance : References [ right References Manual false
	// ONLY FOR startness of references
	for(int i=offset;i<offend;i++ )
	{
		if(content[i] != ' ')
		{
			finac = fitPattern('a',content[i]) && fitPattern('a',content[i+1]) ? 0 : 1;
			if(!finac && fitPattern('n',content[i]))
			{
				finac = 1;
			}
			break;
		}
	}
	fprintf(fp,"%d:%d ",start++,finac);
	*/
	//fp = stdout;
	//for(;lmt<=1000;lmt+=150)
	//{
	lmt = -200;
	
	
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset2(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasYearafterTheOffset(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset0(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset1(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset2(offset,lmt)?1:0));
	
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset(offset,lmt) >= hasPPafterTheOffset(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset2(offset,lmt) >= hasPPafterTheOffset2(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasYearafterTheOffset(offset,lmt) >= hasYearafterTheOffset(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset0(offset,lmt) >= hasNameafterTheOffset0(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset1(offset,lmt) >= hasNameafterTheOffset1(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset2(offset,lmt) >= hasNameafterTheOffset2(offset,-lmt)));

	fprintf(fp,"%d:%d ",start++,asciiCodeDensity(offset,lmt) >= asciiCodeDensity(offset,-lmt));
	fprintf(fp,"%d:%d ",start++,dataDensity(offset,lmt) >= dataDensity(offset,-lmt));

	lmt = 200;
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasPPafterTheOffset2(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasYearafterTheOffset(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset0(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset1(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasNameafterTheOffset2(offset,lmt)?1:0));

	lmt = -1000;
	
	fprintf(fp,"%d:%d ",start++,(hasSeqOfTheOffset(offset,lmt)?1:0));
	fprintf(fp,"%d:%d ",start++,(hasSeqOfTheOffset2(offset,lmt)?1:0));	
	fprintf(fp,"%d:%d ",start++,(hasSeqOfTheOffset(offset,lmt) >= hasSeqOfTheOffset(offset,-lmt)));
	fprintf(fp,"%d:%d ",start++,(hasSeqOfTheOffset2(offset,lmt) >= hasSeqOfTheOffset2(offset,-lmt)));
	
	fprintf(fp,"%d:%d ",start++,asciiCodeDensity(offset,lmt) >= asciiCodeDensity(offset,-lmt));
	fprintf(fp,"%d:%d ",start++,dataDensity(offset,lmt) >= dataDensity(offset,-lmt));
	
	//}
	
	//32
	//fprintf(fp,"%d:%f ",start++,(double)offset/getPclen());
	//fprintf(fp,"%d:%f ",start++,(double)(getPclen()-offset)/offset);
	//rateWrite(fp,start,(double)offset/getPclen());
	//start+=5;
	
	for(int i=0;i<ENDCALLBACKLEN;i++)
	{
		//rateWrite(fp,start,(fd.offset == getPclen()) ? -1 :((double)fd.fid[i][0]/fd.offset)/
		//			((fd.fid[i][1]-fd.fid[i][0])/(getPclen()-fd.offset)));
		//start+=5; //TODO ADD 
		
		fprintf(fp,"%d:%d ",start++,fd.density[i][0][1] <= fd.density[i][0][0]);
		fprintf(fp,"%d:%d ",start++,fd.density[i][1][1] <= fd.density[i][1][0]);
		fprintf(fp,"%d:%d ",start++,fd.density[i][2][1] <= fd.density[i][2][0]);
		fprintf(fp,"%d:%d ",start++,fd.vari[1] <= fd.vari[0]);
		
		//fprintf(fp,"%d:%f ",start++,quot(fd.density[i][0][1],fd.density[i][0][0]));
		//fprintf(fp,"%d:%f ",start++,quot(fd.density[i][1][1],fd.density[i][1][0]));
		//fprintf(fp,"%d:%f ",start++,quot(fd.density[i][2][1],fd.density[i][2][0]));
		//fprintf(fp,"%d:%f ",start++,quot(fd.vari[1],fd.vari[0]));
		
		int wide = 6;
		rankWrite(fp,start,fd.seq[i][0],wide);
		start+=wide;

		rankWrite(fp,start,fd.seq[i][1],wide);
		start+=wide;
		
		rankWrite(fp,start,fd.seq[i][2],wide);
		start+=wide;
		
		rankWrite(fp,start,fd.seq[i][3],wide);
		start+=wide;

	}

	return start;
}




int prepareDensityData(void)
{
	int beforeData;
	int totalData;
	for(int i=0;i<mfdc.top;i++)
	{
		
		for(int j=0;j<ENDCALLBACKLEN;j++)
		{
			offsetStat(mfdc.data[i].offset,
					&totalData,&beforeData,endFunctionList[j]);
			mfdc.data[i].fid[j][0] = beforeData;
			mfdc.data[i].fid[j][1] = totalData;
		}
	}
	
	//density and etc
	
	printf(".");fflush(NULL);//TODO TIPS
	
	// adjacencyOffset setting ... 
	for(int i=0;i<mfdc.top;i++)
	{
		mfdc.data[i].adjacencyOffset[0] = 0;
		mfdc.data[i].adjacencyOffset[1] = getPclen();
		for(int j=0;j<mfdc.top;j++)
		{
			if(i == j) continue;
			if(mfdc.data[i].offset < mfdc.data[i].offset && mfdc.data[i].adjacencyOffset[0] < mfdc.data[i].offset)
				mfdc.data[i].adjacencyOffset[0] = mfdc.data[i].offset ;
			else if(mfdc.data[i].offset > mfdc.data[i].offset && mfdc.data[i].adjacencyOffset[1] > mfdc.data[i].offset)
				mfdc.data[i].adjacencyOffset[1] = mfdc.data[i].offset ;
		}
	}
	
	
	printf(".");fflush(NULL);//TODO TIPS
	
	int diff;
	int lmt;
	//density && variance setting ... 
	CloseKWD ckwd;
	for(int i=0;i<mfdc.top;i++)
	{
		for(int j=0;j<ENDCALLBACKLEN;j++)
		{
			mfdc.data[i].density[j][0][0] = (double)mfdc.data[i].fid[j][0]/NOTZERO(mfdc.data[i].offset);
			mfdc.data[i].density[j][0][1] = (double)(mfdc.data[i].fid[j][1]-mfdc.data[i].fid[j][0])/NOTZERO(getPclen()-mfdc.data[i].fid[j][0]);
			
			offsetBetweenStat(mfdc.data[i].offset,mfdc.data[i].adjacencyOffset[0],&diff,endFunctionList[j]);
			//mfdc.data[i].density[j][1][0] = (double)(mfdc.data[i].offset-mfdc.data[i].adjacencyOffset[0])/NOTZERO(diff);
			mfdc.data[i].density[j][1][0] = (double)diff/NOTZERO(VALUESDIFF(mfdc.data[i].offset,mfdc.data[i].adjacencyOffset[0]));
			
			offsetBetweenStat(mfdc.data[i].offset,mfdc.data[i].adjacencyOffset[1],&diff,endFunctionList[j]);
			mfdc.data[i].density[j][1][1] = (double)diff/NOTZERO(VALUESDIFF(mfdc.data[i].adjacencyOffset[1],mfdc.data[i].offset));
			
			
			lmt = 300;
			// prev && next 300* words
			offsetBetweenStat(mfdc.data[i].offset,LMB(mfdc.data[i].offset-lmt),&diff,endFunctionList[j]);
			mfdc.data[i].density[j][2][0] = (double)diff/NOTZERO(VALUESDIFF(mfdc.data[i].offset,LMB(mfdc.data[i].offset-lmt)));
			
			offsetBetweenStat(mfdc.data[i].offset,LMB(mfdc.data[i].offset+lmt),&diff,endFunctionList[j]);
			mfdc.data[i].density[j][1][1] = (double)diff/NOTZERO(VALUESDIFF(LMB(mfdc.data[i].offset+lmt),mfdc.data[i].offset));
			
			// variance 
			//int getCloseKWD(int offset,CloseKWD *closeKWD,int (*callback)(int offset,int limit));
			getCloseKWD(mfdc.data[i].offset,&ckwd,endFunctionList[j]);
			mfdc.data[i].vari[0] = getVariance(ckwd.prev,ckwd.pLen);
			mfdc.data[i].vari[1] = getVariance(ckwd.next,ckwd.nLen);
		}
	}
	
	
	// sequence setting ... 
	printf(".");fflush(NULL);//TODO TIPS
	for(int i=0;i<mfdc.top;i++)
	{
		for(int k=0;k<ENDCALLBACKLEN;k++)
		{
			for(int z=0;z<4;z++) mfdc.data[i].seq[k][z] = 1;
		}
		
		for(int j=0;j<mfdc.top;j++)
		{
			if(i==j) continue;
			for(int k=0;k<ENDCALLBACKLEN;k++)
			{
				if(mfdc.data[i].density[k][0][1]/NOTZERO(mfdc.data[i].density[k][0][0])
					< mfdc.data[j].density[k][0][1]/NOTZERO(mfdc.data[j].density[k][0][0]))
					mfdc.data[i].seq[k][0] ++;
				
				if(mfdc.data[i].density[k][1][1]/NOTZERO(mfdc.data[i].density[k][1][0])
					< mfdc.data[j].density[k][1][1]/NOTZERO(mfdc.data[j].density[k][1][0]))
					mfdc.data[i].seq[k][1] ++;
				
				if(mfdc.data[i].density[k][2][1]/NOTZERO(mfdc.data[i].density[k][2][0])
					< mfdc.data[j].density[k][2][1]/NOTZERO(mfdc.data[j].density[k][2][0]))
					mfdc.data[i].seq[k][2] ++;	
				
				if(mfdc.data[i].vari[1]/NOTZERO(mfdc.data[i].vari[0])
					< mfdc.data[j].vari[1]/NOTZERO(mfdc.data[j].vari[0]))
					mfdc.data[i].seq[k][3] ++;
			}
		}
	}
	
	return 1;
}




unsigned int getReferenceEndOffset()
{
	int i;
	int mypclen = getPclen();
	int in = 0;
	unsigned int myTags;
	unsigned int nowTag;
	if(mypclen == 0)
	{
		printf("error getting reference end of offset\n");
		getchar();
	}
	for(i=0;i<mypclen;i++)
	{
		myTags = *(getTags()+i);
		if(in && !myTags) return i;
		while((nowTag=tokenPop(&myTags)) > 0)
			if(nowTag == 1)
			{
				in = 1;
				break;
			}
				
	}
	return 0;
}

//TODO MAY USELESS ... 
//int getLastElement(int (*callback)(int offset,int limit));

int hasDifferneces(int dest,int src)
{
	if(src>=getPclen()) src = getPclen()-1;
	if(src<0) src = 0;
	if(dest<0) dest = 0;
	
	if(dest>=getPclen()) dest = getPclen()-1;
	int th = 0;
	char *content = getPcontent();

	if(dest < src)
	{
		int tmp = src;
		src = dest;
		dest = tmp;
	}
	if((dest - src) <= thresholdForDifferneces) return 0;
	for(int i=src;i<dest;i++)
	{
		// no ascii code
		//if(!(content[i]>='a'&&content[i]<='z')||(content[i]>='A'&&content[i]<='Z')) th++;
		if(content[i] != ' ' && content[i] != '\r' && content[i] != '\n') th++;
		
		if(th>2) return 1;
		/*
		if(fitPattern('a',content[i])&&fitPattern('a',content[i+1])) 
		{
			return 1;
			//th++;
			//if(th > 3) return 1;
			//i+=2;
		}*/
	}
	return 0;
}
/*
int hasDiffernecesH(int dest,int src)
{
	if(src>=getPclen()) src = getPclen()-1;
	if(src<0) src = 0;
	if(dest<0) dest = 0;
	
	if(dest>=getPclen()) dest = getPclen()-1;
	int th = 0;
	char *content = getPcontent();

	if(dest < src)
	{
		int tmp = src;
		src = dest;
		dest = tmp;
	}
	if((dest - src) <= thresholdForDifferneces) return 0;
	for(int i=src;i<dest;i++)
	{
		// no ascii code
		if(content[i] != ' ' && content[i] != '\r' && content[i] != '\n') return 1;
	}
	return 0;
}
*/

//TODO MY LORD ...
endFeatureDataContainer *getEndFeatureDataContainer(void)
{
	return &mfdc;
}

endFeatureData *getEndFeatureData(void)
{
	return &mfd;
}





