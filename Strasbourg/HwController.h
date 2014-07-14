#ifndef __HWCONTROLLER_H__
#define __HWCONTROLLER_H__

#include <string>
#include <map>
#include <vector>
#include <boost/cstdint.hpp>
#include "Cbc/CbcRegInfo.h"

using namespace Cbc;

namespace Strasbourg{

	typedef std::pair< std::string, unsigned int > GlibItem;
	typedef std::map< std::string, unsigned int > GlibSetting; 

	class Data;
	class Event;

	//first: FE id
	class CbcRegUpdateMap : public std::map< unsigned int, std::vector<uint32_t> > {
		public:
			CbcRegUpdateMap(){}
			~CbcRegUpdateMap(){}
			void Reset(unsigned int pNFe );
			void ClearList();
	};

	class HwController {

		public:
			HwController( const char *pName );
			virtual ~HwController();

			virtual void ConfigureGlib( const char *pUhalConfigFileName, const char *pBoardId )=0;
			virtual void StartAcquisition()=0;
			virtual void ReadDataInSRAM( unsigned int pNthAcq, bool pBreakTrigger )=0;
			virtual void EndAcquisition( unsigned int pNthAcq )=0;
			virtual void WriteAndReadbackCbcRegValues( uint16_t pFe, std::vector<uint32_t>& pVecReq )=0;
			virtual void CbcHardReset()=0;
			virtual void CbcFastReset()=0;

			void ConfigureGlibController( const char *pUhalConfigFileName, const char *pBoardId );
			void ConfigureCbc();

			const std::string & Name()const{ return fName; }
			const Event *GetNextEvent()const;
			const char *GetBuffer( UInt_t &pBufSize )const;
			const GlibSetting &GetGlibSetting()const{ return fGlibSetting; } 
			unsigned int GetGlibSetting( const char * pName )const;
			const std::string &GetCbcRegSettingFileName( unsigned int pFe, unsigned int pCbc )const; 
			const CbcRegMap &GetCbcRegSetting()const{ return fCbcRegSetting; }
			unsigned int GetNumberOfTotalAcq()const{ return fNTotalAcq; }

			std::vector<const CbcRegItem *> UpdateCbcRegValues();
			std::vector<const CbcRegItem *> ReConfigureCbc( unsigned int pFe, unsigned int pCbc );
			void AddGlibSetting( const char * pName, unsigned int pValue );
			void SetGlibSetting( const char * pName, unsigned int pValue );
			void SetCbcRegSettingFileName( unsigned int pFe, unsigned int pCbc, const std::string pName ); 
			void AddCbcRegUpdateItem( unsigned int pFe, unsigned int pCbc, unsigned int pPage, unsigned int pAddr, unsigned int pVal );
			void AddCbcRegUpdateItemsForEnableTestPulse( unsigned int pFe, unsigned int pCbc, bool pEnable );
			void AddCbcRegUpdateItemsForNewTestPulseGroup( unsigned int pFe, unsigned int pCbc, unsigned int pTestPulseGroup );
			void AddCbcRegUpdateItemsForNewVCth( unsigned int pVCth );
			void AddCbcRegUpdateItemsForNewTestPulseDelay( unsigned int pDel );
			std::vector<const CbcRegItem *> ResetCbcRegUpdateList();
			void LoadCbcRegInfo( unsigned int pFe, unsigned int pCbc );
			void SaveCbcRegInfo( unsigned int pFe, unsigned int pCbc );
			void SaveCbcRegInfo( const char *newdir = 0 );

		protected:
			Data *getData()const{ return fData; }
			virtual void decodeRegInfo( uint32_t pVecReq, uint32_t &pCbcId, uint32_t &pPage, uint32_t &pAddr, uint32_t &pWrite)=0;
			virtual void addCbcReg(std::vector<uint32_t>& pVecReq, uint32_t pCbcId, uint32_t pPage, uint32_t pAddr, uint32_t pWrite)=0;

			std::vector<const CbcRegItem *> setCbcRegSettings( unsigned int pFe, std::vector<uint32_t> &pReadVecReq );	
			const CbcRegItem * setCbcRegSetting( unsigned int pFe, unsigned int pCbc, unsigned int pPage, unsigned int pAddr, unsigned int pValue ); 

			std::string fName;

			Data *fData;

			GlibSetting fGlibSetting; 
			CbcRegMap fCbcRegSetting;
			CbcRegUpdateMap fCbcRegUpdateList; 

			unsigned int fNFe;
			unsigned int fNCbc;
			unsigned int fNeventPerAcq;
			unsigned int fNTotalAcq;
	};
}

#endif

