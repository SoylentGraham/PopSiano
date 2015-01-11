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
class TSianoLib
{
public:
	TSianoLib(std::stringstream& Error);
	~TSianoLib();
	
	void	IsdbtUserCtrlCallback( SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,
										  SMSHOSTLIB_ERR_CODES_E		ErrCode,
										  void* 						pPayload,
										  UINT32						PayloadLen);
	void	IsdbtUserDataCallback(UINT32 ServiceDevHandle,UINT8* pBuf,UINT32 BufSize);

public:
	bool	mSyncFlag;
	
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

void SmsLiteMsControlRxCallback(  UINT32 handle_num, UINT8* p_buffer, UINT32 buff_size )
{
	/*
	SmsMsgData_ST* pSmsMsg = (SmsMsgData_ST*)p_buffer;
	SMSHOSTLIB_ERR_CODES_E RetCode = SMSHOSTLIB_ERR_UNDEFINED_ERR;
	UINT32 ResponseMsgType = SMSHOSTLIB_MSG_INVALID_RESPONSE_VAL;
	UINT8* pPayload = NULL;
	UINT32 PayloadLength = 0;
	
	// Return code and payload for the messages which have retcode as the first 4 bytes
	UINT8* pPayloadWoRetCode = NULL;
	UINT32 PayloadLengthWoRetCode = 0;
	SMSHOSTLIB_ERR_CODES_E RetCodeFromMsg = SMSHOSTLIB_ERR_UNDEFINED_ERR;
	
	SMS_ASSERT( handle_num == 0 );
	SMS_ASSERT( p_buffer != NULL );
	SMS_ASSERT( buff_size != 0 );
	SMS_ASSERT( buff_size >= pSmsMsg->xMsgHeader.msgLength );
	SMS_ASSERT( pSmsMsg->xMsgHeader.msgLength >= sizeof( SmsMsgHdr_ST ) );
	
	pPayload = (UINT8*)&pSmsMsg->msgData[0];
	PayloadLength = pSmsMsg->xMsgHeader.msgLength - sizeof( SmsMsgHdr_ST );
	
	if ( PayloadLength >= 4 )
	{
		
		RetCodeFromMsg = pSmsMsg->msgData[0];
		pPayloadWoRetCode = pPayload + 4;
		PayloadLengthWoRetCode = PayloadLength - 4;
	}
	
	SMSHOST_LOG3( SMSLOG_ERROR, "Control callback. Type %d, Retcode %#x, Payload Length %d",
				 pSmsMsg->xMsgHeader.msgType,
				 RetCode,
				 PayloadLength );
	
	switch( pSmsMsg->xMsgHeader.msgType )
	{
		case MSG_SMS_NEW_CRYSTAL_RES:
		case MSG_SMS_INIT_DEVICE_RES:
		{
			g_LibMsState.SyncFlag = TRUE;
		}
			break;
		case MSG_SMS_TRANSMISSION_IND:
		{
			// Update the DVBT statistics. No need for a response to the app.
			
			memcpy(	&g_LibMsState.DvbtStatsCache.TransmissionData,
				   (TRANSMISSION_STATISTICS_ST*)pSmsMsg->msgData,
				   sizeof( g_LibMsState.DvbtStatsCache.TransmissionData ));
			g_LibMsState.DvbtStatsCache.ReceptionData.IsDemodLocked = 0;
			
			//no need to correct guard interval (as opposed to old statistics message).
			CORRECT_STAT_BANDWIDTH(g_LibMsState.DvbtStatsCache.TransmissionData);
			CORRECT_STAT_TRANSMISSON_MODE(g_LibMsState.DvbtStatsCache.TransmissionData);
		}
			break;
		case MSG_SMS_HO_PER_SLICES_IND:
		{
			// Update the DVBT statistics. No need for a response to the app.
			SmsHandlePerSlicesIndication(pSmsMsg);
		}
			break;
		case MSG_SMS_SIGNAL_DETECTED_IND:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_SMS_SIGNAL_DETECTED_IND;
			RetCode = SMSHOSTLIB_ERR_OK;
		}
			break;
		case MSG_SMS_NO_SIGNAL_IND:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_SMS_NO_SIGNAL_IND;
			RetCode = SMSHOSTLIB_ERR_OK;
		}
			break;
		case MSG_SMS_ADD_PID_FILTER_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_ADD_PID_FILTER_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			
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
		}
			break;
		case MSG_SMS_REMOVE_PID_FILTER_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_REMOVE_PID_FILTER_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			RetCode = RetCodeFromMsg;
			
			if ( RetCodeFromMsg == SMS_E_NOT_FOUND )
			{
				RetCode = SMSHOSTLIB_ERR_PID_FILTER_DOES_NOT_EXIST;
			}
		}
			break;
		case MSG_SMS_GET_PID_FILTER_LIST_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_RETRIEVE_PID_FILTER_LIST_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			RetCode = RetCodeFromMsg;
		}
			break;
		case MSG_SMS_RF_TUNE_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_TUNE_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			RetCode = RetCodeFromMsg;
		}
			break;
		case MSG_SMS_ISDBT_TUNE_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_ISDBT_TUNE_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			RetCode = RetCodeFromMsg;
		}
			break;
		case MSG_SMS_GET_STATISTICS_EX_RES:
		{
			// Statistics EX response - relevant only for ISDBT
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			ResponseMsgType = SMSHOSTLIB_MSG_GET_STATISTICS_EX_RES;
			RetCode = RetCodeFromMsg;
		}
			break;
		case MSG_SMS_GET_STATISTICS_RES:
		{
			// Statistics EX response - relevant only for ISDBT
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			ResponseMsgType = SMSHOSTLIB_MSG_GET_STATISTICS_RES;
			RetCode = RetCodeFromMsg;
		}
			break;
		case MSG_SMS_DUMMY_STAT_RES:
		{
			// I2C Statistics response - relevant only for DVB-T
			//pPayload = pPayloadWoRetCode;
			//PayloadLength = PayloadLengthWoRetCode;
			ResponseMsgType = SMSHOSTLIB_MSG_GET_STATISTICS_RES;
			RetCode = SMSHOSTLIB_ERR_OK;
		}
			break;
		case MSG_SMS_SET_AES128_KEY_RES:
		{
			ResponseMsgType = SMSHOSTLIB_MSG_SET_AES128_KEY_RES;
			pPayload = pPayloadWoRetCode;
			PayloadLength = PayloadLengthWoRetCode;
			RetCode = RetCodeFromMsg;
		}
			break;
			
		default:
			SmsLiteCommonControlRxHandler( handle_num, p_buffer, buff_size );
			break;
	}
	
	// Call the user callback
	if ( ResponseMsgType != SMSHOSTLIB_MSG_INVALID_RESPONSE_VAL )
	{
		SmsLiteCallCtrlCallback( ResponseMsgType, RetCode, pPayload, PayloadLength );
	}
	*/
}

extern "C"
{
	SMSHOSTLIB_ERR_CODES_E SmsAdr_RegisterUSBPersonalitiesAPI(void*);
	//void SmsAdr_SmsDeviceInitTimerCallBack(void);
};

void FunctionTest()
{
	std::Debug << "called" <<std::endl;
}

class CountDevicesParams
{
public:
	char x[100];
};
class TryRegisterDeviceParams
{
public:
	uint32	mRemainingCalls;
	int x[100];
};
typedef uint32(*cbCountDevices)(CountDevicesParams);
typedef int(*cbTryRegisterDevice)(TryRegisterDeviceParams);

class RegisterUsbStruct
{
public:
	RegisterUsbStruct() :
		Pad			{ 0,1,2,3,4,5,6,7 },
		mCountDevicesCallback	( &CountDevices ),
		mTryRegisterCallback	( &TryRegister ),
		Data		{ 8,9,10,11,12,13,14,15,16,17,18,19,20,21,22 }
	{
	}
	static uint32		CountDevices(CountDevicesParams a)
	{
		std::Debug << "CountDevices()" << std::endl;
		static int CallFuncbXTimes = 100;
		return CallFuncbXTimes;	//	caller seems to check for non-zero?
	}
	static int		TryRegister(TryRegisterDeviceParams a)
	{
		std::Debug << "TryRegister(" << (int)a.mRemainingCalls << ") ";
		for ( int i=0;	i<10;	i++ )
			std::Debug << a.x[i] << " ";
		std::Debug << std::endl;
		static int ret = 1;
		return ret;
	}
	
	char	Pad[8];
	cbCountDevices			mCountDevicesCallback;
	cbTryRegisterDevice		mTryRegisterCallback;
	char	Data[100];
};
RegisterUsbStruct Dummy;


//	gr: can't use lambda with a capture variable as a function pointer
static TSianoLib* gLib = nullptr;
auto CallbackWrapper = [](SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,		//!< Response type
						  SMSHOSTLIB_ERR_CODES_E		ErrCode,		//!< Response success code
						  UINT8* 						pPayload,		//!< Response payload
						  UINT32						PayloadLen)
{
	gLib->IsdbtUserCtrlCallback( MsgType, ErrCode, pPayload, PayloadLen );
};

auto DataCallbackWrapper = [](UINT32 ServiceDevHandle,UINT8* pBuf,UINT32 BufSize)
{
	gLib->IsdbtUserDataCallback( ServiceDevHandle, pBuf, BufSize );
};

auto DeviceCallbackWrapper = [](void* ClientPtr1, UINT32 handle_num1, UINT8* p_buffer1, UINT32 buff_size1, UINT32 xx)
{
	std::Debug << "device callback" << std::endl;
	//		typedef void ( *ADR_pfnFuncCb1 )(  void* ClientPtr1, UINT32 handle_num1, UINT8* p_buffer1, UINT32 buff_size1, UINT32 xx );
	
};

TSianoLib::TSianoLib(std::stringstream& Error) :
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

	auto Crystal = SMSHOSTLIB_DEFAULT_CRYSTAL;
//	auto Crystal = ISDBT_USER_CRISTAL;
	auto DeviceMode = SMSHOSTLIB_DEVMD_DVBT;
	
	//	initialise devices before we can communicate
	SMSHOSTLIB_ERR_CODES_E Result = SmsAdr_RegisterUSBPersonalitiesAPI(&Dummy);

	
	Result = SmsLiteInit( CallbackWrapper );
	if( Result != SMSHOSTLIB_ERR_OK)
	{
		Error << "SmsLiteMsLibInit() result: " << Result;
		return;
	}
	
	Result = SmsLiteAdrInit( DeviceMode,
							 SmsLiteMsControlRxCallback,
							 DataCallbackWrapper );
	if( Result != SMSHOSTLIB_ERR_OK)
	{
		Error << "SmsLiteMsLibInit() result: " << Result;
		return;
	}
	
	
	//SmsAdr_SmsDeviceInitTimerCallBack();
	std::Debug << "hello" << std::endl;
	
/*
	//	open device handle
	void* Device = nullptr;
	UINT32 HandleNum = 0;
	Result = ADR_OpenHandle1( Device, HandleNum, DeviceCallbackWrapper, this );
	if( Result != SMSHOSTLIB_ERR_OK)
	{
		Error << "ADR_OpenHandle1() result: " << Result;
		return;
	}

	*/
	
/*
	//	get version first
	
	{
		UINT16 Len = sizeof(SmsMsgHdr_ST);
		SmsMsgData_ST SmsMsg = {0};
		SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
		SmsMsg.xMsgHeader.msgSrcId = SMS_HOST_LIB_INTERNAL;
		SmsMsg.xMsgHeader.msgDstId = HIF_TASK;
		SmsMsg.xMsgHeader.msgFlags = 0;
		SmsMsg.xMsgHeader.msgType  = MSG_SMS_GET_VERSION_EX_REQ;
		SmsMsg.xMsgHeader.msgLength = Len;
		SmsLiteAdrWriteMsg( &SmsMsg );
	}
	
	
	{
		SmsMsgData3Args_ST SmsMsg = {0};
		SMS_SET_HOST_DEVICE_STATIC_MSG_FIELDS( &SmsMsg );
		SmsMsg.xMsgHeader.msgType  = MSG_SMS_INIT_DEVICE_REQ;
		SmsMsg.xMsgHeader.msgLength =
		SmsMsg.msgData[0] = DeviceMode;
		
		mSyncFlag = false;
		Result = SmsLiteSendCtrlMsg( (SmsMsgData_ST*)&SmsMsg );
		
		// Wait for device init response
		if ( !SmsHostWaitForFlagSet( &mSyncFlag, 200 ) )
		{
			Error << "failed to init sms device";
			return;
		}
	}
	
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

TSianoLib::~TSianoLib()
{
	SmsLiteMsLibTerminate();
}


// Data callback function. This function is being given to SMS11xx host library
//as a callback for data.
void TSianoLib::IsdbtUserDataCallback(UINT32 ServiceDevHandle,UINT8* pBuf,UINT32 BufSize)
{
	//UINT32 Now;
	int BufferSize = BufSize;
	auto BufferArray = GetRemoteArray( pBuf, BufferSize, BufferSize );
	
	Array<uint8> Data;
	Data.PushBackArray( BufferArray );
	mDataCallbacks.PushBack( Data );
	
//	g_IsdbtState.Stats.NumBytes += BufSize;
//	g_IsdbtState.Stats.NumCallbacks++;
}

void TSianoLib::IsdbtUserCtrlCallback( SMSHOSTLIB_MSG_TYPE_RES_E	MsgType,
									SMSHOSTLIB_ERR_CODES_E		ErrCode,
									void* 						pPayload,
									UINT32						PayloadLen)

{
	//Handle indications.
	switch(MsgType)
	{
		case SMSHOSTLIB_MSG_GET_VERSION_RES:
		{
			std::string VersionString( static_cast<const char*>(pPayload) );
			std::Debug << "TSTV:SIANO: [sms]Version:" << VersionString << std::endl;
		}
		return;
			
		case SMSHOSTLIB_MSG_SMS_SIGNAL_DETECTED_IND:
			g_bHaveSignalIndicator = true;
			g_IsdbtState.Signal_exist = true;
			//		OSW_EventSet(&g_IsdbtState.hTuneEvent);
			std::Debug << "TSTV:SIANO: SMSHOSTLIB_MSG_SMS_SIGNAL_DETECTED_IND !!!" << std::endl;
			return;
			
		case SMSHOSTLIB_MSG_SMS_NO_SIGNAL_IND:
			g_bHaveSignalIndicator = true;
			g_IsdbtState.Signal_exist = false;
			//		OSW_EventSet(&g_IsdbtState.hTuneEvent);
			std::Debug << "TSTV:SIANO: SMSHOSTLIB_MSG_SMS_NO_SIGNAL_IND !!!\n" << std::endl;
			return;

		case SMSHOSTLIB_MSG_GET_STATISTICS_EX_RES:
		{
			SMSHOSTLIB_STATISTICS_ISDBT_ST* pStat = (SMSHOSTLIB_STATISTICS_ISDBT_ST*)pPayload;
			
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
		}
			return ;
			
		case SMSHOSTLIB_MSG_GET_STATISTICS_RES:
		{
			SMSHOSTLIB_FAST_STATISTICS_ST* pStat = (SMSHOSTLIB_FAST_STATISTICS_ST*)pPayload;
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
		return ;
			
		default:
			break;
	}
	
	
	
	if(MsgType != g_IsdbtState.ExpectedResponse)
	{
		return;
	}
	
	//
	//Handle expected response arrival.
	//
	
	//Copy error code.
	g_IsdbtState.ErrCode = ErrCode;
	
	//Copy payload length.
	g_IsdbtState.PayloadLen = PayloadLen;
	
	//Copy payload.
	if(g_IsdbtState.PayloadLen)
	{
		//	if((g_IsdbtState.pPayload = OswLiteMemAlloc(g_IsdbtState.PayloadLen)) == NULL)
		//	{
		//		printf("Allocation failed\n");
		//		exit(1);
		//	}
		
		//	memcpy(g_IsdbtState.pPayload, pPayload, g_IsdbtState.PayloadLen);
	}
	
	//Set response event.
	//	OSW_EventSet(&g_IsdbtState.hResponseEvent);
	
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


//*******************************************************************************
// Control callback function. This function is being given to SMS11xx host library
//as a callback for control events.
//


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
	/*
	std::lock_guard<std::recursive_mutex> Lock(mContextLock);

	//	gr: use this for the thread block
	int TimeoutMs = 10;
	int TimeoutSecs = 0;
	int TimeoutMicroSecs = TimeoutMs*1000;
	timeval Timeout = {TimeoutSecs,TimeoutMicroSecs};
	auto Result = freenect_process_events_timeout( mContext, &Timeout );
	if ( Result < 0 )
	{
		std::Debug << "Freenect_events error: " << Result;
	}
*/
	return true;
}

std::shared_ptr<TVideoDevice> SianoContainer::AllocDevice(const std::string& Serial,std::stringstream& Error)
{
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



