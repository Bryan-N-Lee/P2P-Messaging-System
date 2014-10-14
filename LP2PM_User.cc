#include "LP2PM_User.h"

using namespace std;

LP2PM_User::LP2PM_User():
user_connected(0),user_established(0),tcp_socket(0),UDP_port(0),TCP_port(0)
{	bzero(username, MAX_USERNAME_LENGTH);
	bzero(hostname, MAX_HOSTNAME_LENGTH);
	bzero(IPv4, 16);
	bzero(IPv6, 40);
	for(int i = 0; i < MSG_HISTORY_LENGTH; ++i)
		bzero(msg_history[i], MAX_MSG_LENGTH);
	bzero((char*) &tcp_connection, sizeof(tcp_connection));
	bzero((char*) &udp_connection, sizeof(udp_connection));
	incoming_packet.clearPacket();
	outgoing_packet.clearPacket();
	if(DEBUG) cout << "LP2PM_User::LP2PM_User() - User Created" << endl;
}

LP2PM_User::~LP2PM_User(){ shutdown(); }

/* ---- LP2PM_User Setup ---- */
int LP2PM_User::init(const char* user_name, const char* host_name,
					 int udp_p, int tcp_p)
{	setUsername(user_name);
	setHostname(host_name);
	setUDPPort(udp_p);
	setTCPPort(tcp_p);
	//establishUDP(hostname, UDP_port, socket);
	// Just have everything in place to setup a TCP connection
	//		- if they want to initiate the connection or us
	//establishTCP(hostname, TCP_port);
	//TCP_port = tcp_p;
	return 0;
}

/*int LP2PM_User::establishUDP(const char* host_name, int port, int socket)
{	if(DEBUG) cout << "LP2PM_User::establishUDP()" << endl;
	struct hostent* hp = gethostbyname(host_name);
	
	bzero((char*)&udp_connection, sizeof(udp_connection));
	udp_connection.sin_family = AF_INET;
	if(!hp) return -1; // Error
	
	bcopy((char*)hp->h_addr, (char*)&udp_connection.sin_addr,hp->h_length);
	udp_connection.sin_port = htons(port);
	return 0;
}*/

int LP2PM_User::establishTCP()
{	if(DEBUG) cout << "LP2PM_User::establishTCP()" << endl;
	
	struct hostent* user_host = gethostbyname(hostname);
	if(!user_host){
		return -1;
		// get host by name failed
	}
	TCP_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(TCP_socket < 0){
		return -1;
		// socket failed
	}
	bzero((char*) &tcp_connection, sizeof(tcp_connection));
	tcp_connection.sin_family = AF_INET;
	bcopy((char*)user_host->h_addr,(char*)&tcp_connection.sin_addr.s_addr,
		  user_host->h_length);
	
	tcp_connection.sin_port = htons(port);
	int s = connect(TCP_socket, (struct sockaddr*) &tcp_connection,
					sizeof(tcp_connection));
	if(s < 0){
		return -1;
		// Connection failed
	}
	userConnected();
	return 0;
}

int LP2PM_User::setUsername(const char* user)
{	bzero(username, MAX_USERNAME_LENGTH);
	if(strlen(user) > MAX_USERNAME_LENGTH)
		strncpy(username,user,MAX_USERNAME_LENGTH);
	else strcpy(username,user);
	return strlen(username);
}

int LP2PM_User::setHostname(const char* host)
{	bzero(hostname, MAX_HOSTNAME_LENGTH);
	strncpy(hostname,host,std::max(strlen(host),MAX_HOSTNAME_LENGTH));
	return strlen(hostname);
}

void LP2PM_User::setUDPPort(uint16_t t){ TCP_port = t; }
void LP2PM_User::setTCPPort(uint16_t u){ UDP_port = u; }

/* ---- LP2PM_User Status ---- */
void LP2PM_User::userConnected()	{ user_connected	= 1;	}
void LP2PM_User::userDisconnected()	{ user_connected	= 0;	}
bool LP2PM_User::isUserConnected()	{ return user_connected;	}

bool LP2PM_User::checkIPv4(const char* ip)
{	return strcmp(IPv4,ip) == 0; }

bool Lp2PM_User::checkIPv6(const char* ip)
{	return strcmp(IPv6,ip) == 0; }

void LP2PM_User::setIPv4(const char* ip)
{	strncpy(IPv4,ip,std::max(strlen(ip),16)); }

void LP2PM_User::setIPv6(const char* ip)
{	strncpy(IPv6,ip,std::max(strlen(ip),40)); }

const char* LP2PM_User::getUsername(){ return username; }
const char* LP2PM_User::getHostname(){ return hostname; }

int LP2PM_User::getUDPPort() const { return UDP_port; }
int LP2PM_User::getTCPPort() const { return TCP_port; }

int LP2PM_User::shutdown()
{	//bzero((char*) &udp_connection, sizeof(udp_connection));
	bzero((char*) &tcp_connection, sizeof(tcp_connection));
	close(TCP_socket);
	TCP_socket = 0;
	bzero(username, MAX_USERNAME_LENGTH);
	bzero(hostname, MAX_HOSTNAME_LENGTH);
	bzero(IPv4, 16);
	bzero(IPv6, 40);
	user_connected = user_established = 0;
	UDP_port = TCP_port = 0;
	for(int i = 0; i < MSG_HISTORY_LENGTH; ++i)
		bzero(msg_history[i],MAX_MSG_LENGTH);
	incoming_packet.clearPacket();
	outgoing_packet.clearPacket();
	userDisconnected();
	return 0;
}

bool LP2PM_User::isSameUDPSockaddr(const sockaddr_in& a)
{ return a.sin_addr.s_addr == udp_connection.sin_addr.s_addr; }

bool LP2PM_User::isSameTCPSockaddr(const sockaddr_in& a)
{ return a.sin_addr.s_addr == tcp_connection.sin_addr.s_addr; }

int LP2PM_User::createReplyMsg(const char* user_name, const char* host_name,
							   int udp, int tcp)
{ return outgoing_packet.createReplyMsg(user_name, host_name, udp, tcp); }

int LP2PM_User::createRequestComMsg(const char* user_name)
{ return outgoing_packet.createRC(user_name); }

int LP2PM_User::createAcceptComMsg()
{ return outgoing_packet.createAC(); }

int LP2PM_User::createDeclineComMsg()
{ return outgoing_packet.createDC(); }

int LP2PM_User::createDataMsg(const char* msg)
{ return outgoing_packet.createDM(msg); }

int LP2PM_User::createMessageMsg(const char* msg)
{ return outgoing_packet.createMsg(msg); }

int LP2PM_User::createDiscontinueComMsg()
{ return outgoing_packet.createDisC(); }

void LP2PM_User::addNewMessage(const char* msg)
{	for(int i = 0; i < MSG_HISTORY_LENGTH - 1; ++i)
		strcpy(msg_history[i],msg_history[i+1]);
	sprintf(msg_history[MSG_HISTORY_LENGTH - 1],"%s",msg);
}
