#include "crftctl.h"
#include "virtualcontent.h"
#include "dirTraversal.h"
#include "hftctl.h" // int getReferenceHeadOffset(void);
#include "eftfun.h" // int getReferenceEndOffset(void);
#include "strHandle.h" //spilitContent
#include "tokens.h"
#include "minEditDistance.h"
#include "crftfun.h"
#include "hftnpse.h"
#include "dict.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <strings.h>

#define SINGLEWORDLEN 256



FILE *fpTrain; // train.txt
FILE *fpTest; // test.txt



int filteredTokenId(int offset)
{
	int mypclen = getPclen();
	unsigned int myTags;
	unsigned int nowTag;
	if(offset >= mypclen) return 0;
	myTags = *(getTags()+offset);
	int finalTag = 0;
	while((nowTag=tokenPop(&myTags)) > 0)
	{
		//ignored: 
		// note 10
		// RefA 1
		// SinRef 2
		// tech 13
		if(nowTag != 1 && nowTag != 2 && nowTag != 10 ) // ingore RefA , SinRef , note
		{
			finalTag =  nowTag;
		}	
	}		
	return finalTag;
}


int ftEnQueue(pCNSQ Q,int *currentOffset,char *mpredeli)
{
	if(isFullQueue(Q)) return 0;

	int refAreaEnd = getReferenceEndOffset();
	char *content = getPcontent();
	char str[SINGLEWORDLEN];
	
	CrfNodeSnapshot crfNodeSnapshot;
	
	int isPublisher = 0;
	
	//spilitContent(char *dest,int dlen,const char *src,int len)
	if((crfNodeSnapshot.offset = spilitContent(str,SINGLEWORDLEN,
			content+(*currentOffset),
			refAreaEnd-(*currentOffset),
			&(crfNodeSnapshot.predeli),
			&(crfNodeSnapshot.nextdeli))) != 0)
	{
		int slen = strlen(str);
		sprintf(crfNodeSnapshot.str,"%s",str);
		crfNodeSnapshot.slen = slen;
		//int dval;
		int tkcheck;
		int hstkcheck = 0;
		char partStr[1024];
		int psI = 0;
		
		// abbr
		int abbrc = 0; // in Connect status
		int abbrl = 0; // abbr length
		//int abbrs = 0 ; // abbr start type
		
		
		for(int i=(*currentOffset);i<crfNodeSnapshot.offset+(*currentOffset);i++)
		{
			if(!isDelimiter(content[i]) && !hstkcheck)
			{
				tkcheck = i;
				//break;
				hstkcheck = 1;
			}
			if(DIGITLIKE(content[i]))
			{
				partStr[psI] = content[i];
				psI++;
				
			}else if(psI>0)
			{
				partStr[psI]='\0';
				isPublisher = isPublisher || isPublisherInDict(partStr); 
			}
			crfNodeSnapshot.quotflag = 0;
			
			switch(content[i])
			{
				case '\"':
					crfNodeSnapshot.quotflag ++;
					break;
				case '(':
					crfNodeSnapshot.pareSflag = 1;
					break;
				case ')':
					crfNodeSnapshot.pareEflag = 1;
					break;
				case '[':
					crfNodeSnapshot.sqbSflag = 1;
					break;
				case ']':
					crfNodeSnapshot.sqbEflag = 1;
					break;
				case '{':
					crfNodeSnapshot.braSflag = 1;
					break;
				case '}':
					crfNodeSnapshot.braEflag = 1;
					break;	
				
				case '.':
				case ',':
					// filter Abbreviation
					//if(!(abbrc && abbrl < 5 && abbrs))
					if(!(abbrc && abbrl < 6))
					//	crfNodeSnapshot.stopflag = 2;
						crfNodeSnapshot.stopflag = crfNodeSnapshot.stopflag == 2 ? 
										2: 
										1;
					break;
				
					
					//break;
				case '!':
				case '?':
					crfNodeSnapshot.stopflag = 2;
					break;
				
					
			}
			
			if(i == (*currentOffset)) // xxx. Aaa.
			{
				if(isAsciiCode(content[i]))
				{
					abbrl = 0;
					abbrc = 1;
					//abbrs = isUppercaseCode(content[i]) ? 1 : 0 ;
				}
			}else if(!isAsciiOrDigit(content[i-1]) && isAsciiCode(content[i])){
				abbrc = 1;
				abbrl = 0;
				//abbrs = isUppercaseCode(content[i]) ? 1 : 0 ;
			}else if(abbrc && (content[i]>='a' || content[i] <= 'z')){
				abbrl ++;
			}else
			{
				abbrc = 0;
				abbrl = 0;
			}
		}

		crfNodeSnapshot.token = filteredTokenId(tkcheck);//offsum+(offset+1)/2
		
		*currentOffset += crfNodeSnapshot.offset;

		int spstr[10];
		int splen = 0;
		
		spilitStr(str,slen,spstr,&splen); // spilit

		sprintf(crfNodeSnapshot.str,"%s",str);
		
		crfNodeSnapshot.mpredeli = isBlank(crfNodeSnapshot.predeli)?(*mpredeli):' ';
		crfNodeSnapshot.digitl = digitlen(str,slen);
		crfNodeSnapshot.puredigit = puredigit(str,slen);
		crfNodeSnapshot.dval = valofdigit(str,slen);
		crfNodeSnapshot.strtype = strfeature(str,slen);
		crfNodeSnapshot.yearlike = yearlike(str,slen);
		crfNodeSnapshot.monthlike = monthlike(str,slen);
		crfNodeSnapshot.volumnlike = vollkwd(str,slen);
		crfNodeSnapshot.pagelike = pagekwd(str,slen);
		crfNodeSnapshot.edsflag = edsFlag(str,slen);
		crfNodeSnapshot.speflag = specialFlag(str,slen);
		crfNodeSnapshot.procflag =  procFlag(str,slen);		
		for(int z=0;z<splen && (crfNodeSnapshot.procflag != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.procflag = procFlag(str+spstr[k],
								spstr[k+1]-spstr[k]);	
			}
			
		}
		//crfNodeSnapshot.namelike = hasNameafterTheOffset0((*currentOffset)
		//					-crfNodeSnapshot.offset-1,
		//					crfNodeSnapshot.offset+1);
		crfNodeSnapshot.namelike = namelike(str,slen,crfNodeSnapshot.nextdeli,
						crfNodeSnapshot.strtype);
		
		crfNodeSnapshot.isNameDict = isNameInDict(str,slen);
		for(int z=0;z<splen && (crfNodeSnapshot.isNameDict != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.isNameDict = isNameInDict(str+spstr[k],
								spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.rLastNameDict = rateLastNameInDict(str,slen);
		for(int z=0;z<splen && (crfNodeSnapshot.rLastNameDict != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.rLastNameDict = rateLastNameInDict(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.isCountryDict = isCountryInDict(str,slen);
		for(int z=0;z<splen && (crfNodeSnapshot.isCountryDict != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.isCountryDict = isCountryInDict(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.isFunWordDict = isFunWordInDict(str,slen);

		crfNodeSnapshot.isPlaceNameDict = isPlaceNameInDict(str,slen);
		for(int z=0;z<splen && (crfNodeSnapshot.isPlaceNameDict != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.isPlaceNameDict = isPlaceNameInDict(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.isPubliserDict = isPublisherInDict(str,slen) ||isPublisher;
		
		for(int z=0;z<splen && (crfNodeSnapshot.isPubliserDict != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.isPubliserDict = isPublisherInDict(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.isArticle = isArticle(str,slen);
		crfNodeSnapshot.deptflag = deptFlag(str,slen);
		crfNodeSnapshot.uniflag = uniFlag(str,slen);
		for(int z=0;z<splen && (crfNodeSnapshot.uniflag != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.uniflag = uniFlag(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.ltdflag = ltdFlag(str);
		for(int z=0;z<splen && (crfNodeSnapshot.ltdflag != 0);z++)
		{
			//str+flag[i],flag[i+1]-flag[i]
			for(int k=0;k<splen;k++)
			{
				crfNodeSnapshot.ltdflag = ltdFlag(str+spstr[k],
									spstr[k+1]-spstr[k]);	
			}
			
		}
		
		crfNodeSnapshot.domainflag = domainFlag(str);
		
		

		if(crfNodeSnapshot.puredigit)
		{
			int vh = valofdigit(str,slen/2);
			int vl = valofdigit(str+(slen/2),(slen+1)/2);
			if(vh == 0 || vl == 0) 	crfNodeSnapshot.imprnum = 0;
			else crfNodeSnapshot.imprnum = vl > vh ? 1 : -1;
		}else
			crfNodeSnapshot.imprnum = 0;
		
		if(!isBlank(crfNodeSnapshot.nextdeli)) *mpredeli = crfNodeSnapshot.nextdeli;
		
		//*currentOffset = crfNodeSnapshot.offset;
		enQueue(Q,crfNodeSnapshot);
		return 1;
	}else
		return 0;
}


pCrfNodeSnapshot ftDeQueue(pCNSQ Q)
{
	if(isEmptyQueue(Q)) return NULL;
	return deQueue(Q);
}

int genCRFSampleCtl(const char* fileName,int isDir)
{
	static int id = 1;
	FILE *fp; // local train
	int trainOrTest;
	int refAreaStart;
	//int refAreaEnd;
	int currentOffset;
	//char *content;
	char mpredeli=' ';
        if(isDir)
        {
                printf("ignore dir:%s\n",fileName);
                return 1;
        }
	if(rand()%2) //train is 50% and test is 50%
	{
		// train
		fp = fpTrain;
		trainOrTest = 1;
	}else
	{
		// test
		fp = fpTest;
		trainOrTest = 0;
	}
	
	CNSQ preCNSQ;
	CNSQ nextCNSQ;
	clearQueue(&preCNSQ);
	clearQueue(&nextCNSQ);
	pCrfNodeSnapshot pCNS;
	
	//
        printf("[%d] %s:%s",id,(trainOrTest?"train":"test"),fileName);
        fflush(NULL);
        
        // parse tag or etc ,move data to RAM
 	initContent();
	if(!parseFile(fileName))
	{
		fprintf(stderr,"[E] error parsing file : #%s#%s\n",fileName,__FILE__);
		return 0;
	}
	
	//
	refAreaStart = getReferenceHeadOffset();
	//refAreaEnd = getReferenceEndOffset();
	currentOffset = refAreaStart;

	//make queue full
	while(ftEnQueue(&nextCNSQ,&currentOffset,&mpredeli));
	
	
	int httpStatus = 0;
	//int httpTime = 0;
	
	int quotStatus[2];//  AT/NA IN/NIN OUT/NOUT
	int quotTime = 0;
	
	int pareStatus[3];// AT/NA IN/NIN OUT/NOUT 
	int paraFlag = 0;
	
	int sqbStatus[3];// AT/NAT IN/NIN OUT/NOUT 
	int sqbFlag = 0;
	
	int braStatus[3];// AT/NAT IN/NIN OUT/NOUT
	int braFlag = 0;
	
	
	// to make sure data is like this : )<<ignore this |from here to calculate>> ((()))
	int paraCache = 0; 
	int sqbCache = 0;
	int braCache = 0;
	
	int isbnEffect = 0;

	FILE *fexp;
	char exportFileName[1024];
	for(int z=3;z<18;z++)
	{
		sprintf(exportFileName,"res/%s",id2Token(z));
		fexp = fopen(exportFileName,"a");
		fseek(fexp, 0L, SEEK_END);
		fprintf(fexp,">>%s\n",fileName);
		fflush(NULL);
		fclose(fexp);
	}
	
	while((pCNS = ftDeQueue(&nextCNSQ)) != NULL)
	{
		sprintf(exportFileName,"res/%s",id2Token(pCNS->token));
		fexp = fopen(exportFileName,"a");
		fseek(fexp, 0L, SEEK_END);
		fprintf(fexp,"%s ~ [%d]::[%d]\n",pCNS->str,pCNS->isPubliserDict,pCNS->isPlaceNameDict|| pCNS->isCountryDict);
		fflush(NULL);
		fclose(fexp);
		// 0. PREPARE : FLAGS
		
		// 0.0 PAST ONE INFO && NEXT ONE INFO
		pCrfNodeSnapshot lpCNS = pastNElem(&preCNSQ,1);  // previous one node
		pCrfNodeSnapshot npCNS = nextNElem(&nextCNSQ,1); // next one node
		
		// 0.1 TRAVERSAL ALL PAST && NEXT INFO , GET FLAGS
		int thesisFlag = 0;
		int ltdFlag = 0;
		int edsFlag = 0;
		int uniFlag = 0;
		int mitFlag = 0;
		int groupFlag = 0;
		int pressFlag = 0;
		int confFlag = 0; // conferences
		int orgFlag = 0; // organization (as,etc.)
		int labFlag = 0; // 
		int techFlag = 0;
		int repFlag = 0;
		int procFlag = 0;
		//int resFlag = 0; // research
		
		int nextPDigit = 0; // next pure digit
		int domainFlag = 0;
		int domainNoStop = 1;
		int i;
		
		
		
		int stopEffect = 0; // XXXX XXXX XXXX
			// 2:  ',' 
			// 1:  '.','?','!'..  
		int stopEffectEA = 0;// ExceptAbbreviation
		
		// 0.1.1 NEXT 
		for(i=1;i < sizeQueue(&nextCNSQ) ; i++)
		{
			pCrfNodeSnapshot tCNS = (i==0)? pCNS : nextNElem(&nextCNSQ,i);
			//if(tCNS->ltdflag == 1 && i < 4) ltdFlag = 1;
			if(tCNS->edsflag == 1)
				edsFlag = 1;
			
				
			// effect : 1:'.''?''!'  2:','	
			if(i>0)
			{
				if(tCNS->stopflag  == 1)
				{
					stopEffectEA = (stopEffectEA == 2) ? 2 : 1;
					stopEffect = (stopEffect == 2) ? 2 : 1;
				}
				if(tCNS->stopflag  == 2)
				{
					stopEffect = 2;
					stopEffect = 2;
				}
			}
			
			if(tCNS->uniflag == 1 && ((stopEffectEA == 2)||(stopEffectEA == 0)))
				uniFlag = 1;
			
			if(stopEffectEA == 0 && (tCNS->speflag == 56)) //"group"
			{
				groupFlag = 1; // institute
			}
			
			
			if((stopEffectEA == 0 )&& ((tCNS->speflag == 90)||
							(tCNS->speflag == 79)||
							(tCNS->speflag == 60))) //"conference(s)"
			{
				confFlag = 1; // journal
			}
			
			if((stopEffectEA == 0) && ((tCNS->speflag == 30 )
						|| (tCNS->speflag == 31 )
						|| (tCNS->speflag == 32 ))) // publisher
			{
				pressFlag = 1;
			}
			
			if((stopEffectEA == 0) && ((tCNS->speflag == 77 )
						|| (tCNS->speflag == 78 ))) // publisher||institute
			{
				orgFlag = 1;
			}
			
			if((stopEffectEA == 0) && ((tCNS->speflag >= 71 )
						&&(tCNS->speflag <= 75 ))) // institute
			{
				labFlag = 1;
			}
			
			if((stopEffectEA == 0) && ((tCNS->speflag == 50 )
						|| (tCNS->speflag == 53 )
						|| (tCNS->speflag == 54 ))) // publisher
			{
				techFlag = 1;
			}
			
			if((stopEffectEA == 0) && ((tCNS->speflag == 55 ))) // publisher
			{
				repFlag = 1;
			}
			
			if((stopEffectEA == 0) && ((tCNS->procflag > 1))) // publisher
			{
				procFlag = 1;
			}
			
			if(stopEffectEA == 0 && (tCNS->speflag == 25)) thesisFlag = 1;
			
			
			// number of pure digit except 19xx/20xx
			if(tCNS->puredigit > 0 && tCNS->yearlike) nextPDigit ++ ;
			
			if(stopEffectEA == 0 && (tCNS->speflag == 70)) mitFlag = 1;
			
			if((stopEffectEA == 0 ||  stopEffectEA == 2)&& tCNS->ltdflag) ltdFlag = 1;
			
			
			if(!isBlank(tCNS->predeli) && (tCNS->predeli !=':')
							&& (tCNS->predeli !='/')
							&& (tCNS->predeli !='.')
							&& (tCNS->predeli !=',')) // for err
			{
				domainNoStop = 0;
			}
			
			if(domainNoStop && tCNS->domainflag)
			{
				domainFlag = 1;
			}
			
		}
		
		// 0.1.2 PREVIOUS
		httpStatus = 0;
		int inStatus = 0;
		int seqFlag = 0;
		
		int puredata = 1;
		
		paraFlag = 0;
		sqbFlag = 0;
		braFlag = 0;
		paraCache = 0; 
		sqbCache = 0;
		braCache = 0;
		
		stopEffect = 0;
		stopEffectEA = 0;
		domainNoStop = 1;
		for(i=0;i < sizeQueue(&preCNSQ) ; i++)
		{
			pCrfNodeSnapshot tCNS = (i==0) ? pCNS : pastNElem(&preCNSQ,i);
			
			if(i>0)
			{
				/*
				if(tCNS->stopflag  == 1 ) stopEffect = 1; 
				if(tCNS->stopflag  == 2 ) stopEffect = (stopEffect == 1) ? 1 : 2;
			
				if(tCNS->stopflag  == 1)
				{
					if(!((tCNS->slen <=4) && (tCNS->str[0]>='A')
							&&(tCNS->str[0]<='Z')))
						stopEffectEA = 1;
				} 
				if(tCNS->stopflag  == 2)
				{
					if(!((tCNS->slen <=4) && (tCNS->str[0]>='A')
							&&(tCNS->str[0]<='Z')))
						stopEffectEA = (stopEffectEA == 1) ? 1 : 2;
				}*/
				if(tCNS->stopflag  == 1)
				{
					stopEffectEA = (stopEffectEA == 2) ? 2 : 1;
					stopEffect = (stopEffect == 2) ? 2 : 1;
				}
				if(tCNS->stopflag  == 2)
				{
					stopEffect = 2;
					stopEffect = 2;
				}
				
			}
	
			if(tCNS->edsflag == 1) edsFlag = 1;
	
			if(!tCNS->puredigit && tCNS->strtype!=4) puredata = 0;
			
			if(puredata && tCNS->speflag == 40) isbnEffect = 1;
				
			if(!isBlank(tCNS->predeli) && (tCNS->predeli !=':')
							&& (tCNS->predeli !='/')
							&& (tCNS->predeli !='.')
							&& (tCNS->predeli !=',')) // for err
			{
				domainNoStop = 0;
			}

			if((tCNS->speflag == 20|| tCNS->speflag == 21)
					&&!isBlank(tCNS->nextdeli) 
						&& domainNoStop ) 
				httpStatus = 1;
			if((tCNS->speflag == 22)&(tCNS->nextdeli !=':')&&domainNoStop)
				httpStatus = 1;
			
			if(tCNS->procflag == 1 && (stopEffect < 2 )) inStatus = 1; // In xxx , xx
			
			////////////////////////////////////////////////////////////////////
			if(stopEffect == 0 && ((tCNS->deptflag == 1) 
						||(tCNS->speflag == 56))) //"group" institute
			{
				groupFlag = 2; // institute
			}
			
			
			if((stopEffect == 0 )&& ((tCNS->speflag == 90)||
							(tCNS->speflag == 79)||
							(tCNS->speflag == 60))) //"conference(s)"
			{
				confFlag = 2; // journal
			}
			
			if((stopEffect == 0) && ((tCNS->speflag == 30 )
						|| (tCNS->speflag == 31 )
						|| (tCNS->speflag == 32 ))) // publisher
			{
				pressFlag = 2;
			}
			
			if((stopEffect == 0) && ((tCNS->speflag == 77 )
						|| (tCNS->speflag == 78 ))) // publisher||institute
			{
				orgFlag = 2;
			}
			
			if((stopEffect == 0) && ((tCNS->speflag >= 71 )
						&&(tCNS->speflag <= 75 ))) // institute
			{
				labFlag = 2;
			}
			
			if((stopEffect == 0) && ((tCNS->speflag == 50 )
						|| (tCNS->speflag == 53 )
						|| (tCNS->speflag == 54 ))) // publisher
			{
				techFlag = 2;
			}
			
			if((stopEffect == 0) && ((tCNS->speflag == 55 ))) // publisher
			{
				repFlag = 2;
			}
			////////////////////////////////////////////////////////////////////		

			
			// couple delimiter
			if(tCNS->pareEflag) paraCache++;
			if(tCNS->pareSflag)
			{
				paraFlag = paraFlag + 1 - paraCache;
				paraCache = 0;
			}
			if(tCNS->sqbEflag) sqbCache++;
			if(tCNS->sqbSflag)
			{
				sqbFlag = sqbFlag + 1 - sqbCache;
				sqbCache = 0;
			}
			if(tCNS->braEflag) braCache++;
			if(tCNS->braSflag)
			{
				braFlag = braFlag + 1 - braCache;
				braCache = 0;
			}
			
			if(i<3 && tCNS->sqbEflag)
			{
				seqFlag = 1;
			}
		}

		pareStatus[0] = paraFlag > 0 ;
		sqbStatus[0] = sqbFlag > 0;
		braStatus[0] = braFlag > 0;

		
		
		
		// 0.2 SOME MIX FEATURE
		// \"
		quotTime--;
		
		if(quotTime <= 0)
		{
			quotStatus[0] = 0;
			quotTime = 0;
		}else
		{
			quotStatus[0] = 1;
		}
		quotStatus[1] = 0;
		quotStatus[2] = 0;
		if(pCNS->quotflag%2 == 1)
		{
			quotStatus[0] = !quotStatus[0];
			if(quotStatus[0])
				quotTime = 10;
			else
				quotTime = -10;
		}else if(pCNS->quotflag > 0 && pCNS->quotflag%2 == 0)
		{
			quotStatus[1] = quotStatus[2] = 1;
			quotTime = 0;
		}
		if(quotTime == 10) quotStatus[1] =  1;
		else if(quotTime == -10) quotStatus[2] = 1;
		//((quotTime==10||quotTime==-10)?(quotStatus?"IN":"OUT"):(quotStatus?"AT":"NA"))
		
		
		//////////////////////////////////////////////////////////////////////
		
		pareStatus[1] = pareStatus[2] = 0;
		// para
		if(pCNS->pareSflag == 1)
		{
			pareStatus[1] = 1;
		}
		if(pCNS->pareEflag == 1)
		{
			pareStatus[2] = 1;
		}
		
		//////////////////////////////////////////////////////////////////////
		// sqb
		sqbStatus[1] = sqbStatus[2] = 0;
		if(pCNS->sqbSflag == 1)
		{
			sqbStatus[1] = 1;
		}
		
		if(pCNS->sqbEflag == 1)
		{
			sqbStatus[2] = 1;
		}
		
		
		//////////////////////////////////////////////////////////////////////
		// bra
		braStatus[1] = braStatus[2] = 0;
		if(pCNS->braSflag == 1)
		{
			braStatus[1] = 1;
		}
		
		if(pCNS->braEflag == 1)
		{
			braStatus[2] = 1;
		}
		
		// xxxx ( abcd )  or xxxx , (abcd)
		// in template
		int contentConnect = 0;
		if(pareStatus[0] == 1)
		{
			for(i=1;i < sizeQueue(&preCNSQ) ; i++)
			{
				pCrfNodeSnapshot tCNS = pastNElem(&preCNSQ,i);
				if(tCNS->predeli == '(')
				{
					tCNS = pastNElem(&preCNSQ,i+1);
					if(tCNS->nextdeli == '(') contentConnect = 1;
					break;
				}
			}
		}
		
		
		// combined string , combine with next string
		char combinedStr[1024];
		sprintf(combinedStr,"%s%s",pCNS->str,((npCNS==NULL)?"":npCNS->str));
		
		
		// 1. OUTPUT : PRINT FEATURES
		
		// basic
		// 0: string data
		fprintf(fp,"%s\t",pCNS->str); 
		
		// 1: length of string data 0,1,2,3,4,5,6 >6
		fprintf(fp,"串长/%d\t",pCNS->slen<7?pCNS->slen:9); 

		
		// base::string
		// 2: string type 0:AAA 1:aaa 2:Aaa 3:aAa 4:123
		fprintf(fp,"类型/%d\t",pCNS->strtype);
		
		// 3: prefix 
		fprintf(fp,"前缀/%c/%c\t",tolower(pCNS->str[0]),(pCNS->slen>1)?tolower(pCNS->str[1]):'X');
		
		// 4: sufix 
		fprintf(fp,"后缀/%c/%c\t",(pCNS->slen>1)?tolower(pCNS->str[pCNS->slen-2]):'X',
					tolower(pCNS->str[pCNS->slen-1]));
		
		
		// base::digit
		// 5: digit value  > 0 ?
		fprintf(fp,"数值/%d\t",pCNS->dval > 0 );
		
		// 6: digit bigger than previours one
		fprintf(fp,"比前值长/%d\t",lpCNS==NULL?-1:(lpCNS->dval == 0?-1:(pCNS->dval > lpCNS->dval)));
		
		// 7: next one is bigger than this digit 
		fprintf(fp,"比后值短/%d\t",npCNS==NULL?-1:(npCNS->dval == 0?-1:(npCNS->dval > pCNS->dval)));
		
		// 8: digit a improve digit ? 123456 456 > 123
		fprintf(fp,"升数/%d\t",pCNS->imprnum );
		
		// 9: is pure digit ? see 'I' 'l' 'O' etc. as digit
		fprintf(fp,"纯数/%d\t",pCNS->puredigit);
		
		
		// base::delimiter
		// 10: last delimiter
		fprintf(fp,"前分隔符/%d\t",pCNS->predeli);
		
		// 11: last useful delimiter
		fprintf(fp,"前有用分隔符/%d\t",pCNS->mpredeli);
		
		// 12: next delimiter
		fprintf(fp,"后分隔符/%d\t",pCNS->nextdeli);
		
		
		
		// base::string orthographic
		// 13: year like || month like >> time like
		fprintf(fp,"似年/%d\t",(pCNS->yearlike > 0 )|| (pCNS->monthlike > 0 ));
		// 14: volume like  start of volume ?  vol. X num. no. number
		fprintf(fp,"似卷/");
		if(pCNS->volumnlike < 3) fprintf(fp,"%d\t",pCNS->volumnlike);
		else if(!isBlank(pCNS->nextdeli)) fprintf(fp,"%d\t",pCNS->volumnlike-2);
		else fprintf(fp,"0\t");
			
		// 15: page like
		fprintf(fp,"似页/%d\t",pCNS->pagelike);

		
		// base::dict
		// 16: name in dict
		fprintf(fp,"名/%d\t",pCNS->isNameDict || pCNS->rLastNameDict > 0);
		
		// 17: place in dict
		pCNS->isPlaceNameDict = pCNS->isPlaceNameDict || isPlaceNameInDict(combinedStr);		
		pCNS->isCountryDict = pCNS->isCountryDict || isCountryInDict(combinedStr);
		
		fprintf(fp,"地/%d\t",pCNS->isPlaceNameDict>0 || pCNS->isCountryDict > 0);
		
		// 18: publisher in dict
		pCNS->isPubliserDict = pCNS->isPubliserDict || isPublisherInDict(combinedStr);
		fprintf(fp,"出版社/%d\t",pCNS->isPubliserDict);
		
		// 19: fun word in dict 
		fprintf(fp,"功能词汇/%d\t",pCNS->isFunWordDict);

		// base::couple flag ststus
		
		// 20,21,22 quots AT IN OUT
		fprintf(fp,"引号1/%d\t",quotStatus[0]);
		fprintf(fp,"引号2/%d\t",quotStatus[1]);
		fprintf(fp,"引号3/%d\t",quotStatus[2]);
		
		
		// 23,24,25 Parentheses AT IN OUT
		fprintf(fp,"花括号1/%d\t",pareStatus[0]);
		fprintf(fp,"花括号2/%d\t",pareStatus[1]);
		fprintf(fp,"花括号3/%d\t",pareStatus[2]);
		
		// 26,27,28 Square brackets AT IN OUT
		fprintf(fp,"方括号1/%d\t",sqbStatus[0]);
		fprintf(fp,"方括号2/%d\t",sqbStatus[1]);
		fprintf(fp,"方括号3/%d\t",sqbStatus[2]);
		
		// 29,30,31 Braces AT IN OUT
		fprintf(fp,"括号1/%d\t",braStatus[0]);
		fprintf(fp,"括号2/%d\t",braStatus[1]);
		fprintf(fp,"括号3/%d\t",braStatus[2]);
		
		// base::flags
		
		// 32 basic flags
		// special flag (mixed)
		fprintf(fp,"特殊标记/%d\t",pCNS->speflag);
		
		// 33 stop flag  && effect
		fprintf(fp,"停止/括号1/括号2/括号3/");  
		fprintf(fp,"%d/%d/%d/%d\t",pCNS->stopflag,
					(quotStatus[0]||pareStatus[0]||sqbStatus[0]||braStatus[0]),
					(quotStatus[1]||pareStatus[1]||sqbStatus[1]||braStatus[1]),
					(quotStatus[2]||pareStatus[2]||sqbStatus[2]||braStatus[2]));
		
		// 34 eds flag
		fprintf(fp,"编辑标记");
		fprintf(fp,"%d\t",edsFlag);
		
		// 35: name like
		fprintf(fp,"似名/");
		fprintf(fp,"%d\t",pCNS->namelike);
		
		
		// extend::flags effect
		// 36 number of next pure digit
		fprintf(fp,"随后之数/");
		fprintf(fp,"%d\t",nextPDigit);
		
		
		// 37 http effect
		fprintf(fp,"URL态/");
		fprintf(fp,"%d\t",httpStatus);
		
		// 38
		fprintf(fp,"DOM态/");
		fprintf(fp,"%d\t",domainFlag);
		
		
		// extend::mix effect
		// 39 [abc def] author @ abc
		fprintf(fp,"组合顺序前/");
		if(pCNS->mpredeli == '[' && npCNS != NULL)
		{
			if(npCNS->nextdeli == ']') fprintf(fp,"1\t");
			else fprintf(fp,"0\t");
		}else
			fprintf(fp,"0\t");
		
		// [abc def] author @ def
		fprintf(fp,"组合顺序后/");
		if(pCNS->nextdeli == ']' && seqFlag == 1)
		{
			fprintf(fp,"1\t");
		}else
			fprintf(fp,"0\t");
		
		// 40 article xxxx, A process of ...
		fprintf(fp,"A终/");
		fprintf(fp,"%d/%d\t",pCNS->isArticle,pCNS->stopflag);
		
		// 41 ph D  str cmp
		fprintf(fp,"PhD/");
		if((strcasecmp("ph",pCNS->str)== 0 && npCNS->str[0] == 'D')
			|| strcasecmp("phD",pCNS->str)== 0)
			fprintf(fp,"1\t");
		else
			fprintf(fp,"0\t");
		
		// 42 xxx thesis thesis : 25
		fprintf(fp,"Thesis/");
		fprintf(fp,"%d\t",thesisFlag);
		
		// 43 inc ltd limited  : ltdflag 1 2 3
		fprintf(fp,"Inc_Ltd_Limit/");
		fprintf(fp,"%d\t",ltdFlag);
		
		// 44 45 46 ACM / ICPC / IEEE
		fprintf(fp,"ACM_ICPC_IEEE/");
		fprintf(fp,"%d\t",pCNS->speflag == 1); // ISO
		fprintf(fp,"%d\t",pCNS->speflag == 2); // IEEE
		fprintf(fp,"%d\t",pCNS->speflag == 3); // ACM
		
		// 47 CNAC AECSA SRCD ...
		fprintf(fp,"CNAC/"); 
		fprintf(fp,"%d\t",pCNS->strtype == 0 &&  pCNS->slen < 6 && pCNS->slen > 2);
		
		
		// 48 technical report
		fprintf(fp,"TR/");
		fprintf(fp,"%d\t",techFlag);
		fprintf(fp,"%d\t",repFlag);
		
		
		// 49 MIT
		fprintf(fp,"MIT/");
		fprintf(fp,"%d\t",mitFlag);
		
		// 50 51 52 university of 
		fprintf(fp,"UNIVERSITY/");
		fprintf(fp,"%d\t",pCNS->uniflag);
		
		fprintf(fp,"UNI_EFFECT/");
		fprintf(fp,"%d\t",uniFlag);
		
		//fprintf(fp,"%d/%d\t",pCNS->predeli,pCNS->uniflag);
		fprintf(fp,"前终/");
		fprintf(fp,"%d/%d\t",lpCNS == NULL ? 0 : lpCNS->stopflag,pCNS->uniflag);
		
		// (August 1-2 2013)
		// how about ?
		
		// 53 xxxx , (adfadf) or xxxx (adfasf)
		fprintf(fp,"内连/");
		fprintf(fp,"%d\t",contentConnect);
		
		// rfc || request (in tr) 
		//fprintf(fp,"%d\t",rfc);
		
		// 54 55 56 57 58 59 in proceedings of
		// 54
		fprintf(fp,"pIN/");
		fprintf(fp,"%d\t",pCNS->procflag == 1); // In point
		// 55
		fprintf(fp,"pIn/name/");
		fprintf(fp,"%d\t",(pCNS->procflag == 1) && 
				((npCNS->namelike)||(npCNS->isNameDict )
					|| (npCNS->rLastNameDict > 0)));
		// 56
		fprintf(fp,"sIN/");
		fprintf(fp,"%d\t",inStatus);
		// 57
		fprintf(fp,"In/Name/");
		fprintf(fp,"%d\t",inStatus && ((pCNS->namelike)||(pCNS->isNameDict ) || (pCNS->rLastNameDict > 0)));
		
		// 58
		fprintf(fp,"InEffect/");
		fprintf(fp,"%d\t",(pCNS->procflag == 1) || procFlag);
		// 59
		fprintf(fp,"InEffect/ExIn/");
		fprintf(fp,"%d\t",procFlag);
		
		// 60  department of / dept. of
		fprintf(fp,"DeptOf/"); 
		fprintf(fp,"%d\t",pCNS->deptflag);
		
		// 61 press
		fprintf(fp,"Press/");
		fprintf(fp,"%d\t",pressFlag);
		
		
		// 62 conf / journal
		fprintf(fp,"Journal/");
		fprintf(fp,"%d\t",confFlag);

		
		// 63 org
		fprintf(fp,"org/");
		fprintf(fp,"%d\t",orgFlag);
		
		// 64 group
		fprintf(fp,"group/");
		fprintf(fp,"%d\t",groupFlag);
		
		// 65 lib ins/pub
		fprintf(fp,"lib/");
		fprintf(fp,"%d\t",labFlag);
		
		// 66 isbn
		fprintf(fp,"eIsbn/");
		fprintf(fp,"%d\t",isbnEffect);
		
		// 67 group lab or dept
		fprintf(fp,"GLD/");
		fprintf(fp,"%d\t",labFlag > 0 ||groupFlag > 0 || groupFlag > 0);
		
		
		// 68 eds point
		fprintf(fp,"pED/");
		fprintf(fp,"%d/%d\t",pCNS->edsflag,pCNS->nextdeli);// 
		
		
		// 69
		fprintf(fp,"univ/lab/");
		fprintf(fp,"%d\t",uniFlag||labFlag); // university , 
		
		
		// 70 et , al
		fprintf(fp,"et/al/");
		if(lpCNS != NULL && npCNS != NULL)
		{
			if((strcmp(lpCNS->str,"et")==0) && (strcmp(pCNS->str,"al")==0))
				fprintf(fp,"1\t");
			else if((strcmp(pCNS->str,"et")==0) && (strcmp(npCNS->str,"al")==0))
				fprintf(fp,"2\t");
			else 
				fprintf(fp,"0\t");
		}else
			fprintf(fp,"0\t");
		
		
		
		// 71
		// jump to note : Available ... Submitted Unpublished
		fprintf(fp,"Available/");
		if(lpCNS != NULL && npCNS != NULL)
		{
			if(((strcmp(pCNS->str,"Available") == 0)||
				(strcmp(pCNS->str,"Submitted")== 0)||
				(strcmp(pCNS->str,"Unpublished")== 0)||
				((strcmp(pCNS->str,"To")== 0) && (strcmp(npCNS->str,"appear")== 0)))
				&& lpCNS->stopflag != 0)
			{
				fprintf(fp,"1\t");
			}else
				fprintf(fp,"0\t");
			
		}else
			fprintf(fp,"0\t");
		// note start
		
		
		
		
		
		
		// 2. END : PRINT RESULT
		if(lpCNS != NULL && npCNS != NULL)
		{
			if((lpCNS->token == 3 && npCNS->token == 3) || 
				(lpCNS->token == 6 && npCNS->token == 6))
				fprintf(fp,"%s\n",id2Token(lpCNS->token));
			else
				fprintf(fp,"%s\n",pCNS->token == 0 ? "other":id2Token(pCNS->token));	
		}else
			fprintf(fp,"%s\n",pCNS->token == 0 ? "other":id2Token(pCNS->token));
		
		//enQueue && store
		enQueueWithDrop(&preCNSQ,*pCNS);
		ftEnQueue(&nextCNSQ,&currentOffset,&mpredeli);
	}
	
	//
	id++;
        cleanContent();
        printf(" ok\n");
        return 1;
}


int genCRFSample(const char *path)
{
	FILE *fp;
	srand((unsigned int)time(NULL));
	
	//open train.data
	if((fp = fopen("res/train.data","w")) == NULL)
	{
		fprintf(stderr,"error in opening train.data\n");
		return 0;
	}
	setCRFTrainFile(fp);
	
	//open test.data
	if((fp = fopen("res/test.data","w")) == NULL)
	{
		fprintf(stderr,"error in opening train.data\n");
		return 0;
	}
	setCRFTestFile(fp);
	
	// do traversal
	dirTraversal(path,1,genCRFSampleCtl);
	
	
	//close train.data
	fclose(getCRFTrainFile());
	setCRFTrainFile(NULL);
	//close test.data
	fclose(getCRFTestFile());
	setCRFTestFile(NULL);
	return 1;
}


void setCRFTrainFile(FILE *fp){ fpTrain = fp;}
void setCRFTestFile(FILE *fp){fpTest = fp;}
FILE * getCRFTrainFile(){return fpTrain;}
FILE * getCRFTestFile(){return fpTest;}


