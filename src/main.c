#include <stdio.h>
#include <string.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

const APP_MSG App_Msg={
		"Demo",
		"Demo-App",
		"V230526",
		"VANSTONE",
		__DATE__ " " __TIME__,
		"",
		0,
		0,
		0,
		"00001001140616"
};

void MenuThread();

void InitSys(void)
{
	int ret;
	unsigned char bp[32];

	// Turn on debug output to serial COM
	SetApiCoreLogLevel(1);//SetApiCoreLogLevel(5);

	// Load parameters
	initParam();

	//initializes device type
	initDeviceType();

#ifdef __WIFI__

	NetModuleOper_Api(GPRS, 0);
	ret = NetModuleOper_Api(WIFI, 1);
	MAINLOG_L1("NetModuleOper_Api wifi:%d",ret);

	ret = wifiOpen_lib();
	MAINLOG_L1("wifiOpen_lib:%d", ret);

	ApConnect();

#else

	// Turn off WIFI and turn on 4G network
	NetModuleOper_Api(WIFI, 0);
	NetModuleOper_Api(GPRS, 1);

#endif

	// Network initialization
	net_init();

	// MQTT client initialization
	initMqttOs();

	ret = fibo_thread_create(MenuThread, "mainMenu", 14*1024, NULL, 24);
	MAINLOG_L1("fibo_thread_create: %d", ret);

#ifdef __TMSTHREAD__
	ret = fibo_thread_create(TMSThread, "TMSThread", 14*1024, NULL, 24);
	MAINLOG_L1("fibo_thread_create: %d", ret);
#endif

	//check if any app to update
	ret = checkAppUpdate();
	if(ret<0)
		set_tms_download_flag(1);

#ifdef __SECCODEDISP__
	secscrOpen_lib();
	secscrCls_lib();
	secscrSetBackLightValue_lib(0);  //secscrSetBackLightValue_lib is necessary before secscrSetBackLightMode_lib
	secscrSetBackLightMode_lib(0, 300);
#endif

#ifdef __CTLSTRADE__
	Common_Init_Api();
	PayPass_Init_Api();
	PayWave_Init_Api();

	PayPassAddAppExp(0);
	PayWaveAddAppExp();
#endif

	memset(bp, 0, sizeof(bp));
	ret = sysReadBPVersion_lib(bp);
	MAINLOG_L1("Firmware Version == %d, version: %s", ret, bp);

	memset(bp, 0, sizeof(bp));
	ret = sysReadVerInfo_lib(4, bp);
	MAINLOG_L1("lib Version == %d, version: %s, APP Version: %s", ret, bp, App_Msg.Version);
}

int AppMain(int argc, char **argv)
{
	int ret;
	int signal_lost_count;
	int mobile_network_registered;

	SystemInit_Api(argc, argv);
	InitSys();

	signal_lost_count = 0;
	mobile_network_registered = 0;

	while (1) {

#ifdef __WIFI__

		 //int wifiGetStatus_lib(void);
		 ret = wifiGetLinkStatus_lib();
		 MAINLOG_L1("wifiGetLinkStatus_lib:%d", ret);
		 if(ret == 5) //AP not connected
		 {
			 if(ApConnect() != 0)
			 {
				 Delay_Api(5000);
				 continue;
			 }
		 }
		 else if(ret == -6300) // not open
		 {
			ret = wifiOpen_lib();
			MAINLOG_L1("wifiOpen_lib:%d", ret);
			if((ret != 0) || (ApConnect() != 0))
			{
				Delay_Api(5000);
				continue;
			}
		 }
		 else if(ret == -6302) // check status failed
		 {
			 AppPlayTip("Mobile network registration in progress");
			 Delay_Api(10000);
			 continue;
		 }
		 mQTTMainThread();
		 AppPlayTip("Server connection lost");

#else
		// Check mobile network status
		ret = NetLinkCheck_Api(GPRS);
		MAINLOG_L1("*******************4G status:%d",ret);

		if (ret == 2) {
			AppPlayTip("Please insert sim card and restart device");
			Delay_Api(60000);
			//SysPowerReBoot_Api();
			continue;
		}
		else if (ret == 1) {
			if (signal_lost_count > 10) {
				AppPlayTip("Cannot register mobile network, restarting device");
				Delay_Api(60000);
				//SysPowerReBoot_Api();
				continue;
			}

			AppPlayTip("Mobile network registration in progress");

			Delay_Api(3000);
			signal_lost_count++;
			mobile_network_registered = 0;
			continue;
		}
		else {
			signal_lost_count = 0;

			if (mobile_network_registered == 0) {
				mobile_network_registered = 1;
				//AppPlayTip("Mobile network registered");
			}
		}

		mQTTMainThread();
		AppPlayTip("Server connection lost");
#endif

	}

	return 0;
}





