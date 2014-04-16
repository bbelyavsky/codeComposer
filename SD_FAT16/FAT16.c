//-------------------------------------------------------------------------
/*FAT16.C - LINO TECH
  Designed by Carter
  2008-03-19
*/

#include 	<stdint.h>

#include	"FAT16.h"
#include	"sd.h"
//------------------------------------------------------------------------
#define	SEC_Size				512
#define	MBR_Sector				0				//���Ե�ַ
#define	FAT_Sector				0				//�߼���ַ
//-------------------------------------------------------------------------
uint8_t		BUFFER[SEC_Size];
uint8_t		PB_RelativeSector;
uint16_t	BPB_BytesPerSec;
uint8_t		BPB_SecPerClus;
uint16_t	BPB_RsvdSecCnt;
uint8_t		BPB_NumFATs;
uint16_t	BPB_RootEntCnt;
uint16_t	BPB_TotSec16;
uint16_t	BPB_FATSz16;							//FATռ�õ�sectors
uint32_t	BPB_HiddSec;
//-------------------------------------------------------------------------
uint8_t ReadSector(uint32_t sector, uint8_t* buffer )
{
    int8 stat;
    
    hwInterface sdNow;  
    sdNow.sectorCount = 1;
    stat = sd_readSector(&sdNow,sector,buffer,512);
    
    if(stat==0)
    {
        return SD_SUCC;
    }
    else
    {
        return SD_FAIL;
    };
}
uint8_t WriteSector(uint32_t sector, uint8_t* buffer)
{
    hwInterface sdNow;  
    sdNow.sectorCount = 1;
    sd_writeSector(&sdNow,sector,buffer);
    
    return 0;
}

uint8_t ReadBlock(uint32_t LBA){					//���Ե�ַ��һ������
	if(ReadSector(LBA,BUFFER)!=0)return SD_FAIL;
	return SD_SUCC;
}
//-------------------------------------------------------------------------
uint8_t WriteBlock(uint32_t LBA){					//���Ե�ַдһ������
	if(WriteSector(LBA,BUFFER)!=0)return SD_FAIL;
	return SD_SUCC;
}
//-------------------------------------------------------------------------
uint8_t ReadFatBlock(uint32_t Add){					//�߼���ַ��һ������
//	return ReadBlock(Add+PB_RelativeSector);  //for HardDisk
  	return ReadBlock(Add+BPB_HiddSec);
}
//-------------------------------------------------------------------------
uint8_t WriteFatBlock(uint32_t Add){				//�߼���ַдһ������
//	return WriteBlock(Add+PB_RelativeSector);  //for HardDisk
    return WriteBlock(Add+BPB_HiddSec);
}
//-------------------------------------------------------------------------
void CopyBytes(uint8_t *ps,uint8_t *pd,uint16_t size){	//�ڴ濽��
	for(;size;size--)*pd++=*ps++;
}
//-------------------------------------------------------------------------
uint8_t IsEqual(uint8_t *pa,uint8_t *pb,uint8_t size){	//�ڴ�Ƚ�
	for(;size;size--)if(*pa++!=*pb++)return 0;
	return 1;
}
//-------------------------------------------------------------------------
void EmptyBytes(uint8_t *pd,uint16_t size){			//�ڴ����
	for(;size;size--)*pd++ =0;
}
//-------------------------------------------------------------------------
uint8_t ReadMBR(void){									//��ȡMBR���ݽṹ
	uint8_t ok;
	FAT_MBR * MBR=(FAT_MBR*)BUFFER;
	ok=ReadBlock(MBR_Sector);
	if(ok==SD_FAIL)return SD_FAIL;
	if(MBR->MBR_Signature!=0xAA55)return SD_FAIL;		//����Ч��־
		
	//��ȡ����
	PB_RelativeSector=MBR->MBR_pb[0].PB_RelativeSector;//���߼���ַ����Ե�ַ��ƫ��
	return SD_SUCC;
}
//-------------------------------------------------------------------------
uint8_t ReadBPB(void){									//��ȡBPB���ݽṹ
	uint8_t ok;
	FAT_BPB * BPB=(FAT_BPB*)BUFFER;
//	ok=ReadFatBlock(FAT_Sector);
   	ok=ReadBlock(FAT_Sector);
	if(ok==SD_FAIL)return SD_FAIL;
	
	//��ȡ����
	BPB_BytesPerSec = BPB->BPB_BytesPerSec;
	BPB_SecPerClus = BPB->BPB_SecPerClus;
	BPB_RsvdSecCnt = BPB->BPB_RsvdSecCnt;
	BPB_NumFATs = BPB->BPB_NumFATs;
	BPB_RootEntCnt = BPB->BPB_RootEntCnt;
	BPB_TotSec16 = BPB->BPB_TotSec16;
	BPB_FATSz16 = BPB->BPB_FATSz16;
	BPB_HiddSec = BPB->BPB_HiddSec;
	return SD_SUCC;
}
//-------------------------------------------------------------------------
uint32_t DirStartSec(void){							//��ȡ��Ŀ¼��ʼ������
	return BPB_RsvdSecCnt+BPB_NumFATs*BPB_FATSz16;
}
//-------------------------------------------------------------------------
uint16_t GetDirSecCount(void){							//Ŀ¼��ռ�õ�������
	return BPB_RootEntCnt*32/BPB_BytesPerSec;
}
//-------------------------------------------------------------------------
uint32_t DataStartSec(void){							//��ȡ��������ʼ������
	return DirStartSec()+GetDirSecCount();
}
//-------------------------------------------------------------------------
uint32_t ClusConvLBA(uint16_t ClusID){					//��ȡһ���صĿ�ʼ����
	return DataStartSec()+BPB_SecPerClus*(ClusID-2);
}
//-------------------------------------------------------------------------
uint16_t ReadFAT(uint16_t Index){						//��ȡ�ļ�������ָ����
	uint16_t *RAM=(uint16_t*)BUFFER;
	uint32_t SecID;
	
	SecID=BPB_RsvdSecCnt+Index/256;
	ReadFatBlock(SecID);
	return RAM[Index%256];
}
//-------------------------------------------------------------------------
void WriteFAT(uint16_t Index,uint16_t Value){			//д�ļ�������ָ����
	uint16_t *RAM=(uint16_t*)BUFFER;
	uint32_t SecID;
	
	SecID=BPB_RsvdSecCnt+Index/256;
	ReadFatBlock(SecID);
	RAM[Index%256]=Value;
	WriteFatBlock(SecID);
}
//-------------------------------------------------------------------------
uint16_t GetEmptyDIR(void){							//��ȡ��Ŀ¼�п���ʹ�õ�һ��
	uint16_t i,DirSecCut,DirStart,m,ID=0;
	
	DirSecCut=GetDirSecCount();
	DirStart=DirStartSec();
	for(i=0;i<DirSecCut;i++){
		ReadFatBlock(DirStart+i);
		for(m=0;m<16;m++){
			if(BUFFER[m*32]==0)return ID;
			if(BUFFER[m*32]==0xe5)return ID;
			ID++;
		}
	}
	return ID;
}
//-------------------------------------------------------------------------
//��ú��ļ�����Ӧ��Ŀ¼
uint8_t GetFileID(uint8_t Name[11],DIR *ID,uint16_t *pIndex){
	uint16_t  i,DirSecCut,DirStart,m;
	
	DirSecCut = GetDirSecCount();
	DirStart = DirStartSec();
	for(i=0,*pIndex=0;i<DirSecCut;i++){
		ReadFatBlock(DirStart+i);
		for(m=0;m<16;m++){
			if(IsEqual(Name,(uint8_t *)&((DIR*)&BUFFER[m*32])->FileName,11)){
				*ID = *((DIR*)&BUFFER[m*32]);
				return 1; 						//�ҵ���Ӧ��Ŀ¼��,����1.
			}
			(*pIndex)++;
		}
	}
	return 0; 									//û���ҵ���Ӧ��Ŀ¼��,����0.
}
//-------------------------------------------------------------------------
uint16_t GetNextFAT(void){							//��ȡһ���յ�FAT��
	uint16_t FAT_Count,i;
	FAT_Count=BPB_FATSz16*256; 						//FAT��������
	for(i=0;i<FAT_Count;i++){
		if(ReadFAT(i)==0)return i;
	}
	return 0;
}
//-------------------------------------------------------------------------
void ReadDIR(uint16_t Index, DIR* Value){			//��ȡ��Ŀ¼��ָ����
	uint32_t LBA = DirStartSec()+Index/16;
	ReadFatBlock(LBA);
	CopyBytes((uint8_t *)&BUFFER[(Index%16)*32],(uint8_t *)Value,32);
}
//-------------------------------------------------------------------------
void WriteDIR(uint16_t Index, DIR* Value){			//д��Ŀ¼��ָ����
	uint32_t LBA = DirStartSec()+Index/16;
	ReadFatBlock(LBA);
	CopyBytes((uint8_t *)Value,(uint8_t *)&BUFFER[(Index%16)*32],32);
	WriteFatBlock(LBA);
}
//-------------------------------------------------------------------------
void CopyFAT(void){						//�����ļ������,ʹ��ͱ���һ��
	uint16_t i;

	for(i=0;i<BPB_FATSz16;i++){
		ReadFatBlock(BPB_RsvdSecCnt+i);
		WriteFatBlock(BPB_RsvdSecCnt+BPB_FATSz16+i);
	}
}
//-------------------------------------------------------------------------
uint8_t CreateFile(uint8_t *Name,uint32_t Size){	//����һ�����ļ�
	uint16_t ClusID, ClusNum, ClusNext, i,dirIndex;
	DIR FileDir;
	
	if(GetFileID(Name,&FileDir,&dirIndex)==1)return SD_FAIL;	//�ļ��Ѵ���
	
	ClusNum=Size/(BPB_SecPerClus*BPB_BytesPerSec)+1;
	
	EmptyBytes((uint8_t *)&FileDir,sizeof(DIR));
	CopyBytes(Name,(uint8_t *)&FileDir.FileName,11);
	FileDir.FilePosit.Size=Size;
	FileDir.FilePosit.Start=GetNextFAT();
	ClusID=FileDir.FilePosit.Start;
//	for(i=0;i<ClusNum-1;i++){
   	for(i=0;i<ClusNum;i++){
		WriteFAT(ClusID,0xffff);
		ClusNext=GetNextFAT();
		WriteFAT(ClusID,ClusNext);
		ClusID=ClusNext;
	}
	WriteFAT(ClusID, 0xffff);
	WriteDIR(GetEmptyDIR(),&FileDir);
	CopyFAT();
	return SD_SUCC;
}
//-------------------------------------------------------------------------
//���ļ�
uint8_t ReadFile(uint8_t Name[11],uint32_t Start,uint32_t len,uint8_t *p){
	uint16_t BytePerClus,ClusID,m,dirIndex;
	uint32_t LBA;
	uint8_t	 i;
	DIR      FileDir;
	
	if(GetFileID(Name,&FileDir,&dirIndex)==0)return SD_FAIL;//�ļ�������
	
	BytePerClus=BPB_SecPerClus*BPB_BytesPerSec;		//ÿ�ص��ֽ���	
	m=Start/BytePerClus;							//���㿪ʼλ�ð����Ĵ���
	ClusID=FileDir.FilePosit.Start;					//�ļ��Ŀ�ʼ�غ�
	for(i=0;i<m;i++)ClusID=ReadFAT(ClusID);		//���㿪ʼλ�����ڴصĴغ�	
	i=(Start%BytePerClus)/BPB_BytesPerSec;			//���㿪ʼλ�����������Ĵ���ƫ��
	LBA=ClusConvLBA(ClusID)+i;						//���㿪ʼλ�õ��߼�������
	m=(Start%BytePerClus)%BPB_BytesPerSec;			//���㿪ʼλ����������ƫ��

READ:
	for(;i<BPB_SecPerClus;i++){
		ReadFatBlock(LBA++);
		for(;m<BPB_BytesPerSec;m++){
			*p++=BUFFER[m];
			if(--len==0)return SD_SUCC;			//�����ȡ��ɾ��˳�
		}
		m=0;
	}
	i=0;
	ClusID=ReadFAT(ClusID);							//��һ�شغ�
	LBA=ClusConvLBA(ClusID);
	goto READ;
}
//-------------------------------------------------------------------------
//д�ļ�
uint8_t WriteFile(uint8_t Name[11],uint32_t Start,uint32_t len,uint8_t *p){
	uint16_t BytePerClus,ClusID,m,dirIndex;
	uint32_t LBA;
	uint8_t	 i;
	DIR      FileDir;
	
	if(GetFileID(Name,&FileDir,&dirIndex)==0)return SD_FAIL;//�ļ�������
	
	BytePerClus=BPB_SecPerClus*BPB_BytesPerSec;		// ÿ�ص��ֽ���	
	m=Start/BytePerClus;							//���㿪ʼλ�ð����Ĵ���
	ClusID=FileDir.FilePosit.Start;					//�ļ��Ŀ�ʼ�غ�
	for(i=0;i<m;i++)ClusID=ReadFAT(ClusID);		//���㿪ʼλ�����ڴصĴغ�	
	i=(Start%BytePerClus)/BPB_BytesPerSec;			//���㿪ʼλ�����������Ĵ���ƫ��
	LBA=ClusConvLBA(ClusID)+i;						//���㿪ʼλ�õ��߼�������
	m=(Start%BytePerClus)%BPB_BytesPerSec;			//���㿪ʼλ����������ƫ��

WRITE:
	for(;i<BPB_SecPerClus;i++){
		ReadFatBlock(LBA);
		for(;m<BPB_BytesPerSec;m++){
			BUFFER[m]=*p++;
			if(--len==0){							//�����ȡ��ɾ��˳�
				WriteFatBlock(LBA);					//��д����
				return SD_SUCC;				
			}
		}
		m=0;
		WriteFatBlock(LBA++);						//��д����
	}
	i=0;
	ClusID=ReadFAT(ClusID);							//��һ�شغ�
	LBA=ClusConvLBA(ClusID);
	goto WRITE;
}
//-------------------------------------------------------------------------
uint8_t InitFat16(void){							//��ʼ��FAT16�ı���
//	if(ReadMBR()==SD_FAIL)return SD_FAIL;  //for HardDisk
	if(ReadBPB()==SD_FAIL)return SD_FAIL;

	return SD_SUCC;
}
//-------------------------------------------------------------------------
//ɾ���ļ�
uint8_t EreaseFile(uint8_t Name[11]){
	uint16_t ClusID,ClusNext,dirIndex;
	DIR FileDir;
	
	if(GetFileID(Name,&FileDir,&dirIndex)==0)return SD_FAIL;	//�ļ�������
	ClusID=FileDir.FilePosit.Start;					//�ļ��Ŀ�ʼ�غ�

EREASEFAT:
	if((ClusNext=ReadFAT(ClusID))!=0xffff){		//ɾ��FAT���е�����
		WriteFAT(ClusID,0x0000);
		ClusID=ClusNext;
	}else{
		WriteFAT(ClusID,0x0000);
		goto EREASEFATEND;
	}
	goto EREASEFAT;
EREASEFATEND:
	
	FileDir.FileName.NAME[0]=0xe5;					//ɾ��Dir�е��ļ���¼
	WriteDIR(dirIndex,&FileDir);
	CopyFAT();										//FAT2<-FAT1
	return SD_SUCC;
}
//-------------------------------------------------------------------------
