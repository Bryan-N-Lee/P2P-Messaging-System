#include "LP2PM_Packet.h"

/*
 *	Constructor
 */
LP2PM_Packet::LP2PM_Packet():
packet_ID(total_packets++),packet_size(0){
	if(DEBUG){
		cout << "LP2PM_Packet::LP2PM_Packet() - Packet \"" << packet_ID
				<< "\" created..." << endl;
	}
	bzero(data, LP2PM_DEFAULT_MAX_PACKET_SIZE);
}

/*
 * Destructor
 */
LP2PM_Packet::~LP2PM_Packet(){}

/*
 *	Returns packet's current overall size
 *
 *	@return Packet overall size
 */
int LP2PM_Packet::getSize()
{ return packet_size; }

/*
 *	Clear the packet's contents
 *
 *	@action			Set Packet to 0
 */
void LP2PM_Packet::clearPacket(){
	if(DEBUG){
		cout << "LP2PM_Packet::clearPacket() - "
		<< "Clearing Packet Buffer of Packet ID: " << packet_ID << "\n";
	}
	memset(data, packet_size = 0, LP2PM_DEFAULT_MAX_PACKET_SIZE);
}

/*
 *	Prints Packet's Contents
 *
 *	@action			Prints the data member variable
 */
void LP2PM_Packet::printPacket(){
	switch(getType()){
		case LP2PM_TYPE_DISCOVERY:
		case LP2PM_TYPE_REPLY:
		case LP2PM_TYPE_CLOSING:{
			cout << "~~~~~~Packet Dump~~~~~~\n";
			printf(" %-12s %d\n","Packet ID:",packet_ID);
			printf(" %-12s %d\n","Packet Size:",packet_size);
			printf(" %-12s %d\n","Packet Type:",getType());
			printf(" %-12s %d\n","UDP Port:",getUDP());
			printf(" %-12s %d\n","TCP Port:",getTCP());
			printf(" %-12s %s\n","Hostname:",getHostname());
			printf(" %-12s %s\n","Username:",getUsername());
			cout << "~~~~~~~~~~~~~~~~~~~~~~~\n";
			return;
		}
		case LP2PM_TYPE_REQUEST:{
			cout << "~~~~~~Packet Dump~~~~~~\n";
			printf(" %-12s %d\n","Packet ID:",packet_ID);
			printf(" %-12s %d\n","Packet Size:",packet_size);
			printf(" %-12s %d\n","Packet Type:",getType());
			printf(" %-12s %s\n","Username:",getUsername());
			cout << "~~~~~~~~~~~~~~~~~~~~~~~\n";
			return;
		}
		case LP2PM_TYPE_DISCONTINUE:
		case LP2PM_TYPE_ACCEPT:
		case LP2PM_TYPE_DECLINE:{
			cout << "~~~~~~Packet Dump~~~~~~\n";
			printf(" %-12s %d\n","Packet ID:",packet_ID);
			printf(" %-12s %d\n","Packet Size:",packet_size);
			printf(" %-12s %d\n","Packet Type:",getType());
			cout << "~~~~~~~~~~~~~~~~~~~~~~~\n";
			return;
		}
		case LP2PM_TYPE_MESSAGE:
		case LP2PM_TYPE_DATA:{
			cout << "~~~~~~Packet Dump~~~~~~\n";
			printf(" %-12s %d\n","Packet ID:",packet_ID);
			printf(" %-12s %d\n","Packet Size:",packet_size);
			printf(" %-12s %d\n","Packet Type:",getType());
			printf(" %-12s %s\n","Packet Msg:",getMsg());
			cout << "~~~~~~~~~~~~~~~~~~~~~~~\n";
			return;
		}
		default:
			cerr << "[Error] LP2PM_Packet::printPacket - Packet ("
			<< packet_ID << ") has unknown Type: \"" << getType() << "\"\n";
	}
}

int LP2PM_Packet::getPID(){ return packet_ID; }

/*
 *	Add Signature to Packet
 */
int LP2PM_Packet::addSignature(const char* s){
	strcpy((char*)&data,s);
	packet_size += strlen(s);
	if(DEBUG){
		cout << "LP2PM_Packet::addSignature() "
		<< "- Signature added to packet \"" << packet_ID << "\"..." << endl;
	}
	return packet_size;
}

/*
 *	Add Type to Packet
 */
int LP2PM_Packet::addType(uint16_t type){
	addByte((BYTE)(type >> 8),LP2PM_TYPE_POS_L);
	addByte((BYTE)(0x00FF & type),LP2PM_TYPE_POS_R);
	packet_size += sizeof(type);
	if(DEBUG){
		cout << "LP2PM_Packet::addType() "
		<< "- Type \"" << type << "\" added to packet \""
		<< packet_ID << "\"..." << endl;
	}
	return packet_size;
}

/*
 *	Add UDP Port to end of packet
 *	Note: Changes packet size
 *
 *	@param	udpport		The UDP Port Number
 *
 *	@return				Where the function left off (packet size)
 */
int LP2PM_Packet::addUDP(uint16_t udpport){
	addByte((BYTE)(udpport >> 8),LP2PM_UDP_POS_L);
	addByte((BYTE)(0x00FF & udpport),LP2PM_UDP_POS_R);
	packet_size += sizeof(udpport);
	if(DEBUG){
		cout << "LP2PM_Packet::addUDP() "
		<< "- UDP Port \"" << udpport << "\" added to packet \""
		<< packet_ID << "\"..." << endl;
	}
	return packet_size;
}

/*
 *	Add TCP Port to end of packet
 *	Note: Changes packet size
 *
 *	@param	udpport		The TCP Port Number
 *
 *	@return				Where the function left off (packet size)
 */
int LP2PM_Packet::addTCP(uint16_t tcpport){
	addByte((BYTE)(tcpport >> 8),LP2PM_TCP_POS_L);
	addByte((BYTE)(0x00FF & tcpport),LP2PM_TCP_POS_R);
	packet_size += sizeof(tcpport);
	if(DEBUG){
		cout << "LP2PM_Packet::addTCP() "
		<< "- TCP Port \"" << tcpport << "\" added to packet \""
		<< packet_ID << "\"..." << endl;
	}
	return packet_size;
}

/*
 *	Add Hostname to end of packet
 *	Note: Changes packet size
 *
 *	@param	hostname	The Hostname
 *
 *	@return				The Number of bytes added
 */
int LP2PM_Packet::addHostname(const char* hostname){
	int n = 0;
	if(strlen(hostname) > MAX_HOSTNAME_LENGTH){
		char short_host[MAX_HOSTNAME_LENGTH];
		strncpy(short_host,hostname,MAX_HOSTNAME_LENGTH);
		n = addString(short_host,packet_size);
	}
	else n = addString(hostname,packet_size);
	assert(n >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::addHostname() "
		<< "- Hostname \"" << hostname << "\" added to packet \""
		<< packet_ID << "\"..." << endl;
	}
	return n;
}

/*
 *	Add Username to end of packet
 *	Note: Changes packet size
 *
 *	@param	username	The Username
 *
 *	@return				The Number of bytes added
 */
int LP2PM_Packet::addUsername(const char* username){
	int n = 0;
	if(strlen(username) > MAX_USERNAME_LENGTH){
		char short_user[MAX_USERNAME_LENGTH];
		strncpy(short_user,username,MAX_USERNAME_LENGTH);
		n = addString(short_user,packet_size);
	}
	else n = addString(username,packet_size);
	assert(n >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::addUsername() "
		<< "- Username \"" << username << "\" added to packet \""
		<< packet_ID << "\"..." << endl;
	}
	return n;
}

int LP2PM_Packet::addByte(BYTE b, int offset)
{ data[offset] = b; return b; }

/*
 *	Adds given cstring to data at given offset
 *
 *	@param	s		string to be added
 *	@param	offset	where to start added the string
 *
 *	@return			how many bytes were added
 *					-1 if s would cause an overflow
 */

int LP2PM_Packet::addString(const char* s, int offset){
	// addition of string will cause a packet overflow
	if(strlen(s) + 1 + offset > LP2PM_DEFAULT_MAX_PACKET_SIZE) return -1;
	strcpy((char*)&(data[offset]),s);
	packet_size += (strlen(s)+1); // have to take '\0' into account
	return strlen(s)+1;
}

/*
 *	Adds given message to packet
 *
 *	@param	msg		Message to be added
 *	
 *	@return			number of bytes added
 *					-1 if msg would've caused an overflow
 */
int LP2PM_Packet::addMsg(const char* msg){
	int n = addString(msg,LP2PM_MSG_POS);
	assert(n >= 0);
	if(n < 0) return -1;
	if(DEBUG){
	   cout << "LP2PM_Packet::addMsg() "
	   << "- Message \"" << msg << "\" added to packet \""
	   << packet_ID << "\"..." << endl;
	}
	return n;
}

BYTE LP2PM_Packet::getByte(int offset)
{ return (BYTE)data[offset]; }
		   
uint16_t LP2PM_Packet::getType()
{ return (getByte(LP2PM_TYPE_POS_L) << 8) | getByte(LP2PM_TYPE_POS_R); }

uint16_t LP2PM_Packet::getUDP()
{ return (getByte(LP2PM_UDP_POS_L) << 8) | getByte(LP2PM_UDP_POS_R); }

uint16_t LP2PM_Packet::getTCP()
{ return (getByte(LP2PM_TCP_POS_L) << 8) | getByte(LP2PM_TCP_POS_R); }

/*
 *	Get Hostname for Discovery, Reply, or Closing Messages
 *
 *	@return			Starting address of hostname
 */
char* LP2PM_Packet::getHostname()
{ return (char*)&(data[LP2PM_HOSTNAME_POS]); }

/*
 *	Get the Username for Discovery, Reply, or Closing Messages
 *
 *	@return			Starting address of Username
 */
char* LP2PM_Packet::getUsername(){
	if(isRequestCom()) return (char*)&(data[LP2PM_USER_REQ_POS]);
	// 1 for '\0'
	return (char*)&(data[strlen(getHostname())+1+LP2PM_HOSTNAME_POS]);
}

/*
 *	Get Message from a Data Message Packet
 *
 *	@return			Starting address of the message
 */
char* LP2PM_Packet::getMsg()
{ return (char*)&(data[LP2PM_MSG_POS]); }

/*
 *	Returns the number of bytes the message uses (includes the '\0')
 *
 *	@return			Number of bytes the message uses, not including '\0'
 *					(so like strlen())
 */
int LP2PM_Packet::getMsgSize()
{ return strlen(getMsg()); }

unsigned char* LP2PM_Packet::getData(int offset)
{ return (unsigned char*)&(data[offset]); }

// Discover Msg
int LP2PM_Packet::createDiscoveryMsg( const char* username, const char* hostname,
									  uint16_t udpport,	uint16_t tcpport)
{ return createDisM(username, hostname, udpport, tcpport); }

int LP2PM_Packet::createDisM(	const char* username, const char* hostname,
								uint16_t udpport,	uint16_t tcpport){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_DISCOVERY);
	addUDP(udpport);
	addTCP(tcpport);
	int ret = addHostname(hostname);
	assert(ret >= 0);
	ret = addUsername(username);
	assert(ret >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::createDiscoveryMsg() - Packet \"" << packet_ID
		<< "\" is now a Discovery Message Packet..." << endl;
	}
	return packet_size;
}
// Reply Msg
int LP2PM_Packet::createReplyMsg(	const char* username, const char* hostname,
									uint16_t udpport,	uint16_t tcpport)
{ return createRM(username, hostname, udpport, tcpport); }

int LP2PM_Packet::createRM(			const char* username, const char* hostname,
									uint16_t udpport,	uint16_t tcpport){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_REPLY);
	addUDP(udpport);
	addTCP(tcpport);
	int ret = addHostname(hostname);
	assert(ret >= 0);
	ret = addUsername(username);
	assert(ret >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::createReplyMsg() - Packet \"" << packet_ID
		<< "\" is now a Reply Message Packet..." << endl;
	}
	return packet_size;
}

// Closing Msg
int LP2PM_Packet::createClosingMsg(	const char* username, const char* hostname,
									uint16_t udpport,	uint16_t tcpport)
{ return createCM(username, hostname, udpport, tcpport); }

int LP2PM_Packet::createCM(			const char* username, const char* hostname,
									uint16_t udpport,	uint16_t tcpport){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_CLOSING);
	addUDP(udpport);
	addTCP(tcpport);
	int ret = addHostname(hostname);
	assert(ret >= 0);
	ret = addUsername(username);
	assert(ret >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::createClosingMsg() - Packet \"" << packet_ID
		<< "\" is now a Closing Message Packet..." << endl;
	}
	return packet_size;
}

// Request Communication Msg
int LP2PM_Packet::createRequestCom( const char* username )
{ return createRC(username); }

int LP2PM_Packet::createRC( const char* username ){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_REQUEST);
	int ret = addUsername(username);
	assert(ret >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::createRequestCom() - Packet \"" << packet_ID
		<< "\" is now an Request Communication Packet..." << endl;
	}
	return packet_size;
}

// Accept Communication Msg
int LP2PM_Packet::createAcceptCom()
{ return createAC(); }

int LP2PM_Packet::createAC(){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_ACCEPT);
	if(DEBUG){
		cout << "LP2PM_Packet::createAcceptComMsg() - Packet \"" << packet_ID
		<< "\" is now an Accept Communication Packet..." << endl;
	}
	return packet_size;
}

// Decline Communication Msg
int LP2PM_Packet::createDeclineCom()
{ return createDC(); }

int LP2PM_Packet::createDC(){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_DECLINE);
	if(DEBUG){
		cout << "LP2PM_Packet::createDeclineCom() - Packet \""
		<< packet_ID << "\" is now an Decline Communication Packet..."
		<< endl;
	}
	return packet_size;
}

// Message Msg
int LP2PM_Packet::createMessage(const char* msg)
{ return createMsg(msg); }

int LP2PM_Packet::createMsg(const char* msg){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_MESSAGE);
	int ret = addMsg(msg);
	assert(ret >= 0);
	if(DEBUG){
		cout << "LP2PM_Packet::createData() - Packet \""
		<< packet_ID << "\" is now a Message Packet..." << endl;
	}
	return packet_size;
}

// Discontinue Communication Msg
int LP2PM_Packet::createDiscontinueCom()
{ return createDisC(); }

int LP2PM_Packet::createDisC(){
	clearPacket();
	addSignature(LP2PM_SIGNATURE);
	addType(LP2PM_TYPE_DISCONTINUE);
	if(DEBUG){
		cout << "LP2PM_Packet::createDiscontinueCom() - Packet \""
		<< packet_ID
		<< "\" is now a Discontinue Communication Packet..." << endl;
	}
	return packet_size;
}

// Discovery Msg
bool LP2PM_Packet::isDiscovery(){ return isDisM(); }
bool LP2PM_Packet::isDisM(){ return getType() == LP2PM_TYPE_DISCOVERY; }

// Reply Msg
bool LP2PM_Packet::isReply(){ return isRM(); }
bool LP2PM_Packet::isRM(){ return getType() == LP2PM_TYPE_REPLY; }

// Closing Msg
bool LP2PM_Packet::isClosing(){ return isCM(); }
bool LP2PM_Packet::isCM(){ return getType() == LP2PM_TYPE_CLOSING; }

// Request Communication Msg
bool LP2PM_Packet::isRequestCom(){ return isRC(); }
bool LP2PM_Packet::isRC(){ return getType() == LP2PM_TYPE_REQUEST; }

// Accept Communication Msg
bool LP2PM_Packet::isAcceptCom(){ return isAC(); }
bool LP2PM_Packet::isAC(){ return getType() == LP2PM_TYPE_ACCEPT; }

// Decline Communnication Msg
bool LP2PM_Packet::isDeclineCom(){ return isDC(); }
bool LP2PM_Packet::isDC(){ return getType() == LP2PM_TYPE_DECLINE; }

// Data Msg
bool LP2PM_Packet::isMessage(){ return isMsg(); }
bool LP2PM_Packet::isMsg(){ return getType() == LP2PM_TYPE_MESSAGE; }

// Discontinue Communication Msg
bool LP2PM_Packet::isDiscontinueCom(){ return isDisC(); }
bool LP2PM_Packet::isDisC(){ return getType() == LP2PM_TYPE_DISCONTINUE; }
