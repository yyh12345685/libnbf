
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <list>

#include <stdlib.h>

namespace bdf {

//////////////////////////////////////////////////////////////////////////
class CIniFileS{
public:
  class CRecord{
  public:
    std::string  strSecName;
    std::string  strKey;
    std::string  strValue;
    std::string  strComments;
    char    cCommented;
  public:
    CRecord() ;
    CRecord(const CRecord & pkt);
    CRecord(const std::string & srcComments,char srcComment,const std::string & srcSection,
            const std::string &srcKey,const std::string & srcValue);
    CRecord &operator = (const CRecord & pkt) ;
    ~CRecord() ;
  };
  typedef std::map<std::string,CRecord*> CRecordPtrMap ;
  typedef std::map<std::string,CRecord*>::value_type CRecordPtrMapVt ;
  typedef std::map<std::string,CRecord*>::iterator CRecordPtrMapIt ;
  typedef std::map<std::string,CRecord*>::const_iterator CRecordPtrMapCIt ;
  typedef std::list<CRecord*> CRecordPtrList ;
  typedef std::list<CRecord*>::iterator CRecordPtrListIter ;
  typedef std::list<CRecord*>::const_iterator CRecordPtrListCIter ;
        
  class CSection{
  public:
    CSection() {cCommented=' ';} ;
    ~CSection(){};
    std::string  strSecName ;
    std::string  strComments;
    char    cCommented;
    CRecordPtrMap mapKV ;
  };
  typedef std::map<std::string,CSection*> CSecPtrMap ;
  typedef std::map<std::string,CSection*>::const_iterator CSecPtrMapCIter ;
  typedef std::map<std::string,CSection*>::iterator CSecPtrMapIter ;
  typedef std::map<std::string,CSection*>::value_type CSecPtrMapVt ;

  enum CommentChar{
    CC_Pound     = '#',
    CC_SemiColon = ';',
    CC_Record    = ' ',
  };

public:
//文件操作
  bool    Open(const char* szFileName) ;          //打开ini文件
  bool    LoadFromBuffer(const std::string &strBuffer) ;//从内存中导入ini数据。
  bool    SaveToFile(const char* szFileName=NULL);//存配置数据到文件
  std::string  SaveToStr() ;                       //存配置数据到内存
  inline std::string GetFileName() const {return m_strFileName; } ;

//记录操作
  //size_t  GetRecordCount() const{return m_recordList.size() ;};
  inline bool IsExistSection(const char* lpszSecName) {return (m_mapSection.find(lpszSecName)!=m_mapSection.end());};     //判断指定小节(段)是否存在
  inline bool IsExistSection(const char* lpszSecName) const {return (m_mapSection.find(lpszSecName)!=m_mapSection.end());};     //判断指定小节(段)是否存在
  std::vector<std::string>  GetSectionNames() const ;             //获取所有小节(段)名
  size_t  GetSectionNames(std::vector<std::string>& aSec) const ; //获取所有小节(段)名
  const CSecPtrMap& GetSections() const {return m_mapSection;} ;  //获取所有小节(段)名记录
  const CIniFileS::CSection* GetSection(const char* lpszSecName) const ;//获取指定小节(段)所有记录

  bool    IsExistRecord(const char* lpszSecName, const char* lpszKeyName) const ;    //判断指定记录是否存在
  const CRecord* GetRecord(const char* lpszSecName, const char* lpszKeyName) const ;//获取记录
        
  bool    SetValue(const char* lpszKeyName, const char* lpszValue, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetValue(const char* lpszKeyName, unsigned int uValue, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetValue(const char* lpszKeyName, int nValue, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetValue(const char* lpszKeyName, double dValue, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetValue(const char* lpszKeyName, long lValue, const char* lpszSecName,bool bSaveToFile=false) ;

  const std::string& GetComments(const char* lpszSecName,const std::string& lpszDefault=("")) const ;
  const std::string& GetComments(const char* lpszSecName, const char* lpszKeyName,const std::string& lpszDefault=("")) const ;
  const std::string& GetValue(const char* lpszSecName, const char* lpszKeyName,const std::string& lpszDefault=("")) const ;
  int     GetValueInt(const char* lpszSecName, const char* lpszKeyName,int nDefault=0) const ;
  double  GetValueDouble(const char* lpszSecName, const char* lpszKeyName,double dDefault=0) const ;
  long    GetValueLong(const char* lpszSecName, const char* lpszKeyName,long lDefault=0) const ;

  // Sets an arbitrary binary value to INI file.
  bool    WriteProfileBinary(const char* lpszSecName, const char* lpszKeyName, unsigned char* pData,
                              size_t nBytes,bool bSaveToFile=false);
  // Retrieve an arbitrary binary value from INI file.
  bool    GetProfileBinary(const char* lpszSecName, const char* lpszKeyName, unsigned char** ppData, size_t* pBytes) const;

  bool    RenameSection(const char* lpszOldSecName, const char* lpszNewSecName,bool bSaveToFile=false) ;
  bool    CommentRecord(CommentChar cc, const char* lpszSecName, const char* lpszKeyName,bool bSaveToFile=false) ;
  bool    UnCommentRecord(const char* lpszSecName, const char* lpszKeyName,bool bSaveToFile=false) ;
  bool    CommentSection(char cCommentChar, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    UnCommentSection(const char* lpszSecName,bool bSaveToFile=false) ;
  bool    DeleteRecord(const char* lpszSecName, const char* lpszKeyName,bool bSaveToFile=false) ;
  bool    DeleteSection(const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetSectionComments(const char* lpszComments, const char* lpszSecName,bool bSaveToFile=false) ;
  bool    SetRecordComments(const char* lpszComments, const char* lpszSecName, const char* lpszKeyName,bool bSaveToFile=false) ;
//         bool    Sort(std::vector<CRecord> &resultList,bool bIsDescending=false) ;
//         bool    Sort(bool bIsDescending=false,bool bSaveToFile=false) ;
  bool    AddSection(const char* lpszSecName,bool bSaveToFile=false) ;
    
public:
	CIniFileS(const char * szFileName=NULL);
	virtual ~CIniFileS();
  void clear() ;
    
protected:
        
  CSecPtrMap      m_mapSection ;
  std::string          m_strFileName ;
};

//////////////////////////////////////////////////////////////////////////
//
    
inline const std::string& CIniFileS::GetComments(const char* strSecName,const std::string& strDefault) const{
  const CSection* pSec = GetSection(strSecName) ;
  return (pSec)?pSec->strComments:strDefault ;
}

inline const std::string& CIniFileS::GetComments(const char* strSecName, const char* strKeyName,const std::string& strDefault) const{
  const CRecord* pRec = GetRecord(strSecName,strKeyName) ;
  return (pRec)?pRec->strComments:strDefault ;
}

inline const std::string& CIniFileS::GetValue(const char* strSecName, const char* strKeyName,const std::string& strDefault) const{
  const CRecord* pRec = GetRecord(strSecName,strKeyName) ;
  return (pRec)?pRec->strValue:strDefault ;
}
    
inline int CIniFileS::GetValueInt(const char* strSecName, const char* strKeyName,int nDefault) const  {
  const CRecord* pRec = GetRecord(strSecName,strKeyName) ;
  return (pRec)?atoi(pRec->strValue.c_str()):nDefault ;
}
    
inline double CIniFileS::GetValueDouble(const char* strSecName, const char* strKeyName,double dDefault) const  {
  const CRecord* pRec = GetRecord(strSecName,strKeyName) ;
  return (pRec)?atof(pRec->strValue.c_str()):dDefault ;        
}
    
inline long CIniFileS::GetValueLong(const char* strSecName, const char* strKeyName,long lDefault) const  {
  const CRecord* pRec = GetRecord(strSecName,strKeyName) ;
  return (pRec)?atol(pRec->strValue.c_str()):lDefault ;   
}

inline bool CIniFileS::UnCommentSection(const char* strSecName,bool bSaveToFile){
  return CommentSection(CC_Record,strSecName,bSaveToFile) ;
}

inline bool CIniFileS::UnCommentRecord(const char* strSecName, const char* strKeyName,bool bSaveToFile){
  return CommentRecord(CC_Record,strSecName,strKeyName,bSaveToFile);           // In the event the file does not load
}

}

