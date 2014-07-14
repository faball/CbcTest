#include "HwController.h"
#include "utils/Exception.h"
#include <fstream>
#include <sstream>
#include "Data.h"

using namespace CbcDaq;

namespace Strasbourg{

	template<typename T> T swapBit( T pVal, unsigned int pNbits ){
		T cNewValue(0);
		unsigned int cSize = sizeof(T);
		if( pNbits > cSize ) return 0;
		for( unsigned int i=0; i < pNbits; i++ ){
			cNewValue |= pVal & 1<<i ? 1<<(pNbits-1-i) : 0;	
		}
		return cNewValue;
	}

	unsigned int getTestPulseGroupBitSwapped( unsigned int pVal ){

		return swapBit<unsigned int>( pVal, 3 );
	}

	typedef std::pair< unsigned int, std::vector<uint32_t> > CbcRegUpdatePair;

	void CbcRegUpdateMap::Reset( unsigned int pNFe ){

		clear();
		for( unsigned int cFe=0; cFe < pNFe; cFe++ ){
			insert( CbcRegUpdatePair(cFe, std::vector<uint32_t>(0) ) ); 
		}
	}
	void CbcRegUpdateMap::ClearList(){

		CbcRegUpdateMap::iterator cIt = begin();
		for( ; cIt != end(); cIt++ ) cIt->second.clear();
	}

	HwController::HwController( const char *pName ):
		fName( pName ),
		fData(0),
		fGlibSetting(), 
		fCbcRegSetting(), 
		fCbcRegUpdateList(),
		fNFe(0), 
		fNCbc(0), 
		fNeventPerAcq(100),
		fNTotalAcq(0){

			fData = new Strasbourg::Data();
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands.CBC_DATA_PACKET_NUMBER", fNeventPerAcq );
			AddGlibSetting( "FE_expected", 1 ); 
			AddGlibSetting( "CBC_expected", 3 );
			AddGlibSetting( "COMMISSIONNING_MODE_CBC_TEST_PULSE_VALID", 1 );
			AddGlibSetting( "COMMISSIONNING_MODE_DELAY_AFTER_FAST_RESET", 50 );
			AddGlibSetting( "COMMISSIONNING_MODE_DELAY_AFTER_L1A", 400 );
			AddGlibSetting( "COMMISSIONNING_MODE_DELAY_AFTER_TEST_PULSE", 200 );
			AddGlibSetting( "COMMISSIONNING_MODE_RQ", 1 );
			AddGlibSetting( "cbc_stubdata_latency_adjust_fe1", 1 );
			AddGlibSetting( "cbc_stubdata_latency_adjust_fe2", 1 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands.ACQ_MODE", true );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands.CBC_DATA_GENE", 1 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands.INT_TRIGGER_FREQ", 4 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands.TRIGGER_SEL", 0 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands2.clock_shift", 0 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands2.negative_logic_CBC", 1 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands2.negative_logic_sTTS", 0 );
			AddGlibSetting( "user_wb_ttc_fmc_regs.pc_commands2.polarity_tlu", 0 );
			//	AddGlibSetting( "user_wb_ttc_fmc_regs.cbc_acquisition.CBC_TRIGGER_ONE_SHOT", 0 );
		}

	HwController::~HwController(){
		delete fData;
	}
	void HwController::ConfigureGlibController( const char *pUhalConfigFileName, const char *pBoardId ){

		ConfigureGlib( pUhalConfigFileName, pBoardId );

		//Setting internal members
		fNFe = fGlibSetting.find( "FE_expected" )->second;
		fNFe = fNFe == 1 ? 1 : 2;	
		unsigned int cExpectedCbc = fGlibSetting.find( "CBC_expected" )->second;	
		fNCbc = cExpectedCbc == 1 ? 1 : 2;	
		fNeventPerAcq = fGlibSetting.find( "user_wb_ttc_fmc_regs.pc_commands.CBC_DATA_PACKET_NUMBER" )->second;

		//Preparing CBC register setting and update list map
		fCbcRegSetting.Reset( fNFe, fNCbc );
		fCbcRegUpdateList.Reset( fNFe );	

		//Initialising Data object
		fData->Initialise( fNeventPerAcq );
		for( unsigned int cFe=0; cFe < fNFe; cFe++ ){
			fData->AddFe( cFe );
		}
	}
	void HwController::ConfigureCbc(){

//		CbcHardReset();
		fCbcRegSetting.ClearRegisters();	
		fCbcRegUpdateList.ClearList();	

		for( unsigned int cFe=0; cFe < fNFe; cFe++ ){

			for( unsigned int cCbc=0; cCbc < fNCbc; cCbc++ ){

				LoadCbcRegInfo( cFe, cCbc );
			}
		}
		UpdateCbcRegValues();
		return;
	}
	const Event *HwController::GetNextEvent()const{ return fData->GetNextEvent(); }
	const char *HwController::GetBuffer( UInt_t &pBufSize )const{ return fData->GetBuffer( pBufSize ); }
	unsigned int HwController::GetGlibSetting( const char *pName )const{

		GlibSetting::const_iterator cIt = fGlibSetting.find( pName );
		if( cIt == fGlibSetting.end() ){
			std::cerr << fName << " The GLIB setting " << pName << " does not exist." << std::endl; 
			return 0;
		}
		return cIt->second;
	}
	const std::string &HwController::GetCbcRegSettingFileName( unsigned int pFeId, unsigned int pCbcId )const{ 

		const FeCbcRegMap *cFeCbcRegMap = fCbcRegSetting.GetFeCbcRegMap( pFeId );
		if( !cFeCbcRegMap ) throw Exception( Form( "Invalid FE : %d", pFeId ) );
		FeCbcRegMap::const_iterator cIt = cFeCbcRegMap->find( pCbcId );
		if( cIt == cFeCbcRegMap->end() ) throw Exception( Form( "Invalid CBC : %d", pCbcId ) ); 
		return cIt->second.GetFileName();
	}
	std::vector<const CbcRegItem *> HwController::UpdateCbcRegValues(){

		std::vector<const CbcRegItem *> cRegList(0);

		CbcRegUpdateMap::iterator cIt = fCbcRegUpdateList.begin();

		for( ; cIt != fCbcRegUpdateList.end(); cIt++ ){
			unsigned int cFe = cIt->first;
			std::vector<uint32_t> &cSentVecReq = cIt->second;
			std::vector<uint32_t> cReadVecReq = cSentVecReq;
			if( cSentVecReq.size() == 0 ) continue;
			WriteAndReadbackCbcRegValues( (uint16_t)cFe, cReadVecReq );
			if( cSentVecReq.size() != cReadVecReq.size() ) {
				std::cout << "CBC registers are not written correctly." << std::endl;
			}
			std::vector<const CbcRegItem *> cThisList = setCbcRegSettings( cFe, cReadVecReq );	
			cRegList.insert( cRegList.end(), cThisList.begin(), cThisList.end() ); 
			cSentVecReq.clear();
		}
		return cRegList;
	}
	std::vector<const CbcRegItem *> HwController::ReConfigureCbc( unsigned int pFe, unsigned int pCbc ){

		fCbcRegUpdateList.ClearList();
		LoadCbcRegInfo( pFe, pCbc );
		return UpdateCbcRegValues();
	}
	void HwController::AddGlibSetting( const char *pName, unsigned int pValue ){

		fGlibSetting.insert( GlibItem( pName, pValue ) );
	}
	void HwController::SetGlibSetting( const char *pName, unsigned int pValue ){

		GlibSetting::iterator cIt = fGlibSetting.find( pName );
		if( cIt == fGlibSetting.end() ){
			throw Exception( Form( "%s: The Glib setting name : %s does not exist.", fName.c_str(), pName ) );
		}
		cIt->second = pValue;
	}
	void HwController::SetCbcRegSettingFileName( unsigned int pFeId, unsigned int pCbcId, const std::string pName ){ 

		if( fCbcRegSetting.GetCbcRegList( pFeId, pCbcId ) )		
			fCbcRegSetting.SetCbcRegSettingFileName( pFeId, pCbcId, pName );
	}
	void HwController::AddCbcRegUpdateItem( unsigned int pFe, unsigned int pCbc, unsigned int pPage, unsigned int pAddr, unsigned int pVal ){

		fCbcRegSetting.SetValue0( pFe, pCbc, pPage, pAddr, pVal );
//		std::cout << pVal << std::endl;
		CbcRegUpdateMap::iterator cIt = fCbcRegUpdateList.find(pFe);
		if( cIt == fCbcRegUpdateList.end() ){
			std::cerr << "Inconsistent FE id " << pFe << std::endl;
		}

		addCbcReg( cIt->second, pCbc, pPage, pAddr, pVal );
	}
	void HwController::AddCbcRegUpdateItemsForEnableTestPulse( unsigned int pFe, unsigned int pCbc, bool pEnable ){

		unsigned int cPage(0x00), cAddr(0x0F);
		unsigned int cValue = fCbcRegSetting.GetValue( pFe, pCbc, cPage, cAddr );
		unsigned int cMask = 1 << 6;
		cValue &= ~cMask;
		cValue |= pEnable << 6;
		AddCbcRegUpdateItem( pFe, pCbc, 0, cAddr, cValue );
	}
	void HwController::AddCbcRegUpdateItemsForNewTestPulseGroup( unsigned int pFe, unsigned int pCbc, unsigned int pTestPulseGroup ){

		unsigned int cGroupBits = swapBit( pTestPulseGroup, 3 );
		unsigned int cPage(0x00), cAddr(0x0E);
		unsigned int cNewTestGroupVal = fCbcRegSetting.GetValue( pFe, pCbc, cPage, cAddr );

		unsigned int cMask = ( 1<<2 | 1<<1 | 1 );
		cNewTestGroupVal &= ~cMask;
		cNewTestGroupVal |= cGroupBits;

		AddCbcRegUpdateItem( pFe, pCbc, 0, cAddr, cNewTestGroupVal );
	}
	void HwController::AddCbcRegUpdateItemsForNewVCth( unsigned int pVCth ){

		uint32_t cAddr(0x0C), cValue(pVCth );
		for( unsigned int cFe=0; cFe < fNFe; cFe++ ){

			for( unsigned int cCbc=0; cCbc < fNCbc; cCbc++ ){

				AddCbcRegUpdateItem( cFe, cCbc, 0, cAddr, cValue );
			}
		}
	}

	void HwController::AddCbcRegUpdateItemsForNewTestPulseDelay( unsigned int pDel ){ //fb

			uint32_t cAddr(0x0E), cValue(pDel );
			for( unsigned int cFe=0; cFe < fNFe; cFe++ ){

				for( unsigned int cCbc=0; cCbc < fNCbc; cCbc++ ){

					AddCbcRegUpdateItem( cFe, cCbc, 0, cAddr, cValue );
				}
			}
		}

	//clear the CbcRegUpdateList and get the list of corresponding CbcRegItem's (So that GUI can retrieve the original values) 
	std::vector<const CbcRegItem *> HwController::ResetCbcRegUpdateList(){

		std::vector<const CbcRegItem *> cRegList(0);

		CbcRegUpdateMap::iterator cIt = fCbcRegUpdateList.begin();

		for( ; cIt != fCbcRegUpdateList.end(); cIt++ ){
			unsigned int cFe = cIt->first;
			std::vector<uint32_t> &cVecReq = cIt->second;
			for( unsigned int i=0; i < cVecReq.size(); i++ ){
				unsigned int cCbc(0), cPage(0), cAddr(0), cVal(0);
				decodeRegInfo( cVecReq[i], cCbc, cPage, cAddr, cVal ); 
				//std::cout << "Cbc register configuration : " << cFe << cCbc << " " << cPage << " " << cAddr << " " << cVal << std::endl;
				cRegList.push_back( fCbcRegSetting.GetCbcRegItem( cFe, cCbc, cPage, cAddr ) );
			}
			cVecReq.clear();
		}	
		return cRegList;
	}
	void HwController::LoadCbcRegInfo( unsigned int pFe, unsigned int pCbc ){

		CbcRegUpdateMap::iterator cIt = fCbcRegUpdateList.find(pFe);
		if( cIt == fCbcRegUpdateList.end() ){
			throw Exception( Form( "Inconsistent FE id = %d", pFe ) );
		}
		std::vector<uint32_t> &cVecReq = cIt->second;

		std::ifstream cFile;
		cFile.open( GetCbcRegSettingFileName( pFe, pCbc ).c_str() );
		if( !cFile.is_open() ){
			throw Exception( Form( "Failed to open the file %s.", GetCbcRegSettingFileName( pFe, pCbc ).c_str() ) );
		}
		std::string cLine;
		std::string cName;
		unsigned int cPage(0), cAddr(0), cDefVal(0), cVal(0);
		while( ! (getline( cFile, cLine ).eof()) ){  
			if( cLine.find_first_not_of( " \t" ) == std::string::npos ) continue; 
			std::stringstream cSs( cLine );
			cSs >> cName;
			if( cName[0] == '#' || cName[0] =='*' ) continue;
			cSs >> std::hex;
			cSs >> cPage >> cAddr >> cDefVal >> cVal;
#ifdef __CBCDAQ_DEV__
			if( cAddr == 0x0B ){
				std::cout << cLine << std::endl;
				std::cout << cName << "|" << std::hex << cPage << "|" << cAddr << "|"  << cDefVal << "|"  << cVal << std::endl;
			}
#endif
			const CbcRegItem *cItem = fCbcRegSetting.GetCbcRegItem( pFe, pCbc, cName );
			if( cItem == 0 ){
				CbcRegItem * cItemNew = new CbcRegItem( pFe, pCbc, cName.c_str(), cPage, cAddr, cDefVal, cVal );	
				fCbcRegSetting.AddItem( pFe, pCbc, cItemNew );
			}
			else{
				fCbcRegSetting.SetValue0( pFe, pCbc, cName, cVal );
			}
			addCbcReg( cVecReq, pCbc, cPage, cAddr, cVal );
		}
		cFile.close();
#ifdef __CBCDAQ_DEV__
		std::cout << "The file : " << GetCbcRegSettingFileName( pFe, pCbc ) << " is parsed." << std::endl;
#endif
		return;
	}
	void HwController::SaveCbcRegInfo( const char *newdir ){

		for( unsigned int cFe=0; cFe < fNFe; cFe++ ){

			for( unsigned int cCbc=0; cCbc < fNCbc; cCbc++ ){
				if( newdir != 0 ){
					std::string cRegFileName( GetCbcRegSettingFileName( cFe, cCbc ) );
					unsigned found = cRegFileName.rfind( "/" );
					cRegFileName = std::string( newdir ) + "/" + cRegFileName.substr( found + 1 );
					SetCbcRegSettingFileName( cFe, cCbc, cRegFileName );
				}
				SaveCbcRegInfo( cFe, cCbc );
			}
		}

	}
	void HwController::SaveCbcRegInfo( unsigned int pFe, unsigned int pCbc ){

		std::string cFname = GetCbcRegSettingFileName( pFe, pCbc );	
		size_t found = cFname.rfind( "/" );
		if( found != std::string::npos ){
			std::string cDir = cFname.substr( 0, found );
			system( Form( "mkdir -p %s", cDir.c_str() ) );
		}

		std::ofstream cFile;
		cFile.open( cFname.c_str() );
		if( !cFile.is_open() ){
			std::cerr << "The file : " << cFname << " could not be opened." << std::endl;
			return;
		}
		TString cLine = Form( "%-45s\t%s\t%s\t%s\t%s", "* RegName", "Page", "Addr", "Defval", "Value" );
		cFile << cLine << std::endl; 
		cFile << "*--------------------------------------------------------------------------------" << std::endl;
		const CbcRegList *cList = fCbcRegSetting.GetCbcRegList( pFe, pCbc );
		unsigned int cPage(0), cAddr(0), cDefVal(0), cVal(0);
		for( unsigned int i=0; i<cList->size(); i++ ){
			const std::string &cName = cList->at(i)->Name();
			cPage = cList->at(i)->Page();
			cAddr = cList->at(i)->Address();
			cDefVal = cList->at(i)->DefVal();
			cVal = cList->at(i)->Value();
			cLine = Form( "%-45s\t0x%02X\t0x%02X\t0x%02X\t0x%02X", cName.c_str(), cPage, cAddr, cDefVal, cVal );
			cFile << cLine << std::endl; 
		}
		cFile.close();
	}
	const CbcRegItem * HwController::setCbcRegSetting( unsigned int pFe, unsigned int pCbc, unsigned int pPage, unsigned int pAddr, unsigned int pValue ){

		return  fCbcRegSetting.SetValue( pFe, pCbc, pPage, pAddr, pValue ); 
	}

	std::vector< const CbcRegItem *> HwController::setCbcRegSettings( unsigned int pFe, std::vector<uint32_t> &pVecReq ){	
                
		std::vector<const CbcRegItem *> cRegList(0);
		for( unsigned int i=0; i < pVecReq.size(); i++ ){
			unsigned int cCbc(0), cPage(0), cAddr(0), cVal(0);
			decodeRegInfo( pVecReq[i], cCbc, cPage, cAddr, cVal ); 
#ifdef __CBCDAQ_DEV__
			if( cRAddr == 0x0B ) std::cout << "Cbc register configuration : " << pFe << " " << cCbc << " " << cPage << " " << cAddr << " " << cVal << std::endl;
#endif
			cRegList.push_back( setCbcRegSetting( pFe, cCbc, cPage, cAddr, cVal ) );
		}
		return cRegList;
	}
}

