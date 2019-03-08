
//____________________________________________________________________________
//  General Information:
//
//  File Name:      ini_files.cpp
//  Author:         yuyonghui

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Revision History:
//  10/24/2018   yonghui   Initial Version
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef WIN32
#pragma warning( disable : 4786 )
#endif

// ==================
// INCLUDE FILES
// ==================
#include "common/ini_files.h"
#include <algorithm>
#include <fstream>
#include <string.h>

#ifndef WIN32

#ifndef _snprintf
#define _snprintf snprintf 
#endif

#endif

namespace bdf {
namespace common {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CIniFileS::CRecord::CRecord()
{
  cCommented=CC_Record;
}
    
CIniFileS::CRecord::CRecord(const CRecord & rec)
{
  strComments = rec.strComments ;
  cCommented  = rec.cCommented ;
  strSecName  = rec.strSecName ;
  strKey      = rec.strKey ;
  strValue    = rec.strValue ;
}
CIniFileS::CRecord::CRecord(const std::string & srcComments,char srcComment,
  const std::string & srcSection,const std::string &srcKey,const std::string & srcValue)
{
  strComments = srcComments ;
  cCommented  = srcComment ;
  strSecName  = srcSection ;
  strKey      = srcKey ;
  strValue    = srcValue ;
}
CIniFileS::CRecord & CIniFileS::CRecord::operator=(const CRecord & rec)
{
  strComments = rec.strComments ;
  cCommented  = rec.cCommented ;
  strSecName  = rec.strSecName ;
  strKey      = rec.strKey ;
  strValue    = rec.strValue ;
  return *this ;
}
CIniFileS::CRecord::~CRecord()
{
        
}
//////////////////////////////////////////////////////////////////////////
    
    
// A function to trim whitespace from both sides of a given string
static void _ini_Trim(std::string& str, const std::string & ChrsToTrim = (" \t\n\r"), int TrimDir = 0)
{
  size_t startIndex = str.find_first_not_of(ChrsToTrim);
  if (startIndex == std::string::npos){str.erase(); return;}
  if (TrimDir < 2) str = str.substr(startIndex, str.size()-startIndex);
  if (TrimDir!=1) str = str.substr(0, str.find_last_not_of(ChrsToTrim) + 1);
}
    
static std::string _ini_TrimStr(std::string str, const std::string & ChrsToTrim = (" \t\n\r"), int TrimDir = 0)
{
  size_t startIndex = str.find_first_not_of(ChrsToTrim);
  if (startIndex == std::string::npos){str.erase(); return str;}
  if (TrimDir < 2) str = str.substr(startIndex, str.size()-startIndex);
  if (TrimDir!=1) str = str.substr(0, str.find_last_not_of(ChrsToTrim) + 1);
  return str;
}

//////////////////////////////////////////////////////////////////////////
    
CIniFileS::CIniFileS(const char* szFileName)
{
  Open(szFileName) ;
}
    
CIniFileS::~CIniFileS()
{
  clear() ;
}

void CIniFileS::clear()
{
  m_strFileName.erase();

  CSection* pSection = NULL;
  CSecPtrMapCIter it = m_mapSection.begin();
  for (; it != m_mapSection.end(); it++)
  {
    if ((pSection = it->second) == NULL)
    {
      continue;
    }
    CRecordPtrMapCIt itr = pSection->mapKV.begin();
    for (; itr != pSection->mapKV.end(); itr++)
    {
      if (itr->second)
      {
        delete itr->second;
      }
    }
    delete pSection;
  }
  m_mapSection.clear();
}
    
// ____________________________________________________________________________
//
// Class:       CIniFileS
//
// Method:      Open
//
// Purpose:     open and load the configuration file
//
// Arguments:   strFileName  file name of the configuration file
//
// Returns:     true, success
//
// ____________________________________________________________________________
    
bool CIniFileS::Open(const char* szFileName)
{
  if (szFileName == NULL)
  {
    return false;
  }
        
  std::string strFileName(szFileName) ;    
  if (strFileName.empty())
    return false ;
        
  std::string strLine ;            // Holds the current line from the ini file
  std::string strCurSec ;  // Holds the current section name
        
  // Create an input filestream
  std::ifstream inFile(strFileName.c_str(),std::ifstream::in|std::ifstream::binary);    
  if(!inFile.is_open())
    return false;
  //if (!inFile.is_open()) return false; // If the input file doesn't open, then return
        
  clear() ;
  m_strFileName   = strFileName ;
        
  std::string strComments = ("");      // A string to store comments in
  CRecord* pRec = new CRecord() ;   // Define a new record
  CSection* pSection= NULL ;  // define a new section ptr
  size_t npos = -1;
#if (defined(WIN32) &&defined(_UNICODE))
        
  char * pBuffer = NULL ;
  int nWideLen,nSrcLen ;
        
  while(inFile.good())        // Read until the end of the file
  {
    std::getline(inFile, strLine);
            
    nSrcLen = strLine.size() ;
    pBuffer = new char[nSrcLen+1] ;
    pBuffer[nSrcLen] = '\0' ;
            
    memcpy(pBuffer,strLine.c_str(),nSrcLen) ;
    nWideLen    = MultiByteToWideChar(CP_ACP,0,pBuffer,nSrcLen,NULL,0) ; 
            
    char * pRel    = new char[nWideLen+1] ; pRel[nWideLen]  = ('\0') ; 
    MultiByteToWideChar(CP_ACP,0,pBuffer,nSrcLen,pRel,nWideLen); 
    strLine.assign( pRel, nWideLen ) ; 
    delete [] pRel ; 
    pRel = NULL ; 
    delete [] pBuffer ;
    pBuffer = NULL ;
#else
  while(inFile.good())        // Read until the end of the file
  {
    std::getline(inFile, strLine);
#endif
    npos = -1;
    _ini_Trim(strLine);                           // Trim whitespace from the ends
    if(!strLine.empty())                          // Make sure its not a blank line
    {
      if((strLine[0]==('#'))||(strLine[0]==(';')))        // Is this a commented line?
      {//注释行
        strComments += strLine + ('\n');
        continue ; //继续读下一行
      } 
      else// else mark it as not being a comment
      {
        pRec->cCommented = CC_Record;
      }
                
      npos = strLine.find((']'));
      if(strLine.size() > 2 && 
          strLine[0]==('[') && npos != strLine.npos)//strLine.find(('[')) != std::string::npos)             
      {// Is this line a section?
        strLine.erase(strLine.begin());             // Erase the leading bracket
        strLine.erase(npos-1);           // Erase the trailing bracket
        pSection = new CSection ;
        pSection->strComments = _ini_TrimStr(strComments);     // Add the comments string (if any)
        pSection->strSecName = _ini_TrimStr(strLine);          // Set the Section value
        //pSection->strSecName = _ini_TrimStr(strLine);
        pSection->cCommented = pRec->cCommented ;
        strCurSec = pSection->strSecName ;

        strComments.erase() ;                           // Clear the comments for re-use
        if (!m_mapSection.insert(CSecPtrMapVt(pSection->strSecName,pSection)).second) {
          delete pSection;
          pSection = NULL;
        }
        continue;
      }

      //non-section
      if(strLine.find(('=')) != std::string::npos)        
      {// Is this line a Key/Value?
        pRec->strComments = _ini_TrimStr(strComments);     // Add the comments string (if any)
        strComments.erase() ;                           // Clear the comments for re-use
        pRec->strSecName = strCurSec;                 // Set the section to the current Section
        pRec->strKey = _ini_TrimStr(strLine.substr(0,strLine.find(('='))));    // Set the Key value to everything before the = sign
        pRec->strValue = _ini_TrimStr(strLine.substr(strLine.find(('='))+1));  // Set the Value to everything after the = sign
      }

      if(strComments.empty())           // Don't add a record yet if its a comment line
      {
        if(pSection==NULL)
        {
          pSection = new CSection ;
          pSection->strSecName = pRec->strSecName ;
          pSection->cCommented = pRec->cCommented ;
          pSection->strComments = pRec->strComments ;
          if (!m_mapSection.insert(CSecPtrMapVt(pSection->strSecName,pSection)).second) {
              delete pSection ;
              pSection = NULL ;
          }
        }

        if (pSection!=NULL 
          && pSection->mapKV.insert(CRecordPtrMapVt(pRec->strKey,pRec)).second) {
          //Add the record to content list
          //reuse the pRec object when insert into mapKV failure.
          pRec = new CRecord() ;
        }
      }
    }
  }
        
  inFile.close();                       // Close the file

  if (pRec)
  {
    delete pRec;
    pRec = NULL;
  }
        
  return true ;
}

bool CIniFileS::LoadFromBuffer(const std::string &strBuffer) 
{//从内存中导入ini数据。
  if (strBuffer.empty())
  {
    return false;
  }

  std::string strLine ;            // Holds the current line from the ini file
  std::string strCurSec ;          // Holds the current section name
    
  m_strFileName.erase() ;
  clear();         // Clear the content vector
    
  std::string strComments = ("");     // A string to store comments in
  CRecord* pRec = new CRecord() ;  // Define a new record
  CSection* pSection= NULL ;  // define a new section ptr
    
  size_t nPrePos = -1 ;
  size_t nPos = 0 ;

  nPos = strBuffer.find(('\n'),nPrePos+1) ;
  while(nPos!=std::string::npos)
  {
    if ((nPos-nPrePos)>1)
    {
      if(strBuffer[nPos-1]==('\r'))
          strLine = strBuffer.substr(nPrePos+1,nPos-nPrePos-2) ;
      else                        
          strLine = strBuffer.substr(nPrePos+1,nPos-nPrePos-1) ;
      _ini_Trim(strLine);                                // Trim whitespace from the ends
      if(!strLine.empty())                          // Make sure its not a blank line
      {
        if((strLine[0]==('#'))||(strLine[0]==(';')))        // Is this a commented line?
        {//注释行
          strComments += strLine + ('\n');
                        
          nPrePos = nPos ;
          nPos = strBuffer.find(('\n'),nPrePos+1) ;
          continue ; //继续读下一行
        } 
        else// else mark it as not being a comment
        {
          pRec->cCommented = CC_Record;
        }
                    
        if(strLine[0]==('['))//strLine.find(('[')) != std::string::npos)             
        {// Is this line a section?
          strLine.erase(strLine.begin());             // Erase the leading bracket
          strLine.erase(strLine.find((']')));           // Erase the trailing bracket
          pSection = new CSection() ;
          pSection->strComments = _ini_TrimStr(strComments);     // Add the comments string (if any)
          pSection->strSecName = _ini_TrimStr(strLine);          // Set the Section value
          strCurSec = pSection->strSecName ;
          pSection->cCommented = pRec->cCommented ;

          if (!m_mapSection.insert(CSecPtrMapVt(pSection->strSecName,pSection)).second) {
              delete pSection ;
              pSection = NULL ;
          }

          strComments.erase() ;                           // Clear the comments for re-use

          //zhewei_yang modify, add two lines, prevent endless loop, 2013.1.31
          nPrePos = nPos ;
          nPos = strBuffer.find(('\n'),nPrePos+1) ;

          continue;
        }                    
                    
        //non-section
        if(strLine.find(('=')) != std::string::npos)        
        {// Is this line a Key/Value?
          pRec->strComments = _ini_TrimStr(strComments);     // Add the comments std::string (if any)
          strComments.erase() ;                           // Clear the comments for re-use
          pRec->strSecName = strCurSec;                 // Set the section to the current Section
          pRec->strKey = _ini_TrimStr(strLine.substr(0,strLine.find(('='))));    // Set the Key value to everything before the = sign
          pRec->strValue = _ini_TrimStr(strLine.substr(strLine.find(('='))+1));  // Set the Value to everything after the = sign
        }
        //else if invalid format (ignored)

        if(strComments.empty())           // Don't add a record yet if its a comment line
        {
          if(pSection==NULL)
          {
            pSection = new CSection() ;
            pSection->strSecName = pRec->strSecName ;
            pSection->cCommented = pRec->cCommented ;
            pSection->strComments = pRec->strComments ;
            if (!m_mapSection.insert(CSecPtrMapVt(pSection->strSecName,pSection)).second) {
                delete pSection ;
                pSection = NULL ;
            }
          }
          if (pSection!=NULL 
            && pSection->mapKV.insert(CRecordPtrMapVt(pRec->strKey,pRec)).second) {
            // Add the record to content list                       
            //reuse the pRec object when insert into mapKV failure.
            pRec = new CRecord() ;
          }
        }
      }
    }
    nPrePos = nPos ;
    nPos    = strBuffer.find(('\n'),nPrePos+1) ;
  }        
        
  if (pRec)
  {
    delete pRec;
    pRec = NULL;
  }
        
  return true ;
}
    
// ____________________________________________________________________________
//
// Class:       CIniFileS
//
// Method:      SaveToFile
//
// Purpose:     save configuration infomation to file
//
// Arguments:   szFileName  file name of configuration file saved to
//
// Returns:     ture, success
//
// ____________________________________________________________________________
    
bool CIniFileS::SaveToFile(const char* szFileName)
{
  //    ofstream outFile (strFileName.c_str());                 // Create an output filestream
	std::ofstream outFile ;
  if (szFileName != NULL)
  {
    outFile.open(szFileName) ;
  }
  else
  {
    outFile.open(m_strFileName.c_str()) ;
  }
        
  if (!outFile.is_open()) return false;                   // If the output file doesn't open, then return
        
  CRecord* pRec = NULL ;
  CRecordPtrMapCIt itr ;
        
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.begin() ;
  for (; it!=m_mapSection.end(); it++)
  {
    if ((pSection=it->second)==NULL) continue ;
    if (!pSection->strComments.empty())
    {
      outFile << pSection->strComments << std::endl ;             // Write out the comments
    }
    outFile << ("[") << pSection->strSecName << ("]") << std::endl;   // Then format the section

    itr = pSection->mapKV.begin() ;
    for (; itr!=pSection->mapKV.end(); itr++)
    {
      if ((pRec=itr->second)==NULL) continue ;
      if (!pRec->strComments.empty())
      {
          outFile << pRec->strComments << std::endl ;             // Write out the comments
      }
      //outFile << pRec->cCommented << pRec->strKey  
      outFile << pRec->strKey << ("=") << pRec->strValue << std::endl;     // Else format a key/value
    }//end of for (; itr!=pSection->mapKV.end(); itr++)
  }//end of for (; it!=m_mapSection.end(); it++)
        
  outFile.close();    // Close the file
  return true;
}
    
std::string CIniFileS::SaveToStr()
{
  if (m_mapSection.size()>0) //是否已经导入是空文件
  {
    std::string strContent ;  // Hold our return string
    CRecord* pRec = NULL ;
    CRecordPtrMapCIt itr ;
       
//                 CRecordPtrListCIter it = m_recordList.begin() ;
//                 for (; it!=m_recordList.end(); it++)                        // Loop through the content
    CSection* pSection = NULL ;
    CSecPtrMapCIter it = m_mapSection.begin() ;
    for (; it!=m_mapSection.end(); it++)
    {
      if ((pSection=it->second)==NULL) continue ;
                
      if (!pSection->strComments.empty())
      {
        strContent += pSection->strComments + ("\n") ;     // Add the comments
      }
      strContent += ("[") + pSection->strSecName + ("]") ;   // Add the section

      itr = pSection->mapKV.begin() ;
      for (; itr!=pSection->mapKV.end(); itr++)
      {
        if ((pRec=itr->second)==NULL) continue ;
        if(!pRec->strComments.empty())
        {
          strContent += pRec->strComments + ("\n") ;              // Add the comments
        }
        if(pRec->cCommented != CC_Record)
        {
          strContent += pRec->cCommented;               // If this is commented, then add it
        }
        strContent += pRec->strKey + ("=") + pRec->strValue;// Or the Key value to the return srting
      }//end of for (; itr!=pSection->mapKV.end(); itr++)
    }//end of for (; it!=m_mapSection.end(); it++)

    strContent += ('\n'); // If this is not the last line, add a CrLf
    return strContent;    // Return the contents
  }
        
  return ("");
}

size_t CIniFileS::GetSectionNames(std::vector<std::string>& aSec) const
{
  aSec.clear() ;            
  if (m_mapSection.size()>0)                      // Make sure the file is loaded
  {
    CSecPtrMapCIter it = m_mapSection.begin() ;
    for (; it!=m_mapSection.end(); it++)// Loop through the content
    {
      if (it->second /*&& !it->second->strSecName.empty()*/)// If there is no key value, then its a section
      {
        aSec.emplace_back(it->second->strSecName) ;// Add the section to the return data
      }
    }
  }

  return aSec.size() ;
}        
    
std::vector<std::string> CIniFileS::GetSectionNames() const  
{
  std::vector<std::string> data;                            // Holds the return data
  GetSectionNames(data) ;            
  return data;    // Return the data
}
            
const CIniFileS::CSection* CIniFileS::GetSection(const char* strSecName) const  
{
  CSecPtrMapCIter it = m_mapSection.find(strSecName);
  if (it != m_mapSection.end())
  {
    return it->second;
  }

  return NULL;
}
    
bool CIniFileS::IsExistRecord(const char* strSecName, const char* strKeyName) const  
{            
  CSection* pSection = NULL;
  CSecPtrMapCIter it = m_mapSection.find(strSecName);
  if (it != m_mapSection.end() && (pSection = it->second))
  {
    return (pSection->mapKV.find(strKeyName) != pSection->mapKV.end());
  }

  return false;    // The Section/Key was found or the file isn't loaded
}

const CIniFileS::CRecord* CIniFileS::GetRecord(const char* strSecName, const char* strKeyName) const 
{            
  CSection* pSection = NULL;
  CSecPtrMapCIter it = m_mapSection.find(strSecName);
  if (it != m_mapSection.end() && (pSection = it->second))
  {
    CRecordPtrMapCIt itr = pSection->mapKV.find(strKeyName);
    if (itr != pSection->mapKV.end() && itr->second)
    {
      return (itr->second);
    }
  }
  return NULL;
}
            
bool CIniFileS::GetProfileBinary(const char* strSecName, const char* strKeyName, unsigned char** ppData, size_t* pBytes) const 
{// Retrieve an arbitrary binary value from INI file.

  //ASSERT(ppData != NULL);
  //ASSERT(pBytes != NULL);
  if (!m_strFileName.empty() && (ppData != NULL) && (pBytes != NULL)) // Make sure the file is loaded
  {
    *ppData = NULL;
    *pBytes = 0;

    std::string str = GetValue(strSecName, strKeyName);
    if (str.empty())
        return false;

//        ASSERT(str.size()%2 == 0);
    size_t nLen = str.size();
    *pBytes = (size_t)(nLen)/2;
    *ppData = new unsigned char[*pBytes];
    size_t  i ;
    for (i=0;i<nLen;i+=2)
    {
      (*ppData)[i/2] = (unsigned char) (((str[i+1] - 'A') << 4) + (str[i] - 'A')) ;
    }
    return true ;
  }

  return false ;
}


bool CIniFileS::WriteProfileBinary(const char* strSecName, const char* strKeyName, unsigned char* pData, size_t nBytes,bool bSaveToFile)
{// Sets an arbitrary binary value to INI file.

  if (pData != NULL && nBytes>0)
  {// convert to string and write out
        
    char* lpsz = new char[nBytes*2+1];
    size_t i;
    for (i = 0; i < nBytes; i++)
    {
      lpsz[i*2] = (char)((pData[i] & 0x0F) + 'A'); //low nibble
      lpsz[i*2+1] = (char)(((pData[i] >> 4) & 0x0F) + 'A'); //high nibble
    }
    lpsz[i*2] = 0;

    bool bResult = SetValue(strSecName, strKeyName, lpsz, bSaveToFile) ;
    delete[] lpsz;

    return bResult ;
  }

  return false ;
}

bool CIniFileS::SetValue(const char* strKeyName, const char* strValue, const char* strSecName,bool bSaveToFile)
{
  if (!m_strFileName.empty()) // Make sure the file is loaded
  {
    bool rel = true ;

    CRecord* pRecord = NULL ;            
    CSection* pSection = NULL ;
    CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
    CRecordPtrMapCIt itr ;
    if (it==m_mapSection.end()||(pSection=it->second)==NULL)
    {
      // Define a new section
//                 pRecord = new CRecord ;
//                 pRecord->strSecName = strSecName ;
//                 m_recordList.push_back(pRecord) ;// Add the section
      pSection = new CSection() ;
      if (pSection == NULL) 
      {
        return false;
      }
      pSection->strSecName = strSecName ;
      if (!m_mapSection.insert(CSecPtrMapVt(strSecName,pSection)).second) 
      {
        delete pSection;
        pSection = NULL;
	      rel = false ;
      }
		  else 
      {
	      // Define a new record
	      pRecord = new CRecord() ;
	      pRecord->strSecName= strSecName ;
	      pRecord->strKey    = strKeyName ;
	      pRecord->strValue  = strValue ;
	      pRecord->cCommented= CC_Record ;                
//                 m_recordList.push_back(pRecord) ;// Add the record

	      if (!pSection->mapKV.insert(CRecordPtrMapVt(strKeyName,pRecord)).second) 
        {
          delete pRecord;
          pRecord = NULL;
          rel = false;
	      }
		  }
    }
    else if ((itr=pSection->mapKV.find(strKeyName))==pSection->mapKV.end()
        || (pRecord=itr->second)==NULL)
    {                
      // Define a new record
      pRecord = new CRecord() ;
      pRecord->strSecName= strSecName ;
      pRecord->strKey    = strKeyName ;
      pRecord->strValue  = strValue ;
      pRecord->cCommented= CC_Record ;                
      //m_recordList.push_back(pRecord) ;// Add the record
      if (!pSection->mapKV.insert(CRecordPtrMapVt(strKeyName,pRecord)).second) {
        delete pRecord;
        pRecord = NULL;
        rel = false;
      }
    }
    else
    {
      pRecord->strValue = strValue; // Insert the correct value
    }
    if (rel && bSaveToFile)
    {// Save
      return SaveToFile() ;
    }
    return rel ;
  }

  return false ; // In the event the file does not load
}

bool CIniFileS::SetValue(const char* strKeyName, int nValue, const char* strSecName,bool bSaveToFile)
{
  char szValue[64] = "" ;
  _snprintf(szValue,63,"%d",nValue) ;
  return SetValue(strKeyName,(const char*)szValue,strSecName,bSaveToFile) ;
}

bool CIniFileS::SetValue(const char* strKeyName, unsigned int uValue, const char* strSecName,bool bSaveToFile)
{
  char szValue[64] = "" ;
  _snprintf(szValue,63,"%u",uValue) ;

  return SetValue(strKeyName,(const char*)szValue,strSecName,bSaveToFile) ;
}

bool CIniFileS::SetValue(const char* strKeyName, double dValue, const char* strSecName,bool bSaveToFile)
{
  char szValue[256] = "" ;
  _snprintf(szValue,255,"%f",dValue) ;

  return SetValue(strKeyName,(const char*)szValue,strSecName,bSaveToFile) ;
}

bool CIniFileS::SetValue(const char* strKeyName, long lValue, const char* strSecName,bool bSaveToFile)
{
  char szValue[64] = "" ;
  _snprintf(szValue,63,"%ld",lValue) ;
  return SetValue(strKeyName,(const char*)szValue,strSecName,bSaveToFile) ;
}
    
bool CIniFileS::RenameSection(const char* strOldSecName, const char* strNewSecName,bool bSaveToFile)
{        
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strOldSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    pSection->strSecName = strNewSecName;
    CRecordPtrMapCIt itr = pSection->mapKV.begin();
    for (; itr != pSection->mapKV.end(); itr++)
    {
      if (itr->second)
      {
        itr->second->strSecName = strNewSecName;
      }
    }
    if (bSaveToFile)
    {
      return SaveToFile();
    }
    return true; // Save
  }//end of if (it!=m_mapSection.end()&&(pSection=it->second))

  return false;   // In the event the file does not load
}

bool CIniFileS::CommentRecord(CommentChar cc, const char* strSecName, const char* strKeyName,bool bSaveToFile)
{        
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    CRecordPtrMapCIt itr = pSection->mapKV.find(strKeyName) ;
    if (itr!=pSection->mapKV.end() && itr->second)
    {
      itr->second->cCommented = cc;      // Change the Comment value
      if (bSaveToFile)
      {// Save
        return SaveToFile() ;
      }
      return true ;  
    }
  }
  return false;   // In the event the file does not load
}

bool CIniFileS::CommentSection(char cCommentChar, const char* strSecName,bool bSaveToFile)
{
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    pSection->cCommented = cCommentChar ;

    CRecordPtrMapCIt itr = pSection->mapKV.begin() ;
    for (; itr!=pSection->mapKV.end(); itr++)
    {
      if (itr->second==NULL) continue ;
      itr->second->cCommented = cCommentChar ;     // Remove the Comment value
    }
    if (bSaveToFile)
    {// Save
      return SaveToFile() ;
    }
    //yzw modify, miss return, 2013.2.4
    return true;
  }
  return false; // In the event the file does not load
}

bool CIniFileS::DeleteRecord(const char* strSecName, const char* strKeyName,bool bSaveToFile)
{        
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    CRecordPtrMapIt itr = pSection->mapKV.find(strKeyName) ;
    if (itr!=pSection->mapKV.end() && itr->second)
    {
        delete itr->second ;
        itr->second = NULL ;
        pSection->mapKV.erase(itr) ;

        if (bSaveToFile)
        {// Save
            return SaveToFile() ;
        }
        return true ;  
    }
  }
    
    return false; // In the event the file does not load
}

bool CIniFileS::DeleteSection(const char* strSecName,bool bSaveToFile)
{  
  CSection* pSection = NULL ;
  CSecPtrMapIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    CRecordPtrMapIt itr = pSection->mapKV.begin() ;
    for (; itr!=pSection->mapKV.end(); itr++)
    {
      if (itr->second)
      {
        delete itr->second ;
      }
    }
    pSection->mapKV.clear() ;
    delete pSection ;
    m_mapSection.erase(it) ;

    if (bSaveToFile)
    {// Save
      return SaveToFile() ;
    }
    return true ;
  }
    
  return false;   // In the event the file does not load
}

bool CIniFileS::SetSectionComments(const char* szComments, const char* strSecName,bool bSaveToFile)
{        
  size_t uLen = (szComments)?strlen(szComments):0 ;
  if (uLen==0) return false ;

  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    pSection->strComments = szComments ;          // Set the comments

    if (uLen>=2 && szComments[uLen-1]!='\n')// Is there a comment? 
    {// Does the string end in a newline?
      pSection->strComments += '\n' ;// If not, add one
    }
            
    if (bSaveToFile)
    {// Save
      return SaveToFile() ;
    }
    return true ;
  }
  return false;   // In the event the file does not load
}


bool CIniFileS::SetRecordComments(const char* lpszComments, const char* strSecName, const char* strKeyName,bool bSaveToFile)
{
  size_t uLen = (lpszComments)?strlen(lpszComments):0 ;
  if (uLen==0) return false ;
        
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it!=m_mapSection.end()&&(pSection=it->second))
  {
    CRecordPtrMapCIt itr = pSection->mapKV.find(strKeyName) ;
    if (itr!=pSection->mapKV.end() && itr->second)
    {
      //yzw modify, set record comment, 2013.2.4
      itr->second->strComments = lpszComments;
      //pSection->strComments = lpszComments ;          // Set the comments
            
      if (uLen>=2 && ('\n')!=lpszComments[uLen-1])// Is there a comment? 
      {// Does the string end in a newline?
        //yzw modify, 2013.2.4
        itr->second->strComments += '\n';
        //pSection->strComments += '\n' ;// If not, add one
      }
                
      if (bSaveToFile)
      {// Save
        return SaveToFile() ;
      }
      return true ;  
    }            
  }

  return false;   // In the event the file does not load
}

bool CIniFileS::AddSection(const char* strSecName,bool bSaveToFile)
{        
  bool rel = false ;
  CSection* pSection = NULL ;
  CSecPtrMapCIter it = m_mapSection.find(strSecName) ;
  if (it==m_mapSection.end()||(pSection=it->second)==NULL)
  {
    //yzw modify, no memory for pSection, use new to create, 2013.2.4
    pSection = new CSection();
    if(NULL != pSection)
    {
      rel = true ;
      pSection->strSecName = strSecName ;
      //yze modify, insert into the map, 2013.2.4
      if (!m_mapSection.insert(CSecPtrMapVt(pSection->strSecName,pSection)).second) {
        delete pSection;
        pSection = NULL;
        rel = false;
      }
    }
            
    if (rel && bSaveToFile)
    {// Save
      return SaveToFile() ;
    }
  }

  return rel ;   // The section is exist 
}

}

}

