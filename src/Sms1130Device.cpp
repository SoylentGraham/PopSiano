#include "Sms1130Device.h"
#include <SoyDebug.h>
#include <SoyTypes.h>
#include "TJob.h"
#include "TParameters.h"
#include "TChannel.h"



typedef uint32 UINT32;
typedef uint16 UINT16;
typedef uint8 UINT8;
typedef int32 INT32;
typedef int16 INT16;
typedef int8 INT8;
typedef long LONG;
typedef bool BOOL;

//#include "siano/SIANO_SMS1130/Platforms/RKOS/SmsLitePlatDefs.h"
#include "siano/SIANO_SMS1130/include/SmsHostLibLiteMs.h"
#include "siano/SIANO_SMS1130/include_internal/SmsFirmwareApi.h"
#include "siano/SIANO_SMS1130/include_internal/SmsHostLibLiteCommon.h"

//#define SMS_PLAT_DEFS_H
#include "siano/SIANO_SMS1130/include_internal/SmsLiteAppDriver.h"


namespace SmsMsgTypes
{
	typedef MsgTypes_E Type;
	
	DECLARE_SOYENUM_WITHINVALID(SmsMsgTypes,MSG_TYPE_BASE_VAL);
};

#define DEFINE_SOYENUM(Value)	{	Value, #Value	}

std::map<SmsMsgTypes::Type,std::string> SmsMsgTypes::EnumMap =
{
	DEFINE_SOYENUM( MSG_SMS_GET_VERSION_EX_REQ ),
	DEFINE_SOYENUM( MSG_SMS_GET_VERSION_EX_RES ),
	DEFINE_SOYENUM( MSG_SMS_GET_VERSION_RES ),
	DEFINE_SOYENUM( MSG_SMS_GET_VERSION_REQ ),
	DEFINE_SOYENUM( MSG_SMS_TRANSMISSION_IND ),
	DEFINE_SOYENUM( MSG_SMS_PID_STATISTICS_IND ),
	DEFINE_SOYENUM( MSG_SMS_POWER_DOWN_IND ),
	DEFINE_SOYENUM( MSG_SMS_POWER_DOWN_CONF ),
	DEFINE_SOYENUM( MSG_SMS_POWER_UP_IND ),
	DEFINE_SOYENUM( MSG_SMS_POWER_UP_CONF ),
	DEFINE_SOYENUM( MSG_SMS_CMMB_GET_CHANNELS_INFO_REQ ),
	DEFINE_SOYENUM( MSG_SMS_CMMB_GET_CHANNELS_INFO_RES ),
	DEFINE_SOYENUM( MSG_SMS_HO_PER_SLICES_IND ),
	DEFINE_SOYENUM( MSG_SMS_INIT_DEVICE_RES ),
};

std::ostream& operator<< ( std::ostream &out, const MsgTypes_E &in )
{
	out << SmsMsgTypes::ToString(in);
	return out;
}

std::ostream& operator<< ( std::ostream &out, const SMSHOSTLIB_ERR_CODES_E &in )
{
	out.setf(std::ios::hex, std::ios::basefield);
	out << in;
	out.unsetf(std::ios::hex);
	return out;
}


typedef struct IsdbtUserStats_S
{
	UINT32 NumBytes;
	UINT32 NumCallbacks;
	UINT32 LastOutputTime;
}
IsdbtUserStats_ST;

typedef struct IsdbtState_S
{
	//Event hResponseEvent;
	//Event hTuneEvent;
	IsdbtUserStats_ST Stats;
	void *pOutputFile;
	bool Is3Seg;
	UINT32 CurFreq;
	SMSHOSTLIB_MSG_TYPE_RES_E ExpectedResponse;
	SMSHOSTLIB_ERR_CODES_E ErrCode;
	void *pPayload;
	UINT32 PayloadLen;
	bool Signal_exist;
} IsdbtState_ST;

#define ISDBT_USER_CRISTAL  12000000L



typedef struct _QUALITY_CALC_ENTRY
{
	LONG	RangeEnd ;
	LONG	Grade ;
} QUALITY_CALC_ENTRY, *PQUALITY_CALC_ENTRY;






//	real worker
class TSianoLib : public SoyWorkerThread
{
public:
	TSianoLib(std::stringstream& Error);
	~TSianoLib();
	
	void				OnInit();
	
	virtual bool		Iteration() override;
	
	void				OnDataCallback(ArrayBridge<char>&& Buffer);
	void				OnControlCallback(ArrayBridge<char>&& Buffer);
	void				OnLiteControlCallback(SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,SMSHOSTLIB_ERR_CODES_E ErrCode,ArrayBridge<char>&& Buffer);
	
	
	UINT32				GetHandleNumber() const			{	return 0;	}
	
	void				OnNewCrystal(const ArrayBridge<char>& Data);
	void				OnDeviceInitialised(const ArrayBridge<char>& Data);
	void				OnNoSignal(const ArrayBridge<char>& Payload);
	void				OnDetectedSignal(const ArrayBridge<char>& Payload);
	void				OnHandoverPerSlicesIndication(const ArrayBridge<char>& Payload);

	void				OnStats(const TRANSMISSION_STATISTICS_ST& Stats);
	void				OnStats(const SMSHOSTLIB_STATISTICS_ISDBT_ST& Stats);
	void				OnStats(const SMSHOSTLIB_FAST_STATISTICS_ST& Stats);
	
public:
	TRANSMISSION_STATISTICS_ST				mStats;
	
	IsdbtState_ST g_IsdbtState;
	bool g_bHaveSignalIndicator;
	bool g_bDummyHaveSignal;
	UINT32 g_SignalStrength;
	UINT32 g_SNR;
	UINT32 g_BER;
	UINT32 g_RSSI;
	UINT32 g_InBandPower;
	char g_SignalQuality;
	
	Array<Array<uint8>>	mDataCallbacks;
	
	ofMutexM<bool>	mDoInit;
};

LONG ReceptionQualityVal2Grade( PQUALITY_CALC_ENTRY Table, LONG Val )
{
	UINT32 i=0 ;
	for ( i=0 ; Table[i].RangeEnd != -1 ; i++ )
	{
		if ( Val <= Table[i].RangeEnd )
		{
			return Table[i].Grade ;
		}
	}
	return 0 ;
}

UINT32 GetReceptionQuality( SMSHOSTLIB_STATISTICS_ISDBT_ST* StatParams )
{
	return 0;
}

/*
 extern SmsLiteMsGlobalState_ST	g_LibMsState				= { 0 };
 
 SMSHOSTLIB_ERR_CODES_E SmsLiteMsLibInit( SMSHOSTLIBLITE_MS_INITLIB_PARAMS_ST* pInitLibParams )
 {
	SMSHOSTLIBLITE_MS_INITLIB_PARAMS_ST LocalInitParams = {0};
	SMSHOSTLIB_ERR_CODES_E RetCode = SMSHOSTLIB_ERR_OK;
	SmsMsgData3Args_ST SmsMsg = {0};
	
	SMSHOST_LOG0(SMSLOG_APIS,"");
	DBG("TSTV: #################Sms###############\t%s[%d]\n", __FUNCTION__, __LINE__);
	
	if ( g_LibMsState.IsLibInit )
	{
 SMSHOST_LOG1(SMSLOG_APIS | SMSLOG_ERROR,"Return err 0x%x",SMSHOSTLIB_ERR_LIB_ALREADY_INITIATED);
 return SMSHOSTLIB_ERR_LIB_ALREADY_INITIATED;
	}
	
	//ZERO_MEM_OBJ(&g_LibMsState);
	memset(&g_LibMsState, 0, sizeof(g_LibMsState)); //jan
	
	if ( pInitLibParams == NULL
 || pInitLibParams->pCtrlCallback == NULL
 || pInitLibParams->Size == 0 )
	{
 SMSHOST_LOG1(SMSLOG_APIS | SMSLOG_ERROR,"Return err 0x%x",SMSHOSTLIB_ERR_INVALID_ARG);
 return SMSHOSTLIB_ERR_INVALID_ARG;
	}
	memcpy( &LocalInitParams, pInitLibParams, pInitLibParams->Size );
	
	SmsLiteInit( LocalInitParams.pCtrlCallback );
	
	g_LibMsState.DeviceMode = LocalInitParams.DeviceMode;
	
	g_LibMsState.pDataCallback = LocalInitParams.pDataCallback;
	g_LibMsState.Crystal = LocalInitParams.Crystal;
	if ( LocalInitParams.Crystal == 0 )
	{
 g_LibMsState.Crystal = SMSHOSTLIB_DEFAULT_CRYSTAL;
	}
	
	RetCode = SmsLiteAdrInit( g_LibMsState.DeviceMode,
 SmsLiteMsControlRxCallback,
 SmsLiteMsDataCallback );
	
	if ( RetCode != SMSHOSTLIB_ERR_OK )
	{
 SMSHOST_LOG1(SMSLOG_APIS | SMSLOG_ERROR,"Return err 0x%x",RetCode);
 return RetCode ;
	}
	
	// Device init message
	SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
	SmsMsg.xMsgHeader.msgType  = MSG_SMS_INIT_DEVICE_REQ;
	SmsMsg.xMsgHeader.msgLength = (UINT16)sizeof(SmsMsg);
	
	SmsMsg.msgData[0] = g_LibMsState.DeviceMode;
	
	g_LibMsState.SyncFlag = FALSE;
	SmsLiteSendCtrlMsg( (SmsMsgData_ST*)&SmsMsg );
	
	// Wait for device init response
	if ( !SmsHostWaitForFlagSet( &g_LibMsState.SyncFlag, 200 ) )
	{
 return SMSHOSTLIB_ERR_DEVICE_NOT_INITIATED;
	}
	
	
	// Set crystal message
	if ( g_LibMsState.Crystal != SMSHOSTLIB_DEFAULT_CRYSTAL )
	{
 SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
 SmsMsg.xMsgHeader.msgType  = MSG_SMS_NEW_CRYSTAL_REQ;
 SmsMsg.xMsgHeader.msgLength = (UINT16)sizeof(SmsMsg);
 
 SmsMsg.msgData[0] = g_LibMsState.Crystal;
 
 g_LibMsState.SyncFlag = FALSE;
 SmsLiteSendCtrlMsg( (SmsMsgData_ST*)&SmsMsg );
 
 // Wait for device init response
 if ( !SmsHostWaitForFlagSet( &g_LibMsState.SyncFlag, 200 ) )
 {
 return SMSHOSTLIB_ERR_DEVICE_NOT_INITIATED;
 }
	}
	
	
	g_LibMsState.IsLibInit = TRUE ;
 #ifdef SMSHOST_ENABLE_LOGS
	SmsLiteSetDeviceFwLogState();
 #endif
	
	//SmsLiteGetVersion_Req();
	
	SMSHOST_LOG0(SMSLOG_APIS,"LibInit OK");
	return SMSHOSTLIB_ERR_OK;
 }
 */


template<class ARRAY>
void Redirect_SmsLiteCallCtrlCallback(SMSHOSTLIB_MSG_TYPE_RES_E ResponseMsgType,SMSHOSTLIB_ERR_CODES_E RetCode,ARRAY& Payload)
{
	SmsLiteCallCtrlCallback( ResponseMsgType, RetCode, Payload.GetArray(), Payload.GetDataSize() );
}




SianoContainer::SianoContainer() :
SoyWorkerThread	( "SianoContainer", SoyWorkerWaitMode::Sleep )
{
	std::stringstream Error;
	if ( !CreateContext(Error) )
		return;
	Start();
}

SianoContainer::~SianoContainer()
{
	WaitToFinish();
	DestroyLib();
}

bool SianoContainer::InitLib(std::stringstream& Error)
{
	
	
	return true;
}

void SianoContainer::DestroyLib()
{
	mLib.reset();
}

void SianoContainer::GetDevices(ArrayBridge<TVideoDeviceMeta>& Metas)
{
	if ( mDevice )
	{
		Metas.PushBack( mDevice->GetMeta() );
	}
}


bool SianoContainer::CreateContext(std::stringstream& Error)
{
	mLib.reset( new TSianoLib(Error) );
	if ( !Error.str().empty() )
	{
		DestroyLib();
		return false;
	}
	
	mDevice.reset( new SianoDevice("tv", Error) );
	return false;
}

bool SianoContainer::Iteration()
{
	return true;
}

std::shared_ptr<TVideoDevice> SianoContainer::AllocDevice(const TVideoDeviceMeta& Meta,std::stringstream& Error)
{
	//	gr: double check meta?
	return mDevice;
}


SianoDevice::SianoDevice(const std::string& Serial,std::stringstream& Error) :
TVideoDevice	( Serial, Error ),
mSerial			( Serial )
{
	Open(Error);
}

TVideoDeviceMeta SianoDevice::GetMeta() const
{
	TVideoDeviceMeta Meta("tv");
	return Meta;
}

bool SianoDevice::Open(std::stringstream& Error)
{
	return true;
}

void SianoDevice::Close()
{
}

void SianoDevice::OnVideo(void *rgb, uint32_t timestamp)
{
	if ( !Soy::Assert( rgb, "rgb data expected" ) )
		return;
	/*
	 //std::Debug << "On video " << timestamp << std::endl;
	 
	 auto& Pixels = mVideoBuffer.GetPixelsArray();
	 int Bytes = std::min( mVideoMode.bytes, Pixels.GetDataSize() );
	 memcpy( Pixels.GetArray(), rgb, Bytes );
	 
	 //	notify change
	 SoyTime Timecode( static_cast<uint64>(timestamp) );
	 OnNewFrame( mVideoBuffer, Timecode );
	 */
}




class CountDevicesParams
{
public:
	char x[100];
};
class TryRegisterDeviceParams
{
public:
	uint32	mDeviceIndex;
	uint32*	mVendor;	//	b008xxxx	stack+f6c	108
	uint32*	mProduct;	//	b008xxxx	stack+f70	112
	void*	c;	//	b008xxxx	stack+000 null...
	uint32	thousand;	//	1000 first case, then 0
	void*	d;	//	b008xxxx
	int x[100];
};

//	gr; on stack, is pushed 0x22c/556 bytes
//		includes vendor(4) product(4) size=512(4)
//		36 other bytes of data...
//		open, pread and read are called... maybe an fd somewhere
//	gr: not sure if we're downloading or uploading...
//		null probbably ** to buffer

class FirmwareDownloadParams
{
public:
	uint32	mVendor;	//	0
	uint32	mProduct;	//	4
	uint32*	mNull;		//	8
	uint32* mDataRead;		//	12
	
	uint32*	mx700;	//	value is 0x700/1792
	uint32	mBufferSize;	//	value is 0x200/512
	char	mDummy[4+12];
	char	mFilenameBuffer[512];
	uint32*	mDataTotal;
	uint32*	mValues[3];
};
//static_assert(sizeof(FirmwareDownloadParams)==556,"Wrong size struct according to dissaemly");

typedef uint32(*cbCountDevices)(CountDevicesParams);
typedef int(*cbTryRegisterDevice)(TryRegisterDeviceParams);
typedef int(*cbFirmwareDownloadCallback)(FirmwareDownloadParams);
#include <iomanip>


//	gr: can't use lambda with a capture variable as a function pointer
static TSianoLib* gLib = nullptr;
uint32 SanioVendor = 0x187f;
uint32 SanioProduct = 0x0202;

class TUsbDeviceIdentifier
{
public:
	TUsbDeviceIdentifier(int Vendor,int Product,const std::string& Filename) :
	mVendor				( Vendor ),
	mProduct			( Product ),
	mFirmwareFilename	( Filename )
	{
	}
	
	uint32	mVendor;
	uint32	mProduct;
	std::string	mFirmwareFilename;
};

TUsbDeviceIdentifier gDevices[] =
{
	TUsbDeviceIdentifier( SanioVendor, SanioProduct, "firmware/dvb_nova_12mhz_b0.fw" ),
};

	
	
	
	
	
	
	
class RegisterUsbStruct
{
public:
	RegisterUsbStruct() :
	Pad						{ 0,1,2,3 },
	mFirmwareDownloadCallback	( &DownloadFirmware ),
	mCountDevicesCallback	( &CountDevices ),
	mTryRegisterCallback	( &TryRegister )
	{
		std::Debug << "this(RegisterUsbStruct*): " << this << std::endl;
		for ( int d=0;	d<sizeofarray(Data);	d++ )
			Data[d] = d % 256;
	}
	
	static int		DownloadFirmware(FirmwareDownloadParams a)
	{
		TUsbDeviceIdentifier* Device = nullptr;
		for ( int d=0;	d<sizeofarray(gDevices);	d++ )
		{
			auto& Dev = gDevices[d];
			if ( Dev.mProduct != a.mProduct )
				continue;
			if ( Dev.mVendor != a.mVendor )
				continue;
			Device = &Dev;
		}
		if ( !Device )
			return false;
		
		//	get filename into buffer for caller
		std::Debug << "Sending firmware filename; " << Device->mFirmwareFilename << std::endl;
		strcpy( a.mFilenameBuffer, Device->mFirmwareFilename.c_str() );
		
		std::Debug << "stack addr: " << (void*)(&a) << std::endl;
		
		return true;
	}
	
	static uint32		CountDevices(CountDevicesParams a)
	{
		std::Debug << "CountDevices()" << std::endl;
		return sizeofarray(gDevices);
	}
	
	static int		TryRegister(TryRegisterDeviceParams a)
	{
		*a.mProduct = gDevices[a.mDeviceIndex].mProduct;
		*a.mVendor = gDevices[a.mDeviceIndex].mVendor;
		
		std::Debug << "TryRegister(" << (int)a.mDeviceIndex << ") ";
		std::Debug << "[c]" << a.c << " ";
		std::Debug << "[thou]" << a.thousand << " ";
		std::Debug << "[d]" << a.d << " ";
		std::Debug.setf(std::ios::hex, std::ios::basefield);
		for ( int i=0;	i<10;	i++ )
			std::Debug << a.x[i] << ",";
		std::Debug.unsetf(std::ios::hex);
		std::Debug << std::endl;
		static int ret = 0;
		//		std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		return ret;
	}
	
	char	Pad[4];
	cbFirmwareDownloadCallback	mFirmwareDownloadCallback;
	cbCountDevices			mCountDevicesCallback;
	cbTryRegisterDevice		mTryRegisterCallback;
	char	Data[100];
};




class OnDeviceChangedParams
{
public:
	uint32	mAdded;				//	0x1, 0 on removed...
	
	uint32	mSomething;				//	<something> on added, 2 on removed
	uint32*	mPointerToPointer;		//	null on removed
	uint32* mPointerToSomething;	//	content null on removed
	uint32*	Params[10];	//
};

typedef int ( *SmsLiteAdr_DeviceChangedCb )(OnDeviceChangedParams);

extern "C"
{
	SMSHOSTLIB_ERR_CODES_E SmsAdr_RegisterUSBPersonalitiesAPI(void*);
	//void SmsAdr_SmsDeviceInitTimerCallBack(void);
	
	SMSHOSTLIB_ERR_CODES_E SmsLiteAdrInit( SMSHOSTLIB_DEVICE_MODES_E DeviceMode,
										  SmsLiteAdr_pfnFuncCb pfnControlCb,
										  SmsLiteAdr_pfnFuncCb pfnDataCb,
										  SmsLiteAdr_DeviceChangedCb OnDeviceChangedFuncCb,
										  uint32 Param3,
										  uint32 Param4,
										  uint32 Param5
										  );
	
	
};


auto OnDeviceChanged = [](OnDeviceChangedParams a)
{
	std::Debug << "Device changed: " << std::endl;
	std::Debug << "Added device: " << a.mAdded << std::endl;
	std::Debug << "something: " << a.mSomething << std::endl;
	
	if ( a.mAdded )
	{
		gLib->mDoInit.lock();
		gLib->mDoInit.mMember = true;
		gLib->mDoInit.unlock();
	}
	//	gr: reutrning 0 hung on close()...
	//	gr: -1 continued and did some IO stuff...
	static int Return = -1;
	return Return;
};

auto Crystal = SMSHOSTLIB_DEFAULT_CRYSTAL;
//	auto Crystal = ISDBT_USER_CRISTAL;
auto DeviceMode = SMSHOSTLIB_DEVMD_DVBT;


TSianoLib::TSianoLib(std::stringstream& Error) :
SoyWorkerThread			( "SianoLib", SoyWorkerWaitMode::Sleep ),
g_bHaveSignalIndicator	( false ),
g_bDummyHaveSignal		( false ),
g_SignalStrength		( 0 ),
g_SNR					( 0 ),
g_BER					( 0 ),
g_RSSI					( 0 ),
g_InBandPower			( 0 ),
g_SignalQuality			( 0 )
{
	gLib = this;
	static RegisterUsbStruct Dummy;
	
	auto ControlCallbackWrapper = [](UINT32 handle_num,UINT8* p_buffer,UINT32 buff_size)
	{
		if ( !Soy::Assert( p_buffer != nullptr && buff_size > 0, "invalid buffer spec" ) )
			return;
		if ( !Soy::Assert( handle_num == gLib->GetHandleNumber(), "expecting only 0 handle" ) )
			return;
		
		int BufferSize = buff_size;
		auto Buffer = GetRemoteArray( reinterpret_cast<const char*>(p_buffer), BufferSize, BufferSize );
		//SmsMsgData_ST* pSmsMsg = reinterpret_cast<SmsMsgData_ST*>(p_buffer);
		gLib->OnControlCallback( GetArrayBridge( Buffer ) );
	};
	
	auto LiteControlCallbackWrapper = [](SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,		//!< Response type
										 SMSHOSTLIB_ERR_CODES_E		ErrCode,		//!< Response success code
										 UINT8* 						p_buffer,		//!< Response payload
										 UINT32						buff_size)
	{
		if ( !Soy::Assert( p_buffer != nullptr && buff_size > 0, "invalid buffer spec" ) )
			return;
		int BufferSize = buff_size;
		auto Buffer = GetRemoteArray( reinterpret_cast<const char*>(p_buffer), BufferSize, BufferSize );
		gLib->OnLiteControlCallback( MsgType, ErrCode, GetArrayBridge(Buffer) );
	};
	
	auto DataCallbackWrapper = [](UINT32 handle_num,UINT8* p_buffer,UINT32 buff_size)
	{
		if ( !Soy::Assert( p_buffer != nullptr && buff_size > 0, "invalid buffer spec" ) )
			return;
		if ( !Soy::Assert( handle_num == gLib->GetHandleNumber(), "expecting only 0 handle" ) )
			return;
		
		int BufferSize = buff_size;
		auto Buffer = GetRemoteArray( reinterpret_cast<const char*>(p_buffer), BufferSize, BufferSize );
		gLib->OnDataCallback( GetArrayBridge(Buffer) );
	};
	
	
	//	initialise devices before we can communicate
	SMSHOSTLIB_ERR_CODES_E Result = SmsAdr_RegisterUSBPersonalitiesAPI(&Dummy);
	
	
	Result = SmsLiteInit( LiteControlCallbackWrapper );
	if( Result != SMSHOSTLIB_ERR_OK)
	{
		Error << "SmsLiteMsLibInit() result: " << Result;
		return;
	}
	
	//	gr: these aren't used, must just have one extra param
	uint32 Param3 = 0x34343434;
	uint32 Param4 = 0x56565656;
	uint32 Param5 = 0x78787878;
	Result = SmsLiteAdrInit( DeviceMode,
							ControlCallbackWrapper,
							DataCallbackWrapper,
							OnDeviceChanged, Param3, Param4, Param5
							);
	if( Result != SMSHOSTLIB_ERR_OK)
	{
		Error << "SmsLiteMsLibInit() result: " << Result;
		return;
	}
	
	
	//SmsAdr_SmsDeviceInitTimerCallBack();
	std::Debug << "hello" << std::endl;
	
	Start();
}


TSianoLib::~TSianoLib()
{
	SmsLiteMsLibTerminate();
}


bool TSianoLib::Iteration()
{
	bool DoInit = false;
	mDoInit.lock();
	if ( mDoInit.mMember )
	{
		DoInit = true;
		mDoInit.mMember = false;
	}
	mDoInit.unlock();
	
	if ( DoInit )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds(3000) );
		OnInit();
	}
	
	return true;
}



void TSianoLib::OnInit()
{
	std::stringstream Error;
	
	//	get versions
	{
		SmsMsgData_ST SmsMsg = {0};
		SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
		SmsMsg.xMsgHeader.msgSrcId = SMS_HOST_LIB_INTERNAL;
		SmsMsg.xMsgHeader.msgDstId = HIF_TASK;
		SmsMsg.xMsgHeader.msgFlags = 0;
		SmsMsg.xMsgHeader.msgType  = MSG_SMS_GET_VERSION_EX_REQ;
		SmsMsg.xMsgHeader.msgLength = sizeof(SmsMsg);
		SmsLiteAdrWriteMsg( &SmsMsg );
	}
	
	{
		SmsMsgData_ST SmsMsg = {0};
		SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
		SmsMsg.xMsgHeader.msgSrcId = SMS_HOST_LIB_INTERNAL;
		SmsMsg.xMsgHeader.msgDstId = HIF_TASK;
		SmsMsg.xMsgHeader.msgFlags = 0;
		SmsMsg.xMsgHeader.msgType  = MSG_SMS_GET_VERSION_REQ;
		SmsMsg.xMsgHeader.msgLength = sizeof(SmsMsg);
		SmsLiteAdrWriteMsg( &SmsMsg );
	}
	
	
	{
		SmsMsgData3Args_ST SmsMsg = {0};
		SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
		SmsMsg.xMsgHeader.msgType  = MSG_SMS_INIT_DEVICE_REQ;
		SmsMsg.xMsgHeader.msgLength = sizeof(SmsMsg);
		SmsMsg.msgData[0] = DeviceMode;
		SmsLiteAdrWriteMsg( (SmsMsgData_ST*)&SmsMsg );
	}
	/*
	 //	Set crystal message
	 if ( Crystal != SMSHOSTLIB_DEFAULT_CRYSTAL )
	 {
	 SmsMsgData3Args_ST SmsMsg = {0};
	 SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
	 SmsMsg.xMsgHeader.msgType  = MSG_SMS_NEW_CRYSTAL_REQ;
	 SmsMsg.xMsgHeader.msgLength = (UINT16)sizeof(SmsMsg);
	 
	 SmsMsg.msgData[0] = Crystal;
	 
	 mSyncFlag = false;
	 SmsLiteSendCtrlMsg( (SmsMsgData_ST*)&SmsMsg );
	 
	 // Wait for device init response
	 if ( !SmsHostWaitForFlagSet( &mSyncFlag, 200 ) )
	 {
	 Error << "Failed to set crystal";
	 return;
	 }
	 }
	 
	 /*
	 SMSHOSTLIBLITE_MS_INITLIB_PARAMS_ST InitLibParams = {0};
	 
	 //Initialize SMS11xx host library.
	 InitLibParams.Size = sizeof(InitLibParams);
	 InitLibParams.pCtrlCallback = CallbackWrapper;
	 InitLibParams.pDataCallback = DataCallbackWrapper;
	 InitLibParams.Crystal = Crystal;
	 #ifdef TUNER_ISDB_T
	 InitLibParams.DeviceMode = SMSHOSTLIB_DEVMD_ISDBT;
	 #else
	 InitLibParams.DeviceMode = SMSHOSTLIB_DEVMD_DVBT;
	 #endif
	 auto Result = SmsLiteMsLibInit(&InitLibParams);
	 if( Result !=SMSHOSTLIB_ERR_OK)
	 {
	 Error << "SmsLiteMsLibInit() result: " << Result;
	 return;
	 }
	 */
}



void TSianoLib::OnControlCallback(ArrayBridge<char>&& Buffer)
{
	SmsMsgData_ST* pSmsMsg = reinterpret_cast<SmsMsgData_ST*>( Buffer.GetArray() );
	if ( !Soy::Assert( Buffer.GetDataSize() >= pSmsMsg->xMsgHeader.msgLength, "Buffer too small for described message length" ) )
		return;
	if ( !Soy::Assert( pSmsMsg->xMsgHeader.msgLength >= sizeof( SmsMsgHdr_ST ), "Message header larger than expected" ) )
		return;
	
	auto* pPayload = reinterpret_cast<const char*>(&pSmsMsg->msgData[0]);
	int PayloadLength = pSmsMsg->xMsgHeader.msgLength - sizeof( SmsMsgHdr_ST );
	auto Payload = GetRemoteArray( pPayload, PayloadLength, PayloadLength );
	
	//	Return code and payload for the messages which have retcode as the first 4 bytes
	SMSHOSTLIB_ERR_CODES_E RetCodeFromMsg = SMSHOSTLIB_ERR_UNDEFINED_ERR;
	if ( PayloadLength >= 4 )
	{
		RetCodeFromMsg = static_cast<SMSHOSTLIB_ERR_CODES_E>(pSmsMsg->msgData[0]);
	}
	int PayloadLengthWoRetCode = PayloadLength-4;
	auto PayloadWoRetCode = GetRemoteArray( pPayload+4, PayloadLengthWoRetCode, PayloadLengthWoRetCode );
	
	
	
	//	debug
	auto MsgType = static_cast<MsgTypes_E>(pSmsMsg->xMsgHeader.msgType);
	std::Debug << "Control callback. message type " << MsgType << ", Payload Length " << PayloadLength << std::endl;
	
	switch( MsgType )
	{
		case MSG_SMS_NEW_CRYSTAL_RES:
			OnNewCrystal( Buffer );
			break;
			
		case MSG_SMS_INIT_DEVICE_RES:
			OnDeviceInitialised( Buffer );
			break;
			
		case MSG_SMS_TRANSMISSION_IND:
		{
			// Update the DVBT statistics. No need for a response to the app.
			if ( !Soy::Assert( sizeof(mStats) == Payload.GetDataSize(), "Payload wrong size for TRANSMISSION_STATISTICS_ST" ) )
				return;
			
			auto* Stats = reinterpret_cast<TRANSMISSION_STATISTICS_ST*>( Payload.GetArray() );
			OnStats(*Stats);
			//no need to correct guard interval (as opposed to old statistics message).
			//CORRECT_STAT_BANDWIDTH(g_LibMsState.DvbtStatsCache.TransmissionData);
			//CORRECT_STAT_TRANSMISSON_MODE(g_LibMsState.DvbtStatsCache.TransmissionData);
		}
			break;
			
			
		case MSG_SMS_HO_PER_SLICES_IND:
			// Update the DVBT statistics. No need for a response to the app.
			//SmsHandlePerSlicesIndication(pSmsMsg);
			OnHandoverPerSlicesIndication( GetArrayBridge(Payload) );
			break;
			
		case MSG_SMS_SIGNAL_DETECTED_IND:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_SMS_SIGNAL_DETECTED_IND, SMSHOSTLIB_ERR_OK, Payload );
			break;
			
		case MSG_SMS_NO_SIGNAL_IND:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_SMS_NO_SIGNAL_IND, SMSHOSTLIB_ERR_OK, Payload );
			break;
			
		case MSG_SMS_ADD_PID_FILTER_RES:
		{
			SMSHOSTLIB_ERR_CODES_E RetCode = SMSHOSTLIB_ERR_UNDEFINED_ERR;
			
			switch( RetCodeFromMsg )
			{
				case SMS_S_OK:
					RetCode = SMSHOSTLIB_ERR_OK;
					break;
				case SMS_E_ALREADY_EXISTING:
					RetCode = SMSHOSTLIB_ERR_ALREADY_EXIST;
					break;
				case SMS_E_MAX_EXCEEDED:
					RetCode = SMSHOSTLIB_ERR_LIST_FULL;
					break;
				default:
					RetCode = SMSHOSTLIB_ERR_UNDEFINED_ERR;
					break;
			}
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_ADD_PID_FILTER_RES, RetCode, PayloadWoRetCode );
		}
			break;
			
		case MSG_SMS_REMOVE_PID_FILTER_RES:
		{
			SMSHOSTLIB_ERR_CODES_E RetCode = RetCodeFromMsg;
			if ( RetCodeFromMsg == SMS_E_NOT_FOUND )
				RetCode = SMSHOSTLIB_ERR_PID_FILTER_DOES_NOT_EXIST;
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_REMOVE_PID_FILTER_RES, RetCode, PayloadWoRetCode );
		}
			break;
			
		case MSG_SMS_GET_PID_FILTER_LIST_RES:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_RETRIEVE_PID_FILTER_LIST_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		case MSG_SMS_RF_TUNE_RES:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_TUNE_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		case MSG_SMS_ISDBT_TUNE_RES:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_ISDBT_TUNE_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		case MSG_SMS_GET_STATISTICS_EX_RES:
			// Statistics EX response - relevant only for ISDBT
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_GET_STATISTICS_EX_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		case MSG_SMS_GET_STATISTICS_RES:
			// Statistics EX response - relevant only for ISDBT
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_GET_STATISTICS_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		case MSG_SMS_DUMMY_STAT_RES:
			// I2C Statistics response - relevant only for DVB-T
			//pPayload = pPayloadWoRetCode;
			//PayloadLength = PayloadLengthWoRetCode;
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_GET_STATISTICS_RES, SMSHOSTLIB_ERR_OK, Payload );
			break;
			
		case MSG_SMS_SET_AES128_KEY_RES:
			Redirect_SmsLiteCallCtrlCallback( SMSHOSTLIB_MSG_SET_AES128_KEY_RES, RetCodeFromMsg, PayloadWoRetCode );
			break;
			
		default:
			std::Debug << "Passing " << MsgType << " to common SmsLite handler" << std::endl;
			SmsLiteCommonControlRxHandler( GetHandleNumber(), reinterpret_cast<UINT8*>(Buffer.GetArray()), Buffer.GetDataSize() );
			break;
	}
}


void TSianoLib::OnDataCallback(ArrayBridge<char> &&Buffer)
{
	std::Debug << "Incoming data: " << Soy::FormatSizeBytes( Buffer.GetDataSize() ) << std::endl;
}

void TSianoLib::OnLiteControlCallback(SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,SMSHOSTLIB_ERR_CODES_E ErrCode,ArrayBridge<char>&& Payload)
{
	//Handle indications.
	switch ( MsgType )
	{
		case SMSHOSTLIB_MSG_GET_VERSION_RES:
		{
			std::string VersionString( Payload.GetArray() );
			std::Debug << "Version:" << VersionString << std::endl;
		}
			break;
			
		case SMSHOSTLIB_MSG_SMS_SIGNAL_DETECTED_IND:
			OnDetectedSignal( Payload );
			//g_bHaveSignalIndicator = true;
			//g_IsdbtState.Signal_exist = true;
			//		OSW_EventSet(&g_IsdbtState.hTuneEvent);
			break;
			
		case SMSHOSTLIB_MSG_SMS_NO_SIGNAL_IND:
			OnNoSignal( Payload );
			//g_bHaveSignalIndicator = true;
			//g_IsdbtState.Signal_exist = false;
			//		OSW_EventSet(&g_IsdbtState.hTuneEvent);
			break;
			
		case SMSHOSTLIB_MSG_GET_STATISTICS_EX_RES:
		{
			auto* Stats = reinterpret_cast<SMSHOSTLIB_STATISTICS_ISDBT_ST*>( Payload.GetArray() );
			OnStats( *Stats );
			
			/*
			 g_SignalQuality = (char)GetReceptionQuality(pStat);
			 g_SignalStrength = pStat->InBandPwr;
			 
			 g_SNR = pStat->SNR;
			 g_BER = pStat->LayerInfo[0].BER;
			 g_RSSI = pStat->RSSI;
			 g_InBandPower = pStat->InBandPwr;
			 
			 std::Debug << "TSTV:SIANO: g_SignalQuality==" << g_SignalQuality << std::endl;
			 std::Debug << "TSTV:SIANO: g_SignalStrength==" << g_SignalStrength << std::endl;
			 std::Debug << "TSTV:SIANO: g_SNR==" << g_SNR << std::endl;
			 std::Debug << "TSTV:SIANO: g_BER==" << g_BER << std::endl;
			 std::Debug << "TSTV:SIANO: g_RSSI==" << g_RSSI << std::endl;
			 */
		}
			break;
			
		case SMSHOSTLIB_MSG_GET_STATISTICS_RES:
		{
			auto* Stats = reinterpret_cast<SMSHOSTLIB_FAST_STATISTICS_ST*>( Payload.GetArray() );
			OnStats( *Stats );
			/*
			 if (TRUE==pStat->DemodLocked){
			 g_SignalQuality = (char)GetDvbtReceptionQuality(pStat);
			 } else {
			 g_SignalQuality = 0;
			 }
			 g_SignalStrength = (char)pStat->InBandPwr;
			 g_SNR = pStat->SNR;
			 g_BER = pStat->BER;
			 g_InBandPower=pStat->InBandPwr;
			 printk("TSTV:SIANO: [SMS]Lock=%d, InBandPwr=%d, SNR=%d, BER=%d ValidTsPak=%d, ErrTsPak=%d\n",pStat->DemodLocked,pStat->InBandPwr, pStat->SNR, pStat->BER,pStat->TotalTSPackets,pStat->ErrorTSPackets);
			 
			 if (pStat->DemodLocked == TRUE )
			 {      g_bDummyHaveSignal = TRUE;
			 g_IsdbtState.Signal_exist = TRUE;
			 }
			 */
		}
			break;
			
		default:
			break;
	}
}



void TSianoLib::OnNewCrystal(const ArrayBridge<char>& Data)
{
	std::Debug << __func__ << std::endl;
}

void TSianoLib::OnDeviceInitialised(const ArrayBridge<char>& Data)
{
	std::Debug << __func__ << std::endl;
}

void TSianoLib::OnNoSignal(const ArrayBridge<char>& Payload)
{
	std::Debug << __func__ << std::endl;
}
	
void TSianoLib::OnDetectedSignal(const ArrayBridge<char>& Payload)
{
	std::Debug << __func__ << std::endl;
}
	
void TSianoLib::OnHandoverPerSlicesIndication(const ArrayBridge<char>& Payload)
{
	std::Debug << __func__ << std::endl;
}

void TSianoLib::OnStats(const TRANSMISSION_STATISTICS_ST& Stats)
{
	std::Debug << __func__ << std::endl;
}

void TSianoLib::OnStats(const SMSHOSTLIB_STATISTICS_ISDBT_ST& Stats)
{
	std::Debug << __func__ << std::endl;
}

void TSianoLib::OnStats(const SMSHOSTLIB_FAST_STATISTICS_ST& Stats)
{
	
}




