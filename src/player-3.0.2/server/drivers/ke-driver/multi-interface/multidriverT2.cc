/*
 * A simple DIO of how to write a driver that will be built as a
 * shared object.
 */

// ONLY if you need something that was #define'd as a result of configure
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
 #include <config.h>
 */

#if !defined (WIN32)
#include <unistd.h>
#endif
#include <string.h>
#include <sys/wait.h>

#include <libplayercore/playercore.h>
//#include <libplayerinterface/player_interfaces.h>
#include "../driver-client/CYZXInter.h"

#include "background.h"
#include <stdio.h>
#include <fcntl.h>
////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class multidriver: public ThreadedDriver {
public:
	multidriver(ConfigFile* cf, int section);
	virtual int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr,
			void * data);
private:
	// Main function for device thread.
	virtual void Main();
	virtual int MainSetup();
	virtual void MainQuit();
	virtual void Publish(player_msghdr_t* hdr, void* src, bool copy = true);

	// My position interface
	player_devaddr_t m_dio_addr;
	player_devaddr_t m_aio_addr;
	player_devaddr_t m_position_addr;
	player_devaddr_t m_opaque_addr;
	player_devaddr_t m_laser_addr;
	player_devaddr_t m_blobfinder_addr;

	int dealPosMessages(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaqueMessages(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealLaserMessages(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealBlobFinderMessages(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);

	int dealOpaqueMFCAPCommand(myOpaqueSt *popa);
	int dealOpaqueMFCAPReq(myOpaqueSt *popa, myOpaqueSt *res);
	int dealOpaqueMFCAPComReq(myOpaqueSt *popa, myOpaqueSt *res);
	int dealOpaqueAIOComReq(myOpaqueSt *popa, myOpaqueSt *res);
	int dealOpaqueDIOComReq(myOpaqueSt *popa, myOpaqueSt *res);
	int dealOpaquePosition2dComReq(myOpaqueSt *popa, myOpaqueSt *res);
	int dealOpaqueBKComReq(comReqBk1 *popa, respBk1 *res);
	int dealOpaqueMFCAP(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaqueAIO(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaqueDIO(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaquePosition2d(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaqueBK(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	int dealOpaqueSR(QueuePointer & resp_queue, player_msghdr * hdr,
			void * data);
	void writeMessagehrToFile(player_msghdr * hdr, void * data);
	int dealOpaqueBKGETULTRASONICF(comReqBk1 *popa);

private:
	//Opaque relation
	int frontl;
	int frontr;
	int behindl;
	int behindr;

	//laser
	int scan_id=0;
	int AioPortMin;
	int AioPortMax;
	int DioPortMin;
	int DioPortMax;
	char SonarPort;

	//blobfinder

	//position2d
#ifdef WRITELOG
	//all messages count
	int mc;
	//don't know messages count
	int nmc;
	int aiomc;
	int naiomc;
	int diomc;
	int ndiomc;
	int posmc;
	int nposmc;
	int opaquemc;
	int nopaquemc;
	int opaquemfcapmc;
	int nopaquemfcapmc;
	int opaquediomc;
	int nopaquediomc;
	FILE *fl;
	time_t logtime;
#endif
};

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
multidriver_Init(ConfigFile* cf, int section) {
	// Create and return a new instance of this driver

	return ((Driver*) (new multidriver(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void multidriver_Register(DriverTable* table) {

	table->AddDriver("multidriver", multidriver_Init);

}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.

/*
 * if (cf->ReadDeviceAddr(&(this->m_position_addr), section,
 "provides", PLAYER_POSITION2D_CODE, -1, NULL) != 0)
 {
 this->SetError(-1);
 return;
 }
 if (this->AddInterface(this->m_position_addr))
 {
 this->SetError(-1);
 return;
 }

 // Create my laser interface
 if (cf->ReadDeviceAddr(&(this->m_laser_addr), section,
 "provides", PLAYER_LASER_CODE, -1, NULL) != 0)
 {
 this->SetError(-1);
 return;
 }
 if (this->AddInterface(this->m_laser_addr))
 {
 this->SetError(-1);
 return;
 }
 * */

multidriver::multidriver(ConfigFile* cf, int section) :
		ThreadedDriver(cf, section) {

#ifdef WRITELOG
	printf("multidriver initial begin\n");
#endif

	// Read an option from the configuration file
	/*if (cf->ReadDeviceAddr(&(this->m_dio_addr), section, "provides",
	 PLAYER_DIO_CODE, -1, NULL) != 0) {
	 this->SetError(-1);
	 return;
	 }
	 if (this->AddInterface(this->m_dio_addr)) {
	 this->SetError(-1);
	 return;
	 }

	 if (cf->ReadDeviceAddr(&(this->m_aio_addr), section, "provides",
	 PLAYER_AIO_CODE, -1, NULL) != 0) {
	 this->SetError(-1);
	 return;
	 }
	 if (this->AddInterface(this->m_aio_addr)) {
	 this->SetError(-1);
	 return;
	 }*/
	if (cf->ReadDeviceAddr(&(this->m_position_addr), section, "provides",
			PLAYER_POSITION2D_CODE, -1, NULL) != 0) {
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(this->m_position_addr)) {
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&(this->m_laser_addr), section, "provides",
			PLAYER_LASER_CODE, -1, NULL) != 0) {
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(this->m_laser_addr)) {
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&(this->m_blobfinder_addr), section, "provides",
			PLAYER_BLOBFINDER_CODE, -1, NULL) != 0) {
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(this->m_blobfinder_addr)) {
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&(this->m_opaque_addr), section, "provides",
			PLAYER_OPAQUE_CODE, -1, NULL) != 0) {
		this->SetError(-1);
		return;
	} else {
		if (this->AddInterface(this->m_opaque_addr)) {
			this->SetError(-1);
			return;
		}
	}

#ifdef WRITELOG
	this->mc = 0;
	this->nmc = 0;
	this->diomc = 0;
	this->ndiomc = 0;
	this->aiomc = 0;
	this->naiomc = 0;
	this->posmc = 0;
	this->nposmc = 0;
	this->opaquemc = 0;
	this->nopaquemc = 0;
	this->opaquemfcapmc = 0;
	this->nopaquemfcapmc = 0;
	this->opaquediomc = 0;
	this->nopaquediomc = 0;
#endif

#ifdef WRITELOG
	if ((this->fl = (fopen(LOGFILENAME, "w"))) == NULL) {
		printf("can't open log file:");
		printf(LOGFILENAME);
		printf("\n");
		return;
	}
#endif

	this->frontl = cf->ReadInt(section, "frontL", 8);
	this->frontr = cf->ReadInt(section, "frontR", 9);
	this->behindl = cf->ReadInt(section, "rearL", 10);
	this->behindr = cf->ReadInt(section, "rearR", 11);

#ifdef WRITELOG
	printf("background special init begin\n");
#endif

	/*MFCapOpen();
	MFInit();
	MFSetPortDirect(0x00000000);
	for (int i = 0; i < 8; i++) {
		MFADEnable(i);
	}
	MFDigiInit(100);
	DelayMS(100);
	MFADInit(100);
	MFSetServoMode(this->frontl, 1);
	MFSetServoMode(this->frontr, 1);
	MFSetServoMode(this->behindl, 1);
	MFSetServoMode(this->behindr, 1);
	DelayMS(100);
	MFUSonicEnable();
	DelayMS(100);*/

#ifdef WRITELOG
	printf("multidriver initial end\n");
#endif

	return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int multidriver::MainSetup() {

	// Here you do whatever is necessary to setup the device, like open and
	// configure a serial port.

#ifdef WRITELOG
	if ((fl = (fopen(LOGFILENAME, "w"))) == NULL) {
		printf("can't open log file:");
		printf(LOGFILENAME);
		printf("\n");
		return -1;
	}
#endif

	return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void multidriver::MainQuit() {

	// Here you would shut the device down by, for DIO, closing a
	// serial port.
#ifdef WRITELOG
	printf(
			"all messages count %d , aio message %d , dio message %d , position message %d , opaque message %d \n",
			this->mc, this->aiomc, this->diomc, this->posmc, this->opaquemc);
	printf(
			"not messages %d , not aio %d , not dio %d , not pos %d , not opaque %d \n",
			this->nmc, this->naiomc, this->ndiomc, this->nposmc,
			this->nopaquemc);
	fprintf(this->fl,
			"all messages count:%d\nerror messages count:%d\nopaque messages count:%d\n",
			this->mc, this->nmc, this->opaquemc);
	fflush(this->fl);
#endif

}


void multidriver::writeMessagehrToFile(player_msghdr * hdr, void * data) {
#ifdef WRITELOG
	char *buf;

#endif
}

/*int multidriver::dealAIOmessages(QueuePointer & resp_queue, player_msghdr * hdr,
 void * data) {
 if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_AIO_CMD_STATE,
 *(player_devaddr_t *) &(this->m_aio_addr))) {
 player_aio_data_t aiod;
 player_aio_cmd_t *aioc = (player_aio_cmd_t*) data;
 this->getAIOValue(aioc, &aiod);
 Publish(this->m_aio_addr, resp_queue, PLAYER_MSGTYPE_DATA,
 PLAYER_AIO_DATA_STATE, (void*) &aiod, sizeof(aiod), NULL);
 return 0;
 } else {
 #ifdef WRITELOG
 this->naiomc++;
 #endif
 return 0;
 }
 return -1;
 }
 */
int multidriver::dealOpaqueMFCAPCommand(myOpaqueSt *popa) {
#ifdef WRITELOG
	printf("coming opaque command : %d %d %d %d\n", popa->type, popa->subtype,
			popa->p1, popa->p2);
#endif
	switch (popa->subtype) {
	case 1: {
		MFCapSetH(popa->p1, popa->p2);
		break;
	}
	case 2: {
		MFCapSetS(popa->p1, popa->p2);
		break;
	}
	case 3: {
		MFCapSetI(popa->p1, popa->p2);
		break;
	}
	case 4: {
		MFCapSetMode(popa->p1);
		break;
	}
	default:
		break;
	}
	return 0;
}
int multidriver::dealOpaqueBKComReq(comReqBk1 *popa, respBk1 *res) {
	res->type = popa->type;
	res->subtype = popa->subtype;

	if (BKDELAYMSF == popa->subtype) {
		DelayMS(popa->p1);
	} else if (BKLCDDISF == popa->subtype) {

	} else if (BKUSONICENABLEF == popa->subtype) {
		MFUSonicEnable();
	} else if (BKGETULTRASONICF == popa->subtype) {

	} else if (BKCOMSENDF == popa->subtype) {

	} else if (BKMP3PLAYF == popa->subtype) {

	} else if (BKMP3STOPF == popa->subtype) {
		MFMp3Stop();
	} else
		return -1;
	return 0;
}
int multidriver::dealOpaqueBKGETULTRASONICF(comReqBk1 *popa) {
	player_opaque_data_t opaqueRes;
	respBk1 res;

#ifdef WRITELOG
	printf("req opaque get ultrasonic ");
	printf("%d %d %d\n", popa->type, popa->subtype, popa->p1);
#endif

	res.type = popa->type;
	res.subtype = popa->subtype;
	res.p1 = MFGetUltrasonic();

	opaqueRes.data_count = sizeof(res);
	uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
			+ opaqueRes.data_count;
	opaqueRes.data = new uint8_t[opaqueRes.data_count];
	memcpy(opaqueRes.data, &res, opaqueRes.data_count);

#ifdef WRITELOG
	printf("req opaque ultrasonic response data_count %d , size %d\n",
			opaqueRes.data_count, size);
	printf("outing ultrasonic message %d %d %d\n", res.type, res.subtype,
			res.p1);
#endif

	Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
			PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
	return 0;
}
int multidriver::dealOpaqueMFCAPComReq(myOpaqueSt *popa, myOpaqueSt *pr) {
	pr->type = popa->type;
	pr->subtype = popa->subtype;
	switch (popa->subtype) {
	case CAPSETHF: {
		MFCapSetH(popa->p1, popa->p2);
		break;
	}
	case CAPSETSF: {
		MFCapSetS(popa->p1, popa->p2);
		break;
	}
	case CAPSETIF: {
		MFCapSetI(popa->p1, popa->p2);
		break;
	}
	case CAPSETMODEF: {
		MFCapSetMode(popa->p1);
		break;
	}
	case CAPGETCENTERXF: {
		pr->p1 = MFCapGetCenterX();
		break;
	}
	case CAPGETCENTERYF: {
		pr->p1 = MFCapGetCenterY();
		break;
	}
	case CAPGETSUMF: {
		pr->p1 = MFCapGetSum();
		break;
	}
	default: {
		break;
	}
	}
	return 0;
}
int multidriver::dealOpaqueMFCAPReq(myOpaqueSt *popa, myOpaqueSt *pr) {
	pr->type = popa->type;
	pr->subtype = popa->subtype;
	switch (popa->subtype) {
	case 5: {
		pr->p1 = MFCapGetCenterX();
		break;
	}
	case 6: {
		pr->p1 = MFCapGetCenterY();
		break;
	}
	case 7: {
		pr->p1 = MFCapGetSum();
		break;
	}
	default: {
		break;
	}
	}
#ifdef WRITELOG
	printf("comming opaque req : %d %d %d %d \n", popa->type, popa->subtype,
			popa->p1, popa->p2);
	printf("outing opaque message : %d %d %d %d\n", pr->type, pr->subtype,
			pr->p1, pr->p2);
#endif
	return 0;
}

int multidriver::dealOpaqueMFCAP(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;
	myOpaqueSt *popa = (myOpaqueSt *) opaquedata->data;

	player_opaque_data_t opaqueRes;
	myOpaqueSt res;
#ifdef WRITELOG
	int t = sizeof(myOpaqueSt);
	printf("opaqueMFCap size %d,coming message  ", t);
	printf("%d %d %d %d\n", popa->type, popa->subtype, popa->p1, popa->p2);
#endif

	switch (popa->subtype) {
	case CAPGETCENTERXF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETCENTERXF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETCENTERXF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaqueMFCap response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing MFCap message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case CAPGETCENTERYF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETCENTERYF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETCENTERYF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaqueMFCap response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing MFCap message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case CAPGETSUMF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETSUMF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPGETSUMF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaqueMFCap response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing MFCap message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case CAPSETHF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETHF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETHF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETSF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETSF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETSF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETIF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETIF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETIF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETMODEF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETMODEF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETMODEF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETYELLOW: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETYELLOW:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETYELLOW done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETRED: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETRED:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETRED done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case CAPSETGREEN: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETGREEN:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueMFCAPComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "CAPSETGREEN done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	default: {
#ifdef WRITELOG
		this->nopaquemc++;
#endif
		return -1;
		break;
	}
	}

	return 0;
}
int multidriver::dealOpaqueBK(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
	pid_t pid;
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;
	myOpaqueSt0 *popa = (myOpaqueSt0 *) opaquedata->data;
	switch (popa->subtype) {
	case BKMFSETSERVOROTASPDF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOROTASPDF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt2 *popa = (myOpaqueSt2 *) opaquedata->data;
		MFSetServoRotaSpd(popa->p1, popa->p2);
		//MFServoAction();
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOROTASPDF done:%s",
				ctime(&(this->logtime)));
#endif
		break;
	}
	case BKMFSETSERVOMODEF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOMODEF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt2 *popa = (myOpaqueSt2 *) opaquedata->data;
		MFSetServoMode(popa->p1, popa->p2);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOMODEF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case BKMFSETSERVOPOSF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOPOSF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt3 *popa = (myOpaqueSt3 *) opaquedata->data;
		MFSetServoPos(popa->p1, popa->p2, popa->p3);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSETSERVOPOSF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}

	case BKMFGETSERVOPOSF: {
		break;
	}

	case BKGETULTRASONICF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKGETULTRASONICF:%s", ctime(&(this->logtime)));
#endif
		comReqBk1 *popa = (comReqBk1 *) opaquedata->data;
		if (-1 == dealOpaqueBKGETULTRASONICF(popa))
			return -1;
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKGETULTRASONICF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case BKMFSERVOACTIONF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSERVOACTIONF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt0 *popa = (myOpaqueSt0 *) opaquedata->data;
		MFServoAction();
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMFSERVOACTIONF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case BKMFSETSERVOROTASPDTIMEF: {
		break;
	}

	case BKMP3PLAYF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMP3PLAYF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueStMp3 *popa = (myOpaqueStMp3 *) opaquedata->data;

		if ((pid = fork()) < 0) {
			printf("fork error\n");
		} else if (pid == 0) {
			char mp3Title[MP3MAXLENGTH + 1];
			for (int i = 0; i < MP3MAXLENGTH + 1; i++)
				mp3Title[i] = '\0';
			for (int i = 0; i < MP3MAXLENGTH; i++)
				mp3Title[i] = popa->mp3Title[i];
			if (execl("/mnt/yaffs/test/MP3Play", mp3Title, (char *) 0) < 0)
				printf("exe error\n");
		}
		/*
		 if (waitpid(pid, NULL, 0) < 0) {
		 printf("wait error\n");
		 }*/
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMP3PLAYF done:%s", ctime(&(this->logtime)));
#endif
		return 0;
		break;
	}

	case BKCOMSENDF: {
		break;
	}
	case BKMP3STOPF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMP3STOPF:%s", ctime(&(this->logtime)));
#endif
		comReqBk1 *popa = (comReqBk1 *) opaquedata->data;

		//MFMp3Stop();

		if ((pid = fork()) < 0) {
			printf("fork error\n");
		} else if (pid == 0) {
			if (execl("/mnt/yaffs/test/MP3Stop", "MP3STOP", (char *) 0) < 0)
				printf("exe error\n");
		}
		/*
		 if (waitpid(pid, NULL, 0) < 0) {
		 printf("wait error\n");
		 }*/
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKMP3STOPF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}

	case BKDELAYMSF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKDELAYMSF:%s", ctime(&(this->logtime)));
#endif
		comReqBk1 *popa = (comReqBk1 *) opaquedata->data;
		DelayMS(popa->p1);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKDELAYMSF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case BKLCDDISF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKLCDDISF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueStLCD *popa = (myOpaqueStLCD*) opaquedata->data;

		if (pid = fork() < 0) {
			printf("fork error\n");
		} else if (pid == 0) {
			char lcdDisp[LCDMAXLENGTH];
			for (int i = 0; i < LCDMAXLENGTH; i++)
				lcdDisp[i] = '\x20';
			for (int i = 0; i < LCDMAXLENGTH; i++) {
				lcdDisp[i] = popa->disp[i];
			}
			if (execl("/mnt/yaffs/test/LCDDisp", lcdDisp, (char *) 0) < 0)
				printf("exe error\n");
		}
		/*
		 if (waitpid(pid, NULL, 0) < 0) {
		 printf("wait error\n");
		 }
		 */

#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKLCDDISF done:%s", ctime(&(this->logtime)));
#endif
		return 0;
		break;
	}
	case BKUSONICENABLEF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKUSONICENABLEF:%s", ctime(&(this->logtime)));
#endif
		comReqBk1 *popa = (comReqBk1 *) opaquedata->data;
		MFUSonicEnable();
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "BKUSONICENABLEF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	}

	return 0;
}
int multidriver::dealOpaqueSR(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
	return -1;
}
int multidriver::dealOpaqueAIO(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;
	myOpaqueSt *popa = (myOpaqueSt *) opaquedata->data;

	player_opaque_data_t opaqueRes;
	myOpaqueSt res;
#ifdef WRITELOG
	int t = sizeof(myOpaqueSt);
	printf("opaqueAIO size %d,coming message  ", t);
	printf("%d %d %d %d\n", popa->type, popa->subtype, popa->p1, popa->p2);
#endif

	switch (popa->subtype) {
	case AIOCOMTF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMTF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueAIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMTF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = new uint8_t[opaqueRes.data_count];
		memcpy(opaqueRes.data, &res, opaqueRes.data_count);

#ifdef WRITELOG
		printf("req opaqueAIO response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing AIO message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case AIOCOMINFRAREDF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMINFRAREDF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueAIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMINFRAREDF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = new uint8_t[opaqueRes.data_count];
		memcpy(opaqueRes.data, &res, opaqueRes.data_count);

#ifdef WRITELOG
		printf("req opaqueAIO response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing AIO message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case AIOCOMF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueAIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "AIOCOMF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = new uint8_t[opaqueRes.data_count];
		memcpy(opaqueRes.data, &res, opaqueRes.data_count);

#ifdef WRITELOG
		printf("req opaqueAIO response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing AIO message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	default: {
		return -1;
		break;
	}
	}
	return 0;
}
int multidriver::dealOpaquePosition2d(QueuePointer & resp_queue,
		player_msghdr * hdr, void * data) {
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;
	comReqPos2d *popa = (comReqPos2d *) opaquedata->data;

	player_opaque_data_t opaqueRes;
	respPos2d res;
#ifdef WRITELOG
	int t = sizeof(myOpaqueSt);
	printf("opaquePosition2d size %d,coming message  ", t);
	printf("%d %d %d %d\n", popa->type, popa->subtype, popa->p1, popa->p2);
#endif

	switch (popa->subtype) {
	case POS2DSET4TIRESF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSET4TIRESF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt4Tires *popa4tires = (myOpaqueSt4Tires *) opaquedata->data;
		int id1 = popa4tires->tires[0];
		int id2 = popa4tires->tires[1];
		int id3 = popa4tires->tires[2];
		int id4 = popa4tires->tires[3];
		int speed1 = popa4tires->speed1;
		int speed2 = popa4tires->speed2;
		int speed3 = popa4tires->speed3;
		int speed4 = popa4tires->speed4;
#ifdef WRITELOG
		printf("coming  pos2dset4Tires : %d %d %d %d, %d %d %d %d\n", id1, id2,
				id3, id4, speed1, speed2, speed3, speed4);
#endif
		if (id1 != 0)
			MFSetServoRotaSpd(id1, speed1);
		if (id2 != 0)
			MFSetServoRotaSpd(id2, speed2);
		if (id3 != 0)
			MFSetServoRotaSpd(id3, speed3);
		if (id4 != 0)
			MFSetServoRotaSpd(id4, speed4);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSET4TIRESF done:%s", ctime(&(this->logtime)));
#endif
		return 0;
		break;
	}
	case POS2DSET4TIRESPOSF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSET4TIRESPOSF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt4TiresPos *popa4TiresPos =
				(myOpaqueSt4TiresPos*) opaquedata->data;
		int id1 = popa4TiresPos->tires[0];
		int id2 = popa4TiresPos->tires[1];
		int id3 = popa4TiresPos->tires[2];
		int id4 = popa4TiresPos->tires[3];
		int speed = popa4TiresPos->speed;
		int pos = popa4TiresPos->pos;
#ifdef WRITELOG
		printf("coming pos2dset4TiresPos :pos %d,speed %d,%d %d %d %d\n", pos,
				speed, id1, id2, id3, id4);
#endif
		if (id1 != 0)
			MFSetServoPos(id1, pos, speed);
		if (id2 != 0)
			MFSetServoPos(id2, pos, speed);
		if (id3 != 0)
			MFSetServoPos(id3, pos, speed);
		if (id4 != 0)
			MFSetServoPos(id4, pos, speed);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSET4TIRESPOSF done:%s",
				ctime(&(this->logtime)));
#endif
		return 0;
		break;
	}
	case POS2DSTOP: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSTOP:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSTOP done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DFORWARDF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DFORWARDF:%s", ctime(&(this->logtime)));
#endif

		myOpaqueSt4 *popa = (myOpaqueSt4 *) opaquedata->data;

		MFSetServoRotaSpd(this->frontl, popa->p1);
		MFSetServoRotaSpd(this->frontr, popa->p2);
		MFSetServoRotaSpd(this->behindl, popa->p3);
		MFSetServoRotaSpd(this->behindr, popa->p4);
		MFServoAction();

#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DFORWARDF done %d,%d,%d,%d:%s", popa->p1,
				popa->p2, popa->p3, popa->p4, ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DBACKWARDF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DBACKWARDF:%s", ctime(&(this->logtime)));
#endif

		myOpaqueSt4 *popa = (myOpaqueSt4 *) opaquedata->data;

		MFSetServoRotaSpd(this->frontl, -(popa->p1));
		MFSetServoRotaSpd(this->frontr, -(popa->p2));
		MFSetServoRotaSpd(this->behindl, -(popa->p3));
		MFSetServoRotaSpd(this->behindr, -(popa->p4));
		MFServoAction();

#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DBACKWARDF done %d,%d,%d,%d:%s", popa->p1,
				popa->p2, popa->p3, popa->p4, ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DTURNLEFTF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DTURNLEFTF:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt4 *popa = (myOpaqueSt4 *) opaquedata->data;

		MFSetServoRotaSpd(this->frontl, -(popa->p1));
		MFSetServoRotaSpd(this->frontr, (popa->p2));
		MFSetServoRotaSpd(this->behindl, -(popa->p3));
		MFSetServoRotaSpd(this->behindr, (popa->p4));
		MFServoAction();
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DTURNLEFTF done %d,%d,%d,%d:%s", popa->p1,
				popa->p2, popa->p3, popa->p4, ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DTURNRIGHT: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DTURNRIGHT:%s", ctime(&(this->logtime)));
#endif
		myOpaqueSt4 *popa = (myOpaqueSt4 *) opaquedata->data;

		MFSetServoRotaSpd(this->frontl, (popa->p1));
		MFSetServoRotaSpd(this->frontr, -(popa->p2));
		MFSetServoRotaSpd(this->behindl, (popa->p3));
		MFSetServoRotaSpd(this->behindr, -(popa->p4));
		MFServoAction();
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DTURNRIGHT done %d,%d,%d,%d:%s", popa->p1,
				popa->p2, popa->p3, popa->p4, ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETFRONTL: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETFRONTL:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETFRONTL done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETFRONTR: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETFRONTR:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETFRONTR done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETREARL: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETREARL:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETREARL done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETREARR: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETREARR:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETREARR done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DGETFRONTL: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETFRONTL:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETFRONTL done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(res);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaquePosition2d response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing position2d message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case POS2DGETFRONTR: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETFRONTR:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETFRONTR done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(res);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaquePosition2d response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing position2d message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case POS2DGETREARL: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETREARL:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETREARL done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(res);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaquePosition2d response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing position2d message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case POS2DGETREARR: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETREARR:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETREARR done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(res);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaquePosition2d response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing position2d message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case POS2DSETTIRES: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETTIRES:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETTIRES done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DGETTIRES: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETTIRES:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DGETTIRES done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(res);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaquePosition2d response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing position2d message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case POS2DSERVOMODESERVOF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSERVOMODESERVOF:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSERVOMODESERVOF done:%s",
				ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSERVOMODEMOTOF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSERVOMODEMOTOF:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSERVOMODEMOTOF done:%s",
				ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETSERVOTURN: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETSERVOTURN:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETSERVOTURN done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case POS2DSETSERVOTURNA: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETSERVOTURNA:%s", ctime(&(this->logtime)));
#endif
		dealOpaquePosition2dComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "POS2DSETSERVOTURNA done:%s",
				ctime(&(this->logtime)));
#endif
		break;
	}
	default: {
		return -1;
		break;
	}
	}
	return 0;

}

int multidriver::dealOpaqueDIO(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;
	myOpaqueSt *popa = (myOpaqueSt *) opaquedata->data;

	player_opaque_data_t opaqueRes;
	myOpaqueSt res;
#ifdef WRITELOG
	int t = sizeof(myOpaqueSt);
	printf("opaqueDIO size %d,coming message  ", t);
	printf("%d %d %d %d\n", popa->type, popa->subtype, popa->p1, popa->p2);
#endif

	switch (popa->subtype) {
	case DIOSETPORTDIRECTF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIOSETPORTDIRECTF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueDIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIOSETPORTDIRECTF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	case DIOGETDIGIINPUTF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIOGETDIGIINPUTF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueDIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIOGETDIGIINPUTF done:%s", ctime(&(this->logtime)));
#endif
		opaqueRes.data_count = sizeof(myOpaqueSt);
		uint32_t size = sizeof(opaqueRes) - sizeof(opaqueRes.data)
				+ opaqueRes.data_count;
		opaqueRes.data = (uint8_t *) &res;
#ifdef WRITELOG
		printf("opaqueDIO response data_count %d , size %d\n",
				opaqueRes.data_count, size);
		printf("outing DIO message %d %d %d %d\n", res.type, res.subtype,
				res.p1, res.p2);
#endif
		Publish(this->m_opaque_addr, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_OPAQUE_REQ_DATA, &opaqueRes, size, NULL);
		break;
	}
	case DIODIGIOUTPUTF: {
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIODIGIOUTPUTF:%s", ctime(&(this->logtime)));
#endif
		dealOpaqueDIOComReq(popa, &res);
#ifdef WRITELOG
		time(&(this->logtime));
		fprintf(this->fl, "DIODIGIOUTPUTF done:%s", ctime(&(this->logtime)));
#endif
		break;
	}
	default: {
		return -1;
		break;
	}
	}
	return 0;
}
inline int deaperDealAIO(int t, int type) {
	if (type == whatKindAIOAIOINFRAREDR) {
		int dis = 1;
#ifdef WRITELOG
#endif
		if (t > 500) {
			return 1;
		} else if (t <= 500 && t > 470) {
			return 2;
		} else if (t <= 470 && t > 400) {
			return 5;
		} else if (t <= 400 && t > 350) {
			return 6;
		} else if (t <= 350 && t > 320) {
			return 7;
		} else if (t <= 320 && t > 230) {
			return 10;
		} else if (t <= 235 && t > 115) {
			return (20 - (t - 115) / 12);
		} else if (t <= 115 && t > 72) {
			return (30 - (t - 72) / 4);
		} else if (t <= 72 && t > 52) {
			return (40 - (t - 52) / 2);
		} else if (t <= 52 && t > 42) {
			return (92 - t);
		} else if (t <= 42 && t > 33) {
			return (60 - (10 * t - 330) / 9);
		} else if (t <= 33 && t > 29) {
			return (70 - (10 * t - 290) / 4);
		} else if (t <= 29 && t > 23) {
			return (80 - (10 * t - 230) / 6);
		} else
			return 80;
		return dis;
	} else {
		return t;
	}
}

inline int deaperDealAIOOld(int t, int type) {
	if (type == whatKindAIOAIOINFRAREDR) {
		float volt = 0.0;
		t *= 5;
		float ft = (float) t;
		volt = ft / 1023;
#ifdef WRITELOG
		printf("t %d , ft %f , volt %f\n", t, ft, volt);
#endif

		int tmp = 0;

		if (volt > 2.45)
			return 10;
		else if (volt > 1.4) {
			ft = 20 - 10 * ((volt - 1.4) / 1.05);
			return (int) ft;
		} else if (volt > 1.0) {
			ft = 30 - 10 * ((volt - 1.0) / 0.4);
			return (int) ft;
		} else if (volt > 0.75) {
			ft = 40 - 10 * ((volt - 0.75) / 0.25);
			return (int) ft;
		} else if (volt > 0.6) {
			ft = 50 - 10 * ((volt - 0.6) / 0.15);
			return (int) ft;
		} else if (volt > 0.53) {
			ft = 60 - 10 * ((volt - 0.53) / 0.07);
			return (int) ft;
		} else if (volt > 0.45) {
			ft = 70 - 10 * ((volt - 0.45) / 0.08);
			return (int) ft;
		} else if (volt > 0, 4) {
			ft = 80 - 10 * ((volt - 0.4) / 0.05);
			return (int) ft;
		} else
			return 80;
	} else {
		return t;
	}
}
int multidriver::dealOpaqueAIOComReq(myOpaqueSt *popa, myOpaqueSt *res) {
	res->type = popa->type;
	res->subtype = popa->subtype;
	res->p1 = MFGetAD(popa->p1);
	if (popa->p2 != whatKindAIOAIODEF)
		res->p1 = deaperDealAIO(res->p1, popa->p2);
	return 0;
}
int multidriver::dealOpaquePosition2dComReq(myOpaqueSt *popa, myOpaqueSt *res) {
	res->type = popa->type;
	res->subtype = popa->subtype;
	if (POS2DSERVOMODESERVOF == popa->subtype) {
		MFSetServoMode(this->frontl, 0);
		MFSetServoMode(this->frontr, 0);
		MFSetServoMode(this->behindl, 0);
		MFSetServoMode(this->behindr, 0);
	} else if (POS2DSERVOMODEMOTOF == popa->subtype) {
		MFSetServoMode(this->frontl, 1);
		MFSetServoMode(this->frontr, 1);
		MFSetServoMode(this->behindl, 1);
		MFSetServoMode(this->behindr, 1);
	} else if (POS2DFORWARDF == popa->subtype) {
		MFSetServoRotaSpd(this->frontl, (popa->p1) & 0x0000FFFF);
		MFSetServoRotaSpd(this->frontr, (popa->p1) >> 16);
		MFSetServoRotaSpd(this->behindl, (popa->p2) & 0x0000FFFF);
		MFSetServoRotaSpd(this->behindr, (popa->p2) >> 16);
		MFServoAction();
	} else if (POS2DBACKWARDF == popa->subtype) {
		MFSetServoRotaSpd(this->frontl, -((popa->p1) & 0x0000FFFF));
		MFSetServoRotaSpd(this->frontr, -((popa->p1) >> 16));
		MFSetServoRotaSpd(this->behindl, -((popa->p2) & 0x0000FFFF));
		MFSetServoRotaSpd(this->behindr, -((popa->p2) >> 16));
		MFServoAction();
	} else if (POS2DTURNLEFTF == popa->subtype) {
		MFSetServoRotaSpd(this->frontl, -((popa->p1) & 0x0000FFFF));
		MFSetServoRotaSpd(this->frontr, ((popa->p1) >> 16));
		MFSetServoRotaSpd(this->behindl, -((popa->p2) & 0x0000FFFF));
		MFSetServoRotaSpd(this->behindr, ((popa->p2) >> 16));
		MFServoAction();
	} else if (POS2DTURNRIGHT == popa->subtype) {
		MFSetServoRotaSpd(this->frontl, (popa->p1) & 0x0000FFFF);
		MFSetServoRotaSpd(this->frontr, -((popa->p1) >> 16));
		MFSetServoRotaSpd(this->behindl, (popa->p2) & 0x0000FFFF);
		MFSetServoRotaSpd(this->behindr, -((popa->p2) >> 16));
		MFServoAction();
	}
	/*else if (POS2DFORWARDF == popa->subtype) {
	 MFSetServoRotaSpd(this->frontl, popa->p1);
	 MFSetServoRotaSpd(this->frontr, -(popa->p1));
	 MFSetServoRotaSpd(this->behindl, popa->p1);
	 MFSetServoRotaSpd(this->behindr, -(popa->p1));
	 MFServoAction();
	 } else if (POS2DBACKWARDF == popa->subtype) {
	 MFSetServoRotaSpd(this->frontl, -(popa->p1));
	 MFSetServoRotaSpd(this->frontr, popa->p1);
	 MFSetServoRotaSpd(this->behindl, -(popa->p1));
	 MFSetServoRotaSpd(this->behindr, popa->p1);
	 MFServoAction();
	 } else if (POS2DTURNLEFTF == popa->subtype) {
	 MFSetServoRotaSpd(this->frontl, -(popa->p1));
	 MFSetServoRotaSpd(this->frontr, -(popa->p1));
	 MFSetServoRotaSpd(this->behindl, -(popa->p1));
	 MFSetServoRotaSpd(this->behindr, -(popa->p1));
	 MFServoAction();
	 } else if (POS2DTURNRIGHT == popa->subtype) {
	 MFSetServoRotaSpd(this->frontl, popa->p1);
	 MFSetServoRotaSpd(this->frontr, popa->p1);
	 MFSetServoRotaSpd(this->behindl, popa->p1);
	 MFSetServoRotaSpd(this->behindr, popa->p1);
	 MFServoAction();
	 } */else if (POS2DSTOP == popa->subtype) {
		MFSetServoRotaSpd(this->frontl, 0);
		MFSetServoRotaSpd(this->frontr, 0);
		MFSetServoRotaSpd(this->behindl, 0);
		MFSetServoRotaSpd(this->behindr, 0);
		MFServoAction();
	} else if (POS2DSETSERVOTURN == popa->subtype) {
		MFSetServoPos(this->frontl, popa->p1, popa->p2);
		MFSetServoPos(this->frontr, popa->p1, popa->p2);
		MFSetServoPos(this->behindl, popa->p1, popa->p2);
		MFSetServoPos(this->behindr, popa->p1, popa->p2);
		MFServoAction();
	} else if (POS2DSETFRONTL == popa->subtype) {
		this->frontl = popa->p1;
	} else if (POS2DSETFRONTR == popa->subtype) {
		this->frontr = popa->p1;
	} else if (POS2DSETREARL == popa->subtype) {
		this->behindl = popa->p1;
	} else if (POS2DSETREARR == popa->subtype) {
		this->behindr = popa->p1;
	} else if (POS2DGETFRONTL == popa->subtype) {
		res->p1 = this->frontl;
	} else if (POS2DGETFRONTR == popa->subtype) {
		res->p1 = this->frontr;
	} else if (POS2DGETREARL == popa->subtype) {
		res->p1 = this->behindl;
	} else if (POS2DGETREARR == popa->subtype) {
		res->p1 = this->behindr;
	} else if (POS2DSETTIRES == popa->subtype) {
		this->frontl = (popa->p1) & 0x0000FFFF;
		this->frontr = (popa->p1) >> 16;
		this->behindl = (popa->p2) & 0x0000FFFF;
		this->behindr = (popa->p2) >> 16;
	} else if (POS2DGETTIRES == popa->subtype) {
		unsigned int ttire = 0x00000000;
		ttire |= ((this->frontl & 0x0000FFFF)
				| ((this->frontr << 16) & 0xFFFF0000));
#ifdef WRITELOG
		printf("int size %d\n", sizeof(int));
		printf("test tire id %x , %x , %x\n", (this->frontl & 0x0000FFFF),
				(this->frontr << 16) & 0xFFFF0000, ttire);
#endif
		res->p1 = ttire;
		ttire = 0x00000000;
		ttire |= ((this->behindl & 0x0000FFFF)
				| ((this->behindr << 16) & 0xFFFF0000));
		res->p2 = ttire;
#ifdef WRITELOG
		printf("test tire id %x , %x , %x\n", (this->behindl & 0x0000FFFF),
				(this->behindr << 16) & 0xFFFF0000, ttire);
#endif
	} else {
		return -1;
	}
	return 0;
}
int multidriver::dealOpaqueDIOComReq(myOpaqueSt *popa, myOpaqueSt *pr) {
	pr->type = popa->type;
	pr->subtype = popa->subtype;
	if (DIOSETPORTDIRECTF == popa->subtype) {
		MFSetPortDirect(popa->p1);
	} else if (DIOGETDIGIINPUTF == popa->subtype) {
		pr->p1 = MFGetDigiInput(popa->p1);
	} else if (DIODIGIOUTPUTF == popa->subtype) {
		MFDigiOutput(popa->p1, popa->p2);
	} else {
		return -1;
	}
	return 0;
}

int multidriver::dealOpaqueMessages(QueuePointer & resp_queue,
		player_msghdr * hdr, void * data) {
	/*
	 * struct myOpaqueSt{
	 int type;
	 int subtype;
	 int p1;
	 int p2;
	 };
	 * */
	player_opaque_data_t *opaquedata = (player_opaque_data_t *) data;

	myOpaqueSt *popa = (myOpaqueSt *) opaquedata->data;
	if (popa->type == MYOPAQUESTTYPEMFCAP) {
		dealOpaqueMFCAP(resp_queue, hdr, data);
		return 0;
	} else if (popa->type == MYOPAQUESTTYPEAIO) {
		dealOpaqueAIO(resp_queue, hdr, data);
		return 0;
	} else if (popa->type == MYOPAQUESTTYPEDIO) {
		dealOpaqueDIO(resp_queue, hdr, data);
		return 0;
	} else if (popa->type == MYOPAQUESTTYPEPOS) {
		dealOpaquePosition2d(resp_queue, hdr, data);
		return 0;
	} else if (popa->type == MYOPAQUEBk) {
		dealOpaqueBK(resp_queue, hdr, data);
		return 0;
	} else if (popa->type == MYOPAQUESR) {
		dealOpaqueSR(resp_queue, hdr, data);
		return 0;
	} else {
#ifdef WRITELOG
		printf("coming opaque messages type wrong ! \n");
#endif
		return -1;
	}

}

/*int multidriver::dealDIOmessages(QueuePointer & resp_queue, player_msghdr * hdr,
 void * data) {
 if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_DIO_CMD_VALUES,
 *(player_devaddr_t *) &(this->m_dio_addr))) {

 player_dio_cmd_t *pdc = (player_dio_cmd_t *) data;
 player_dio_data_t pdd;
 this->getDIOValue(pdc, &pdd);

 Publish(this->m_dio_addr, resp_queue, PLAYER_MSGTYPE_DATA,
 PLAYER_DIO_DATA_VALUES, (void*) &pdd, sizeof(pdd), NULL);

 return 0;
 } else {
 #ifdef WRITELOG
 this->ndiomc++;
 #endif
 return 0;
 }
 return -1;
 }*/
int multidriver::dealLaserMessages(QueuePointer & resp_queue,
		player_msghdr * hdr, void * data) {
	/*
	 #define PLAYER_LASER_DATA_SCAN 1
	 #define PLAYER_LASER_DATA_SCANPOSE 2
	 #define PLAYER_LASER_DATA_SCANANGLE 3
	 Y	 #define PLAYER_LASER_REQ_GET_GEOM 1
	 #define PLAYER_LASER_REQ_SET_CONFIG 2
	 #define PLAYER_LASER_REQ_GET_CONFIG 3
	 #define PLAYER_LASER_REQ_POWER 4
	 Y#define PLAYER_LASER_REQ_GET_ID 5
	 #define PLAYER_LASER_REQ_SET_FILTER 6*/

	// ---[ Get IDentification information
	/*if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_ID,
			this->m_laser_addr)) {
		return (0);
	} else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_GET_GEOM, this->m_laser_addr)) {
		player_laser_geom_t geom;
		memset(&geom, 0, sizeof(geom));
		geom.pose.px = 0;
		geom.pose.py = 0;
		geom.pose.pyaw = 0;
		geom.size.sl = 0;
		geom.size.sw = 0;

		Publish(this->m_laser_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK,
				PLAYER_LASER_REQ_GET_GEOM, (void*) &geom, sizeof(geom), NULL);
		return (0);
	}
	// ---[ Set power
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_POWER, this->m_laser_addr)) {

		return (0);
	}
	// ---[ Set configuration
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_SET_CONFIG, device_addr)) {
		return (-1);
	}
	// ---[ Get configuration
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_GET_CONFIG, device_addr)) {
		return (0);
	}
	// ---[ Set filter settings
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_SET_FILTER, device_addr)) {
		return (-1);
	} else*/
		return (-1);
}
int multidriver::dealBlobFinderMessages(QueuePointer & resp_queue,
		player_msghdr * hdr, void * data) {
	/*
	 #define PLAYER_BLOBFINDER_DATA_BLOBS 1
	 #define PLAYER_BLOBFINDER_REQ_SET_COLOR 1
	 #define PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS 2
	 #define PLAYER_BLOBFINDER_REQ_GET_COLOR 3
	 */
	/*if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
			PLAYER_CAMERA_DATA_STATE, this->camera_id)) {
		this->Publish(this->blobfinder_provided_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_BLOBFINDER_DATA_BLOBS, reinterpret_cast<void *>(output),
				0, &(hdr->timestamp), false); // copy = false
	}*/
	return -1;
}
int multidriver::dealPosMessages(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {

	/*
	 #define PLAYER_POSITION2D_REQ_GET_GEOM 1
	 #define PLAYER_POSITION2D_REQ_MOTOR_POWER 2
	 #define PLAYER_POSITION2D_REQ_VELOCITY_MODE 3
	 #define PLAYER_POSITION2D_REQ_POSITION_MODE 4
	 #define PLAYER_POSITION2D_REQ_SET_ODOM 5
	 #define PLAYER_POSITION2D_REQ_RESET_ODOM 6
	 #define PLAYER_POSITION2D_REQ_SPEED_PID 7
	 #define PLAYER_POSITION2D_REQ_POSITION_PID 8
	 #define PLAYER_POSITION2D_REQ_SPEED_PROF 9
	 #define PLAYER_POSITION2D_DATA_STATE 1
	 #define PLAYER_POSITION2D_DATA_GEOM 2
	 #define PLAYER_POSITION2D_CMD_VEL 1
	 #define PLAYER_POSITION2D_CMD_POS 2
	 #define PLAYER_POSITION2D_CMD_CAR 3
	 #define PLAYER_POSITION2D_CMD_VEL_HEAD 4
	 */

// Handle new data from the laser
	/*
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->device_addr)) {
		Message* msg;
		this->Publish(resp_queue, rephdr, repdata);
		delete msg;
		return (0);
	}
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
			PLAYER_POSITION2D_CMD_POS, this->device_addr)) {
		return 0;
	}

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
			PLAYER_POSITION2D_CMD_VEL, this->device_addr)) {
		return 0;
	}
	*/
	return -1;
}

int multidriver::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,
		void * data) {
// Process messages here.  Send a response if necessary, using Publish().
// If you handle the message successfully, return 0.  Otherwise,
// return -1, and a NACK will be sent for you, if a response is required.

	if (hdr->addr.interf == this->m_opaque_addr.interf) {
		if (-1 == dealOpaqueMessages(resp_queue, hdr, data)) {
			return -1;
		}
		return 0;
	} else if (hdr->addr.interf == this->m_position_addr.interf) {
		if (-1 == dealPosMessages(resp_queue, hdr, data)) {
			return -1;
		}
		return 0;
	} else if (hdr->addr.interf == this->m_laser_addr.interf) {
		if (-1 == dealLaserMessages(resp_queue, hdr, data)) {
			return -1;
		}
		return 0;
	} else if (hdr->addr.interf == this->m_blobfinder_addr.interf) {
		if (-1 == dealBlobFinderMessages(resp_queue, hdr, data)) {
			return -1;
		}
		return 0;
	} else {
		return -1;
	}
	/*else if (hdr->addr.interf == this->m_dio_addr.interf) {

	 #ifdef WRITELOG
	 printf("--dio message--\n");
	 this->diomc++;
	 #endif
	 if (-1 == dealDIOmessages(resp_queue, hdr, data)) {
	 #ifdef WRITELOG
	 printf("--dio message can not deal--\n");
	 #endif
	 return -1;
	 }
	 return 0;
	 } else if (hdr->addr.interf == this->m_aio_addr.interf) {
	 #ifdef WRITELOG
	 printf("--aio message--\n");
	 this->aiomc++;
	 #endif
	 if (-1 == dealAIOmessages(resp_queue, hdr, data)) {
	 #ifdef WRITELOG
	 printf("--aio message can not deal--\n");
	 #endif
	 return -1;
	 }
	 return 0;
	 } */
}

void multidriver::Publish(player_msghdr_t* hdr, void* src, bool copy = true) {
	//publish laserData

	  ModelLaser* mod = (ModelLaser*)this->mod;
		ModelLaser::Sample* samples = mod->GetSamples(NULL);

	  // don't publish anything until we have some real data
	  if( samples == NULL )
	    return;

	  player_laser_data_t pdata;
	  memset( &pdata, 0, sizeof(pdata) );

	  pdata.min_angle = -cfg.fov/2.0;
	  pdata.max_angle = +cfg.fov/2.0;
	  pdata.resolution = cfg.fov / cfg.sample_count;
	  pdata.max_range = cfg.range_bounds.max;
	  pdata.ranges_count = pdata.intensity_count = 21;
	  pdata.id = this->scan_id++;

	  pdata.ranges = new float[pdata.ranges_count];
	  pdata.intensity = new uint8_t[pdata.ranges_count];

	  for( unsigned int i=0; i<cfg.sample_count; i++ )
	    {
	      //printf( "range %d %d\n", i, samples[i].range);
	      pdata.ranges[i] = samples[i].range;
	      pdata.intensity[i] = (uint8_t)samples[i].reflectance;
	    }

	  // Write laser data
	  this->driver->Publish(this->addr,
				PLAYER_MSGTYPE_DATA,
				PLAYER_LASER_DATA_SCAN,
				(void*)&pdata, sizeof(pdata), NULL);

	  delete [] pdata.ranges;
	  delete [] pdata.intensity;

}
////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void multidriver::Main() {
// The main loop; interact with the device here

	for (;;) {
		// test if we are supposed to cancel
		pthread_testcancel();

		// Process incoming messages.  multidriver::ProcessMessage() is
		// called on each message.
		ProcessMessages();
		//printf("ProcessMessages() excuted\n");

		// Interact with the device, and push out the resulting data, using
		Driver::Publish(hdr, src);

		// Sleep (you might, for DIO, block on a read() instead)
		usleep(100000);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

/* need the extern to avoid C++ name-mangling  */
extern "C" {
int player_driver_init(DriverTable* table) {

	multidriver_Register(table);

	return (0);
}
}
