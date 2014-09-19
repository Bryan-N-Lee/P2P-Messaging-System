#include "LP2PM_client.h"

using namespace std;

LP2PM_Client::LP2PM_Client(const char* username, int init, int max, int udp,
						   int tcp):
init_broadcast_timeout(init),
current_broadcast_timeout(init),
max_broadcast_timeout(max),
UDP_port(udp),TCP_port(tcp),
user_availability_status(1){
	struct in_addr thisAddr;
	if(getLocalHostInfo(hostname,&thisAddr) == 0)
		inet_ntop(AF_INET, &thisAddr, ip_addr, INET_ADDRSTRLEN);
	else
		cout << "[Error] LP2PM_Client::LP2PM_Client() - Error getting Host info\n";

	strcpy(this->username,username);
	if(DEBUG) cout << "LP2PM_Client::LP2PM_Client() - Client Constructed" << endl;
}

LP2PM_Client::LP2PM_Client(const char* username){
	LP2PM_Client(username,LP2PM_DEFAULT_INIT_TIMEOUT, LP2PM_DEFAULT_MAX_TIMEOUT,
				LP2PM_DEFAULT_UDP_PORT, LP2PM_DEFAULT_TCP_PORT);
}

LP2PM_Client::~LP2PM_Client(){
	if(DEBUG) cout << "LP2PM_Client::~LP2PM_Client()" << endl;
	close(UDP_socket); close(TCP_socket);
}

bool LP2PM_Client::checkSockaddr(sockaddr_in const &a, sockaddr_in const &b)
{ return a.sin_addr.s_addr == b.sin_addr.s_addr; }

int LP2PM_Client::getLocalHostInfo(char* name, struct in_addr* addr){
	char Buffer[256];
	struct hostent *LocalHostEntry;
	
	if(-1 == gethostname(Buffer, 255)) return -1;
	
	LocalHostEntry = gethostbyname(Buffer);
	if(!LocalHostEntry) return -1;
	
	strcpy(name, LocalHostEntry->h_name);
	LocalHostEntry = gethostbyname(name);
	if(!LocalHostEntry) return -1;
	
	bcopy((char *)LocalHostEntry->h_addr, (char *)addr,
		  LocalHostEntry->h_length);
	return 0;
}

bool LP2PM_Client::hostIsMe(char* user, char* host){
	return (strcmp(user,username) == 0 && strcmp(host,hostname) == 0);
}

int LP2PM_Client::establishUDPserver()
{	if(DEBUG) cout << "LP2PM_Client::establishUDP()" << endl;
	UDP_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(UDP_socket < 0){
		cout << "LP2PM_Client::establishUDPserver() - socket error\n";
		close(TCP_socket);
		return -1;
	}
	bzero(&udp_client_addr, sizeof(udp_client_addr));
	
	udp_client_addr.sin_family = AF_INET;
	udp_client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_client_addr.sin_port = htons(UDP_port);
	if(bind(UDP_socket,(struct sockaddr*) &udp_client_addr,
			sizeof(udp_client_addr)) < 0){
		cout << "LP2PM_Client::establishUDPserver() - bind error\n";
		close(UDP_socket);
		close(TCP_socket);
		return -1;
	}
	int yes = 1; // broadcast enabled (?)
	if(setsockopt(UDP_socket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(int)) < 0)
	{	cout << "LP2PM_Client::establishUDPserver() - set socket options error\n";
		close(UDP_socket);
		close(TCP_socket);
		return -1;
	}
	return 0;
}

void LP2PM_Client::enableUDPbroadcast()
{	UDP_server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); }

void LP2PM_Client::enableUDPdirect(const char* hostname)
{	struct hostent* hp = gethostbyname(hostname);
	bcopy((char*)hp->h_addr, (char*) &UDP_server_addr.sin_addr, hp->h_length);
}

int LP2PM_Client::establishTCPserver(){
	if(DEBUG) cout << "LP2PM_Client::establishTCP()" << endl;
	TCP_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(TCP_socket < 0){
		cout << "[Error] LP2PM_Client::establishTCP() -"
			<< " Error opening TCP Socket\n";
		close(UDP_socket);
		return -1;
	}
	bzero((char*)&TCP_addr, sizeof(TCP_addr));
	TCP_addr.sin_family = AF_INET;
	TCP_addr.sin_addr.s_addr = INADDR_ANY;
	TCP_addr.sin_port = htons(tcp_port);
	if(0 > bind(TCP_socket, (struct sockaddr*)&TCP_addr, sizeof(TCP_addr))){
		cout << "[Error] LP2PM_Client::establishTCP() -"
			<< " Error on TCP binding\n";
		close(UDP_socket);
		close(TCP_socket);
		return -1;
	}
	return 0;
}

int P2PIM_Client::listenForAction(){
	if(DEBUG) cout << "P2PIM_Client::listenForAction()" << endl;
	
	int result, rv;
	struct pollfd ufds[3+P2PIM_DEFAULT_MAX_USERS];
	
	ufds[0].fd = UDP_socket;
	ufds[0].events = POLLIN;
	
	ufds[1].fd = tcp_socket;
	ufds[1].events = POLLIN;
	
	ufds[2].fd = STDIN_FILENO; // Standard Input
	ufds[2].events = POLLIN;
	P2PIM_User* u;
	int modified_timeout = current_broadcast_timeout;
	time_t before, after;
	time(&before); time(&after);
	bool auth_timeout_used = false;
	int modified_auth_timeout = current_auth_timeout;
	int poll_timeout = min(modified_timeout,modified_auth_timeout);
	// Range of 10 to 64 seconds
	int rand_interval = rand() % 40 + 10 + after%15;
	int random_time =  (int)after + rand_interval;
	while(1){
		for(int i = 0; i < users.getSize(); ++i){
			u = users.getUser(i);
			ufds[i+3].fd = u->TCP_socket;
			ufds[i+3].events = POLLIN;
		}
		time(&before);
		if(DEBUG)
			cout << "P2PIM_Client::listenForAction() - Poll Timeout is " << poll_timeout << endl;
		switch(rv = poll(ufds,3+users.getSize(),poll_timeout)){
			case -1:{
				if(DEBUG) cout << "P2PIM_Client::listenForAction() "
					<<"- poll returned error (" << errno << ")\n";
				cout << "# users: " << users.getSize() << endl;
				return -1;
			}
			case 0:{
				if(DEBUG) cout << "P2PIM_Client::listenForAction() "
					<<"- Poll Timeout...\n";
				if(auth_timeout_used){
					current_auth_timeout *= 2;
					if(DEBUG){
						cout << "P2PIM_Client::listenForAction() - "
						<< "Doubling Authentication Timeout to "
						<< current_auth_timeout << endl;
					}
				}
				else{
					if(users.isEmpty()){
						if(DEBUG)
							cout << "P2PIM_Client::listenForAction() - "
							<< "Broadcasting Discovery Msg\n";
						broadcastDiscoveryMsg();
						current_broadcast_timeout *= 2;
						if(current_broadcast_timeout > max_broadcast_timeout)
							current_broadcast_timeout = max_broadcast_timeout;
					}
			}
			default:{
				if(ufds[0].revents & POLLIN){ // UDP Message Received
					if(DEBUG) cout << "P2PIM_Client::listenForAction() - UDP Event Received\n";
					udpMessageReceived();
				}
				else if(ufds[1].revents & POLLIN){ // TCP Message Received
					if(DEBUG) cout << "P2PIM_Client::listenForAction() - TCP Event Received\n";
					tcpMessageReceived();
				}
				else if(ufds[2].revents & POLLIN){ // Keyboard input received
					if(DEBUG) cout << "P2PIM_Client::listenForAction() - Keyboard Event Received" << endl;
					if(keyboardReceived()) return 0;
				}
				else{
					for(int i = 0; i < users.getSize(); ++i){
						if(ufds[i+3].revents & POLLIN){
							if(DEBUG){
								cout << "P2PIM_Client::listenForAction() -"
								<< " TCP Client Event Received\n";
							}
							u = users.getUser(i);
							if(DEBUG){
								cout << "P2PIM_Client::listenForAction() -"
								<< " Received TCP Msg from "
								<< u->getUsername() << endl;
							}
							u->incoming_packet.clearPacket();
							result = read(u->TCP_socket,
										  u->incoming_packet.getData(0),
										  P2PIM_DEFAULT_MAX_PACKET_SIZE-1);
							if(result < 0){
								cerr << "[Error] P2PIM_Client::listenForAction() "
								<<"- TCP read returned error" << endl;
								break;
							}
							tcpMsgHandler(&(u->incoming_packet),u);
							break;
						}
					}
				}
			}
		}
	}
	return 0;
}
	
/*
 *	Handles a UDP event
 *	Reads and stores the incoming UDP packet
 */
void LP2PM_Client::udpMessageReceived(){
	if(DEBUG) cout << "P2PIM_Client::udpMessageReceived()" << endl;
	int result = 0;
	struct sockaddr_in user_addr;
	socklen_t userLn = sizeof(user_addr);
	UDP_in_packet.clearPacket();
	result = recvfrom(UDP_socket, UDP_in_packet.getData(0),
					  P2PIM_DEFAULT_MAX_PACKET_SIZE, 0,
					  (struct sockaddr*)&user_addr, &userLn);
	if(result < 0){
		cout << "[Error] P2PIM_Client::udpMessageReceived() "
		<<"- UDP Recvfrom returned error" << endl;
		return;
	}
	UDP_in_packet.setSize(result);
	if(UDP_in_packet.isDisM() || UDP_in_packet.isCM()){
		if(hostIsMe(UDP_in_packet.getUsername(),UDP_in_packet.getHostname())){
			if(DEBUG){
				cout << "P2PIM_Client::udpMessageReceived() - "
				<< "Received own broadcast ("
				<< UDP_in_packet.getUsername() << "@"
				<< UDP_in_packet.getHostname() << ")\n";
			}
			return;
		}
	}
	if(DEBUG) {
		cout << "P2PIM_Client::udpMessageReceived() - UDP Received "
		<< result << " Bytes from "
		<< inet_ntoa(user_addr.sin_addr) << endl;
		cout << "P2PIM_Client::udpMessageReceived() - UDP Message Type: "
		<< UDP_in_packet.getType() << " - PID: "
		<< UDP_in_packet.getPID() << endl;
		cout << "P2PIM_Client::udpMessageReceived() - ";
		UDP_in_packet.printPacket();
		cout << endl;
	}
	if(CONSOLE_DEBUG){
		sprintf(display.NEW_CONSOLE_LINE,
				"UDP Received (%d Bytes from %s)",
				result,inet_ntoa(user_addr.sin_addr));
		cout << "MOVING: " << display.NEW_CONSOLE_LINE << endl;
		display.addNewConsoleLine();
	}
	udpMsgHandler(&UDP_in_packet);
}

/*
 *	Handles TCP events
 *	Reads and stores the incoming TCP packet
 */
void LP2PM_Client::tcpMessageReceived(){
	if(DEBUG){
		cout << "P2PIM_Client::tcpMessageReceived()" << endl;
		cout << "P2PIM_Client::tcpMessageReceived() - ";
		TCP_in_packet.printPacket();
		cout << endl;
	}
	int result = 0;
	socklen_t clientLn;
	clientLn = sizeof(TCP_addr);
	TCP_in_packet.clearPacket();
	P2PIM_User user;
	user.TCP_socket = accept(TCP_socket,(struct sockaddr*)&TCP_addr, &clientLn);
	if(user.TCP_socket < 0){
		cerr << "[Error] P2PIM_Client::listenForAction() "
		<<"- TCP Accept returned error" << endl;
		return;
	}
	
	result = read(user.TCP_socket,user.incoming_packet.getData(0),
				  P2PIM_DEFAULT_MAX_PACKET_SIZE);
	if(result < 0){
		cerr << "[Error] P2PIM_Client::listenForAction() "
		<<"- TCP read returned error" << endl;
		return;
	}
	user.incoming_packet.setSize(result);
	if(DEBUG){
		cout << "P2PIM_Client::listenForAction() - TCP Message Type: "
		<< user.incoming_packet.getType() << " - PID: "
		<< user.incoming_packet.getPID() << endl;
	}
	tcpMsgHandler(&TCP_in_packet,users.findUser(TCP_addr));
}

int LP2PM_Client::udpMsgHandler(LP2PM_Packet* packet){
	if(DEBUG) cout << "LP2PM_Client::udpMsgHandler()" << endl;
	switch(packet->getType()){
		case LP2PM_TYPE_DISCOVERY:
			if(DEBUG){
				cout << "LP2PM_Client::udpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::discoveryMsgHandler()...\n";
			}
			return discoveryMsgHandler(packet);
		case LP2PM_TYPE_REPLY:
			if(DEBUG){
				cout << "LP2PM_Client::udpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::replyMsgHandler()...\n";
			}
			return replyMsgHandler(packet);
		case LP2PM_TYPE_CLOSING:
			if(DEBUG){
				cout << "LP2PM_Client::udpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::closingMsgHandler()...\n";
			}
			return closingMsgHandler(packet);
		default:
			cerr << "[Error] LP2PM_Client::udpMsgHandler() - "
				<< "Unknown UDP Msg Type: " << packet->getType() << endl;
			return -1;
	}
	return -1;
}

int LP2PM_Client::discoveryMsgHandler(LP2PM_Packet* packet){
	if(DEBUG) cout << "LP2PM_Client::discoveryMsgHandler()" << endl;
	if(users.retrieve(packet->getUsername(),packet->getHostname())){
		if(DEBUG){
			cout << "LP2PM_Client::discoveryMsgHandler() - User("
					<< packet->getUsername() << ") is already added\n";
		}
		return 0;
	}
	LP2PM_User user;
	if(DEBUG) cout << "LP2PM_Client::discoveryMsgHandler() - Setting Up User\n";
	user.init(packet->getUsername(), packet->getHostname(),
			  packet->getUDPPort(), packet->getTCPPort());
	users.insert(user);
	display.addNewHost(user.getUsername(),user.getHostname());
	return 0;
}

int LP2PM_Client::replyMsgHandler(LP2PM_Packet* packet){
	if(DEBUG) cout << "LP2PM_Client::replyMsgHandler()" << endl;
	if(users.retrieve(packet->getUsername(),packet->getHostname())){
		if(DEBUG){
			cout << "LP2PM_Client::replyMsgHandler() - User("
					<< packet->getUsername() << ") is already added\n";
		}
		return 0;
	}
	LP2PM_User user;
	user.init(packet->getUsername(), packet->getHostname(),
			  packet->getUDPPort(), packet->getTCPPort());
	users.insert(user);
	if(DEBUG) cout << "LP2PM_Client::replyMsgHandler() - exiting" << endl;
	return 0;
}
	
int LP2PM_Client::closingMsgHandler(LP2PM_Packet* packet){
	if(DEBUG) cout << "LP2PM_Client::closingMsgHandler()" << endl;
	if(users.remove(packet->getUsername(),packet->getHostname()) < 0){
		cerr << "[Error] LP2PM_Client::closingMsgHandler() - User(" 
			<< packet->getUsername() << ") could not be found\n";
		return -1;
	}
	display.removeHost(packet->getUsername(),packet->getHostname());
	if(users.isEmpty()) current_broadcast_timeout = init_broadcast_timeout;
	return 0;
}

int LP2PM_Client::tcpMsgHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::tcpMsgHandler()" << endl;
	switch(packet->getType()){
		case LP2PM_TYPE_REQUEST:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::establishMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to establishMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return establishMsgHandler(packet->getUsername(),
									   packet->getHostname(),user);
		case LP2PM_TYPE_ACCEPT:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::acceptMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to acceptMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return acceptMsgHandler(user);
		case LP2PM_TYPE_DECLINE:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::userUnavailableMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to userUnavailableMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return userUnavailableMsgHandler(user);
		case LP2PM_TYPE_DATA:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::dataMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to dataMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return dataMsgHandler(packet,user);
		case LP2PM_TYPE_MESSAGE:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet "
				<< "to LP2PM_Client::messageMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to messageMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return messageMsgHandler(packet,user);
		case LP2PM_TYPE_DISCONTINUE:
			if(DEBUG){
				cout << "LP2PM_Client::tcpMsgHandler() - Routing to Packet " 
					<< "to LP2PM_Client::discontinueMsgHandler()...\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"LP2PM_Client::tcpMsgHandler() - Routing to Packet to discontinueMsgHandler()...");
				display.addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return discontinueMsgHandler(user);
		default:
			cerr << "[Error] LP2PM_Client::tcpMsgHandler() - "
				<< "Unknown TCP Msg Type: " << packet->getType() << endl;
			return -1;
	}
	return -1;
}

int LP2PM_Client::requestMsgHandler(	const char* username,
										const char* hostname,
										LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::requestMsgHandler()" << endl;
	// Ask User if they would like to request a connection

	/*
	 *	Get User Input on this
	 */
	if(user_availability_status < 0){
		return sendDeclineMsg(user);
	}
	users.retrieve(username,hostname)->userEstablished();
	return sendAcceptComMsg(user);
}

int LP2PM_Client::acceptMsgHandler(LP2PM_User* user){
	// find user based off of tcp_client_addr and user.tcp_connection
	if(DEBUG) cout << "LP2PM_Client::acceptMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::acceptMsgHandler() - No User found\n";
		return -1;
	}
	user->userEstablished();
	return 0;
}

int LP2PM_Client::declineMsgHandler(LP2PM_User* user){
	// find user based off of tcp_client_addr and user.tcp_connection
	// change status to unavailable
	if(DEBUG) cout << "LP2PM_Client::userUnavailableMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::userUnavailableMsgHandler() - No User found\n";
		return -1;
	}
	user->userUnavailable();
	close(user->TCP_socket);
	return 0;
}
	
int LP2PM_Client::dataMsgHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::dataMsgHandler()" << endl;
	if(DEBUG){
		cout << "LP2PM_Client::dataMsgHandler() - Sending \"" 
				<< packet->getMsg() << "\" to Display...\n";
	}
	user->addNewMessage(packet->getMsg());
	display.addNewMessage(user->getUsername(),user->getHostname(),
						  packet->getMsg());
	return 0;
}
	
int LP2PM_Client::messageMsgHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::messageMsgHandler()" << endl;
	if(DEBUG){
		cout << "LP2PM_Client::messageMsgHandler() - Sending \""
		<< packet->getMsg() << "\" to Display...\n";
	}
	user->addNewMessage(packet->getMsg());
	display.addNewMessage(user->getUsername(),user->getHostname(),
						  packet->getMsg());
	return 0;
}

int LP2PM_Client::discontinueMsgHandler(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::discontinueMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::discontinueMsgHandler() - No User found\n";
		return -1;
	}
	close(user->TCP_socket);
	return 0;
}

/*
 * Broadcast Discovery Messages
 */
int LP2PM_Client::broadcastDiscoveryMsg(){
	if(DEBUG) cout << "LP2PM_Client::broadcastDiscoveryMsg() - Username: "
					<< username << " | Hostname: " << hostname << endl;
	LP2PM_Packet packet;
	packet.createDiscoveryMsg(username, hostname, UDP_port, TCP_port);
	int r = sendUDPPacketBroad(&packet);
	if(DEBUG){
		cout << "LP2PM_Client::broadcastDiscoveryMsg() - Broadcasted " << r 
			<< " Bytes\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::broadcastDiscoveryMsg() - Broadcasted %d Bytes",
				r);
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	return r;
}

/*
 * Broadcast Closing Message
 */
int LP2PM_Client::broadcastClosingMsg(){
	if(DEBUG) cout << "LP2PM_Client::broadcastClosingMsg()" << endl;
	LP2PM_Packet packet;
	packet.createClosingMsg(username, hostname, UDP_port, TCP_port);
	int r = sendUDPPacketBroad(&packet);
	if(DEBUG){
		cout << "LP2PM_Client::broadcastClosingMsg() - Broadcasted " << r 
			<< " Bytes\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::broadcastClosingMsg() - Broadcasted %d Bytes",
				r);
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	return r;
}

/*
 * Send a Reply Message to host
 */
int LP2PM_Client::sendReplyMsg(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendReplyMsg()" << endl;
	if(user->createReplyMsg(username, hostname, UDP_port, TCP_port) < 0){
		cerr << "[Error] LP2PM_Client::sendReplyMsg() -"
				<< " createReplyMsg() returned error\n";
		return -1;
	}
	int r = sendUDPPacketUni(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendReplyMsg() - Sent " << r 
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendReplyMsg() - Sent %d Bytes",
				r);
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	return r;
}

/*
 * Send an Establish Communication Message to known host
 */
int LP2PM_Client::sendRequestComMsg(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendRequestComMsg()" << endl;
	if(user->createRequestCom(username) < 0){
		cerr << "[Error] LP2PM_Client::sendRequestComMsg() -"
				<< " createRequestComMsg() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendRequestComMsg() - Sent " << r
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendRequestComMsg() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendRequestComMsg() - sendTCPPacket returned -1 -"
					<< " closing user socket...";
		}
		user->closeTCPSocket();
	}
	return r;
}

/*
 * Send an Accept Communication Message to known host
 */
int LP2PM_Client::sendAcceptComMsg(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendAcceptComMsg()" << endl;
	if(user->createAcceptCom() < 0){
		cerr << "[Error] LP2PM_Client::sendAcceptComMsg() -"
				<< " createAcceptComMsg() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendAcceptComMsg() - Sent " << r 
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendAcceptComMsg() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendAcceptComMsg() - sendTCPPacket returned -1 -"
			<< " closing user socket...";
		}
		close(user->TCP_socket);
	}
	return r;
}

/*
 * Send a User Unavailable Message to known host
 */
int LP2PM_Client::sendDeclineMsg(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendDeclineMsg()" << endl;
	if(user->createDeclineMsg() < 0){
		cerr << "[Error] LP2PM_Client::sendDeclineMsg() -"
				<< " sendDeclineMsg() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendDeclineMsg() - Sent " << r
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendDeclineMsg() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendDeclineMsg() - sendTCPPacket returned -1 -"
			<< " closing user socket...";
		}
		close(user->TCP_socket);
	}
	return r;
}

/*
 * Send a Data Message to known host
 */
int LP2PM_Client::sendDataMsg(LP2PM_User* user, char* msg){
	if(DEBUG) cout << "LP2PM_Client::sendDataMsg()" << endl;
	if(user->createDataMsg(msg) < 0){
		cerr << "[Error] LP2PM_Client::sendDataMsg() -"
				<< " createDataMsg() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendDataMsg() - Sent " << r 
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendDataMsg() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendDataMsg() - sendTCPPacket returned -1 -"
			<< " closing user socket...";
		}
		close(user->TCP_socket);
	}
	return r;
}
	
/*
 * Send a Message to known host
 */
int LP2PM_Client::sendMessage(LP2PM_User* user, char* msg){
	if(DEBUG) cout << "LP2PM_Client::sendMessage()" << endl;
	if(user->createMessage(msg) < 0){
		cerr << "[Error] LP2PM_Client::sendMessage() -"
		<< " createMessage() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendMessage() - Sent " << r
		<< " Bytes to " << user->getUsername() << " ("
		<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendMessage() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendMessage() - sendTCPPacket returned -1 -"
			<< " closing user socket...";
		}
		close(user->TCP_socket);
	}
	return r;
}

/*
 * Send a Discontinue Communication Message to known host
 */
int LP2PM_Client::sendDiscontinueComMsg(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendDiscontinueComMsg()" << endl;
	if(user->createDiscontinueCom() < 0){
		cerr << "[Error] LP2PM_Client::sendDiscontinueComMsg() -"
				<< " createDiscontinueCom() returned error\n";
		return -1;
	}
	int r = sendTCPPacket(user);
	if(DEBUG){
		cout << "LP2PM_Client::sendDiscontinueComMsg() - Sent " << r 
			<< " Bytes to " << user->getUsername() << " (" 
			<< user->getHostname() << ")\n";
	}
	if(CONSOLE_DEBUG){
		sprintf(NEW_CONSOLE_LINE,
				"LP2PM_Client::sendDiscontinueComMsg() - Sent %d Bytes to %s@%s",
				r,user->getUsername(),user->getHostname());
		display.addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendDiscontinueComMsg() - sendTCPPacket returned -1 -"
			<< " closing user socket...";
		}
		close(user->TCP_socket);
	}
	return r;	
}

/*
 * Send User's outgoing packet
 *
 * @return 		Number of bytes sent
 */
int LP2PM_Client::sendTCPPacket(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::sendTCPPacket()" << endl;
	int result = write(user->TCP_socket, user->outgoing_packet.getData(0),
						user->outgoing_packet.getSize());
	if(result < 0)
		cerr << "[Error] LP2PM_Client::sendTCPPacket() - write returned error\n";
	return result;
}

int LP2PM_Client::sendUDPPacketUni(LP2PM_User* user){
	int result = sendto(UDP_socket, user->outgoing_packet.getData(0),
						user->outgoing_packet.getSize(), 0, 
						(struct sockaddr*)&(user->udp_connection),
						sizeof(user->udp_connection));
	if(result < 0)
		cerr << "[Error] P2P_Client::sendUDPPacketUni() - sendto returned error\n";
	return result;
}

int LP2PM_Client::sendUDPPacketUni(LP2PM_Packet* packet,char* hostname,
								   int port){
	struct sockaddr_in udp_uni_connect;
	struct hostent* server;
	server = gethostbyname(hostname);
	if(server == NULL){
		cerr << "[Error] LP2PM_User::sendUDPPacketUni() - "
		<< "gethostbyname returned error\n";
		return -1;
	}
	bzero((char*)&udp_uni_connect,sizeof(udp_uni_connect));
	udp_uni_connect.sin_family = AF_INET;
	bcopy((char*)server->h_addr,(char*)&udp_uni_connect.sin_addr.s_addr,
		  server->h_length);
	udp_uni_connect.sin_port = htons(port);
	
	int result = sendto(UDP_socket, packet->getData(0),
						packet->getSize(), 0,
						(struct sockaddr*)&(udp_uni_connect),
						sizeof(udp_uni_connect));
	if(result < 0)
		cerr << "[Error] P2P_Client::sendUDPPacketUni() - sendto returned error\n";
	return result;
}

int LP2PM_Client::sendUDPPacketBroad(LP2PM_Packet* packet){
	if(DEBUG) cout << "LP2PM_Client::sendUDPPacketBroad()" << endl;
	int result = sendto(UDP_socket, packet->getData(0),packet->getSize(),0,
						(struct sockaddr*)&udp_client_addr, 
						sizeof(udp_client_addr));
	if(result < 0)
		cerr << "[Error] P2P_Client::sendUDPPacketBroad() -"
				<< " sendto returned error\n";
	return result;
}

void  LP2PM_Client::connectToRequestedClient(char* hosts,
											 int ports){
	LP2PM_Packet packet;
	packet.createDiscoveryMsg(udp_port, tcp_port, hostname, username);
	sendUDPPacketUni(&packet,hosts,ports);
}

void LP2PM_Client::closeClient(){
	if(DEBUG) cout << "LP2PM_Client::closeClient()" << endl;
	users.clearList();
	close(UDP_socket);
	close(TCP_socket);
}

/*
void cleanup(int shut,int s,int howmany)
{
  int     retval;


   // Shutdown and close sock1 completely.
   
  if (shut){
      retval = shutdown(s,howmany);
      if (retval == -1)
          perror ("shutdown");
  }
  retval = close (s);
  if (retval)
      perror ("close");
} 

void sighandler(int sig){
    if (sig == SIGINT){
        cleanup(0, ls,1);
        exit(EXIT_SUCCESS);
    }
}
void declsighandler(){
#ifdef __WIN32
    signal(SIGINT, sighandler);
#else
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask,SIGINT);
    action.sa_flags = 0;
    action.sa_handler = sighandler;
    sigaction(SIGINT,&action,NULL);
#endif
}
*/
