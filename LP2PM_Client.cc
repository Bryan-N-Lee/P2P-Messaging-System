#include "LP2PM_client.h"

// TODO:
// [x]	Store the IP address of client when client first connects via UDP
// [x]	When client connects via TCP (establish/Request is received), match
//		client's IP and given Username to a UDP connected client with same IP
//		address
// [ ]	keyboardReceived()
// [ ]	listenforAction()

using namespace std;

LP2PM_Client::LP2PM_Client(const char* username, int init, int max, int udp,
						   int tcp):
init_broadcast_timeout(init),
current_broadcast_timeout(init),
max_broadcast_timeout(max),
UDP_port(udp),TCP_port(tcp),
user_availability_status(1)
{
	struct in_addr thisAddr;
	if(getLocalHostInfo(hostname,&thisAddr) == 0)
		inet_ntop(AF_INET, &thisAddr, ip_addr, INET_ADDRSTRLEN);
	else
		cout << "[Error] LP2PM_Client::LP2PM_Client() - Error getting Host info\n";

	strcpy(this->username,username);
	if(DEBUG) cout << "LP2PM_Client::LP2PM_Client() - Client Constructed" << endl;
	display = LP2PM_Display::Instance();
}

LP2PM_Client::LP2PM_Client(const char* username){
	LP2PM_Client(username,LP2PM_DEFAULT_INIT_TIMEOUT, LP2PM_DEFAULT_MAX_TIMEOUT,
				LP2PM_DEFAULT_UDP_PORT, LP2PM_DEFAULT_TCP_PORT);
}

LP2PM_Client::~LP2PM_Client(){
	if(DEBUG) cout << "LP2PM_Client::~LP2PM_Client()" << endl;
	close(UDP_socket); close(TCP_socket);
	LP2PM_Display::Destroy();
}

int LP2PM_Client::init(){
	display->setHostname(hostname);
	display->setUsername(username);
	establishUDPserver();
	establishTCPserver();
}

int LP2PM_Client::start(){
	display->init();
	listenForAction();
	// Return and shutdown client
	broadcastClosingMsg();
	closeClient();
	//endwin();
	LP2PM_Display::Destroy();
	display = NULL;
}

bool LP2PM_Client::checkSockaddr(sockaddr_in const &a, sockaddr_in const &b)
{	return a.sin_addr.s_addr == b.sin_addr.s_addr; }

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

int LP2PM_Client::establishUDPserver(){
	if(DEBUG) cout << "LP2PM_Client::establishUDP()" << endl;
	UDP_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(UDP_socket < 0){
		cout << "LP2PM_Client::establishUDPserver() - socket error\n";
		close(TCP_socket);
		return -1;
	}
	bzero(&UDP_server_addr, sizeof(UDP_server_addr));
	
	UDP_server_addr.sin_family = AF_INET;
	UDP_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	UDP_server_addr.sin_port = htons(UDP_port);
	if(bind(UDP_socket,(struct sockaddr*) &UDP_server_addr,
			sizeof(UDP_server_addr)) < 0){
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
	enableUDPbroadcast();
	return 0;
}

void LP2PM_Client::enableUDPbroadcast()
{	UDP_server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); }

void LP2PM_Client::enableUDPdirect(const char* hostname){
	struct hostent* hp = gethostbyname(hostname);
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
	listen(TCP_socket,5);
	return 0;
}

void LP2PM_Client::changeStatus(int new_status){
	user_availability_status = new_status;
	// ADD CONSOLE MESSAGE
}

int LP2PM_Client::listenForAction(){
	if(DEBUG) cout << "LP2PM_Client::listenForAction()" << endl;
	
	int result, rv;
	
	/* ---- Setup poll ---- */
	struct pollfd ufds[3+LP2PM_DEFAULT_MAX_USERS];
	
	ufds[0].fd = UDP_socket;
	ufds[0].events = POLLIN;
	
	ufds[1].fd = TCP_socket;
	ufds[1].events = POLLIN;
	
	ufds[2].fd = STDIN_FILENO; // Standard Input
	ufds[2].events = POLLIN;
	
	/* ---- Setup Timeouts ---- */
	int modified_timeout = current_broadcast_timeout;
	time_t before, after;
	time(&before); time(&after);

	/* ---- The Magic ---- */
	LP2PM_User* u;
	while(1){
		UserNode* un = users.root();
		for(int i = 0; un; un = un->next, ++i){
			ufds[i+3].fd = un->user.TCP_socket;
			ufds[i+3].events = POLLIN;
		}
		if(DEBUG){
			cout << "LP2PM_Client::listenForAction() - Poll Timeout is "
				<< modified_timeout << endl;
		}
		time(&before);
		switch(rv = poll(ufds,3+users.getSize(),modified_timeout)){
			case -1:{ /* ---- Error ---- */
				if(DEBUG) cout << "LP2PM_Client::listenForAction() "
					<<"- poll returned error (" << errno << ")\n";
				return -1;
			}
			case 0:{ /* ---- Poll Timeout ---- */
				if(DEBUG) cout << "LP2PM_Client::listenForAction() "
					<<"- Poll Timeout...\n";
				if(users.isEmpty()){
					if(DEBUG)
						cout << "LP2PM_Client::listenForAction() - "
						<< "Broadcasting Discovery Msg\n";
					broadcastDiscoveryMsg();
					current_broadcast_timeout *= 2;
					if(current_broadcast_timeout > max_broadcast_timeout)
						current_broadcast_timeout = max_broadcast_timeout;
				}
				break;
			}
			default:{ /* ---- Event Occurred ---- */
				if(ufds[0].revents & POLLIN){ // UDP Message Received
					if(DEBUG) cout << "LP2PM_Client::listenForAction() - "
									<< "UDP Event Received\n";
					UDPMessageReceived();
				}
				else if(ufds[1].revents & POLLIN){ // TCP Message Received
					if(DEBUG) cout << "LP2PM_Client::listenForAction() - "
									<< "TCP Event Received\n";
					TCPMessageReceived();
				}
				else if(ufds[2].revents & POLLIN){ // Keyboard input received
					if(DEBUG) cout << "LP2PM_Client::listenForAction() - "
									<< "Keyboard Event Received\n";
					if(keyboardReceived()) return 0;
				}
				else
				{	for(int i = 0; i < users.getSize(); ++i)
					{	if(ufds[i+3].revents & POLLIN)
						{	if(DEBUG){
								cout << "LP2PM_Client::listenForAction() -"
								<< " TCP Client Event Received\n";
							}
							u = users.getUser(i);
							if(DEBUG){
								cout << "LP2PM_Client::listenForAction() -"
								<< " Received TCP Msg from "
								<< u->getUsername() << endl;
							}
							u->incoming_packet.clearPacket();
							result = read(u->TCP_socket,
										  u->incoming_packet.getData(0),
										  LP2PM_DEFAULT_MAX_PACKET_SIZE);
							if(result < 0){
								cerr << "[Error] LP2PM_Client::listenForAction() "
								<<"- TCP read returned error" << endl;
								break;
							}
							TCPHandler(&(u->incoming_packet),u);
							break;
						}
					}
				}
			}
		}
		time(&after);
		modified_timeout = current_broadcast_timeout - (int)(after - before);
	}
	return 0;
} // TODO (DO LAST) after keyboardReceived finished
	
/*
 *	Handles a UDP event
 *	Reads and stores the incoming UDP packet
 */
void LP2PM_Client::UDPMessageReceived(){
	if(DEBUG) cout << "LP2PM_Client::UDPMessageReceived()" << endl;
	int result = 0;
	struct sockaddr_in user_addr;
	socklen_t userLn = sizeof(user_addr);
	UDP_in_packet.clearPacket();
	result = recvfrom(UDP_socket, UDP_in_packet.getData(0),
					  LP2PM_DEFAULT_MAX_PACKET_SIZE, 0,
					  (struct sockaddr*)&user_addr, &userLn);
	if(result < 0){
		cout << "[Error] LP2PM_Client::UDPMessageReceived() "
		<<"- UDP Recvfrom returned error" << endl;
		return;
	}
	UDP_in_packet.setSize(result);
	if(UDP_in_packet.isDisM() || UDP_in_packet.isCM()){
		if(hostIsMe(UDP_in_packet.getUsername(),UDP_in_packet.getHostname())){
			if(DEBUG){
				cout << "LP2PM_Client::UDPMessageReceived() - "
				<< "Received own broadcast ("
				<< UDP_in_packet.getUsername() << "@"
				<< UDP_in_packet.getHostname() << ")\n";
			}
			return;
		}
	}
	if(DEBUG) {
		cout << "LP2PM_Client::UDPMessageReceived() - UDP Received "
			<< result << " Bytes from "
			<< inet_ntoa(user_addr.sin_addr) << endl;
		cout << "LP2PM_Client::UDPMessageReceived() - UDP Message Type: "
			<< UDP_in_packet.getType() << " - PID: "
			<< UDP_in_packet.getPID() << endl;
		cout << "LP2PM_Client::UDPMessageReceived() - ";
		UDP_in_packet.printPacket();
		cout << endl;
	}
	if(CONSOLE_DEBUG){
		sprintf(display->NEW_CONSOLE_LINE, "UDP Received (%d Bytes from %s)",
				result,inet_ntoa(user_addr.sin_addr));
		cout << "MOVING: " << display->NEW_CONSOLE_LINE << endl;
		display->addNewConsoleLine();
	}
	UDPHandler(&UDP_in_packet, inet_ntoa(user_addr.sin_addr));
}

/*
 *	Handles TCP events
 *	Reads and stores the incoming TCP packet
 *
 *	1)	Receive connect() and accept()
 *	2)	wait for request communication message to connect tcp connection with user
 *		and udp
 *	3)	add accepted socket to found user
 */
void LP2PM_Client::TCPMessageReceived(){
	if(DEBUG) cout << "LP2PM_Client::TCPMessageReceived()" << endl;
	int result = 0;
	socklen_t clientLn;
	clientLn = sizeof(TCP_addr);
	TCP_in_packet.clearPacket();
	LP2PM_Packet packet;
	int new_tcp_socket;
	new_tcp_socket = accept(TCP_socket,(struct sockaddr*)&TCP_addr, &clientLn);

	if(temp_tcp_socket < 0){
		cerr << "[Error] LP2PM_Client::TCPMessageReceived() "
			<<"- TCP Accept returned error" << endl;
		return;
	}
	
	result = read(new_tcp_socket,packet.getData(0),LP2PM_DEFAULT_MAX_PACKET_SIZE);
	if(result < 0){
		cerr << "[Error] LP2PM_Client::TCPMessageReceived() "
			<<"- TCP read returned error" << endl;
		return;
	}
	packet.setSize(result);
	if(DEBUG){
		cout << "LP2PM_Client::TCPMessageReceived() - TCP Message Type: "
			<< packet.getType() << " - PID: "
			<< packet.getPID() << endl;
	}
	if(packet.getType() == LP2PM_TYPE_REQUEST){
		LP2PM_User* u = users.retrieve(packet.getUsername(),
									   inet_ntoa(TCP_addr.sin_addr));
		u->TCP_socket = new_tcp_socket;
	}
	else if(DEBUG){
		cout << "LP2PM_Client::TCPMessageReceived() - User at "
			<< inet_ntoa(TCP_addr.sin_addr) << " not found"
			<< ", TCP Socket dropped" << endl;
	}
	//TCPHandler(&TCP_in_packet,users.findUser(TCP_addr));
}

int LP2PM_Client::keyboardReceived(){
	if(DEBUG) cout << "LP2PM_Client::keyboardReceived()" << endl;
	char RXChar;
	if(read(STDIN_FILENO, &RXChar, 1) > 0){
		if(DEBUG){
			cout << "LP2PM_Client::keyboardReceived() -"
				<< " STDIN Event Received: \'" << RXChar << "\'\n";
		}
		switch(RXChar){
			case KEY_F(1): // F1
				return 1;
			case 0x0D:
			case 10:{ // return
				if(DEBUG){
					cout << "LP2PM_Client::keyboardReceived() -"
						<< " Return - Retrieving info\n";
				}
				char dest_user[MAX_USERNAME_LENGTH];
				char dest_host[MAX_HOSTNAME_LENGTH];
				char dest_msg[MAX_MSG_LENGTH];
				
				int toType = display->getToLine(dest_user,dest_host);
				display->getMsgLine(dest_msg,MAX_MSG_LENGTH);
				display->updateDisplay(RXChar);
				LP2PM_User* u = users.retrieve(dest_user,dest_host);
				
				switch(toType){
					case -1:
						if(DEBUG) cout << "LP2PM_Client::keyboardReceived() - Unknown Command\n";
						return 0;
					case TO_TYPE_AWAY:
					case TO_TYPE_HERE:
						changeStatus(toType);
						/*sprintf(display->NEW_CONSOLE_LINE,
								"User Status changed to Available");
						display->addNewConsoleLine();*/
						break;
					case TO_TYPE_MESSAGE:
						if(!u) {}// BAD USER INPUT
						if(!(u->isConnected())){
							// ADD CONSOLE MESSAGE
							// [ User is not connected, message not sent ]
							return 0;
						}
						sendMessage(u, dest_msg);
						display->addNewMessage(username, dest_user, dest_msg);
						break;
					case TO_TYPE_REQUEST:
						sendRequestComMsg(u);
						// ADD CONSOLE MESSAGE
						break;
					case TO_TYPE_ACCEPT:
						sendAcceptComMsg(u);
						// ADD CONSOLE MESSAGE
						break;
					case TO_TYPE_DECLINE:
						sendDeclineMsg(u);
						u->userDisconnected();
						// ADD CONSOLE MESSAGE
						break;
					case TO_TYPE_DISCONTINUE:
						sendDicontinueComMsg(u);
						u->userDisconnected();
						// ADD CONSOLE MESSAGE
						break;
					default:
						// IDK what happened
						return 0;
				}
				break;
			}
			default: // print character to screen
				display->updateDisplay(RXChar);
		}
	}
	return 0;
}


int LP2PM_Client::UDPHandler(LP2PM_Packet* packet, const char* ip){
	if(DEBUG) cout << "LP2PM_Client::UDPHandler()" << endl;
	switch(packet->getType()){
		case LP2PM_TYPE_DISCOVERY:
			if(DEBUG) cout << "LP2PM_Client::UDPHandler() - Discovery Packet\n";
			return discoveryMsgHandler(packet,ip);
			
		case LP2PM_TYPE_REPLY:
			if(DEBUG) cout << "LP2PM_Client::UDPHandler() - Reply Packet\n";
			return replyMsgHandler(packet,ip);
			
		case LP2PM_TYPE_CLOSING:
			if(DEBUG) cout << "LP2PM_Client::UDPHandler() - Closing Packet\n";
			return closingMsgHandler(packet,ip);
			
		default:
			cerr << "[Error] LP2PM_Client::UDPHandler() - "
				<< "Unknown UDP Msg Type: " << packet->getType() << endl;
			return -1;
	}
	return -1;
}

int LP2PM_Client::discoveryMsgHandler(LP2PM_Packet* packet, const char* ip){
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
	user.setIPv4(ip);
	sendReplyMsg(&user);
	users.insert(user);
	display->addNewHost(user.getUsername(),user.getHostname());
	return 0;
}

int LP2PM_Client::replyMsgHandler(LP2PM_Packet* packet, const char* ip){
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
	user.setIPv4(ip);
	user.establishTCP();
	sendRequestComMsg(&user);
	users.insert(user);
	if(DEBUG) cout << "LP2PM_Client::replyMsgHandler() - exiting" << endl;
	return 0;
}
	
int LP2PM_Client::closingMsgHandler(LP2PM_Packet* packet, const char* ip){
	if(DEBUG) cout << "LP2PM_Client::closingMsgHandler()" << endl;
	if(users.remove(packet->getUsername(),packet->getHostname()) < 0){
		cerr << "[Error] LP2PM_Client::closingMsgHandler() - User(" 
			<< packet->getUsername() << ") could not be found\n";
		return -1;
	}
	display->removeHost(packet->getUsername(),packet->getHostname());
	if(users.isEmpty()) current_broadcast_timeout = init_broadcast_timeout;
	return 0;
}

int LP2PM_Client::TCPHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::TCPHandler()" << endl;
	switch(packet->getType()){
		case LP2PM_TYPE_REQUEST:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Request Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"%s Requests to start Communication with you",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return requestMsgHandler(packet->getUsername(),
									   packet->getHostname(),user);
		case LP2PM_TYPE_ACCEPT:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Accept Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"%s has Accepted your Communications Request",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return acceptMsgHandler(user);
		case LP2PM_TYPE_DECLINE:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Decline Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"%s has Declined your Communications Request",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return userUnavailableMsgHandler(user);
		case LP2PM_TYPE_DATA:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Data Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"Data packet received from %s",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return dataMsgHandler(packet,user);
		case LP2PM_TYPE_MESSAGE:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Message Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"Message packet received from %s",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return messageMsgHandler(packet,user);
		case LP2PM_TYPE_DISCONTINUE:
			if(DEBUG){
				cout << "LP2PM_Client::TCPHandler() - Discontinue Packet\n";
			}
			if(CONSOLE_DEBUG){
				sprintf(NEW_CONSOLE_LINE,
						"%s has Discontinued communications with you",
						user->getUsername());
				display->addNewConsoleLine(NEW_CONSOLE_LINE);
			}
			return discontinueMsgHandler(user);
		default:
			cerr << "[Error] LP2PM_Client::TCPHandler() - "
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
	if(user_availability_status == STATUS_AWAY){
		user->userDisconnected();
		return sendDeclineMsg(user);
	}
	user->userConnected();
	return sendAcceptComMsg(user);
}

int LP2PM_Client::acceptMsgHandler(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::acceptMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::acceptMsgHandler() - No User found\n";
		return -1;
	}
	user->userConnected();
	return 0;
}

int LP2PM_Client::declineMsgHandler(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::userUnavailableMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::userUnavailableMsgHandler() - No User found\n";
		return -1;
	}
	//close(user->TCP_socket);
	user->userDisconnected();
	return 0;
}
	
int LP2PM_Client::dataMsgHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::dataMsgHandler()" << endl;
	if(DEBUG){
		cout << "LP2PM_Client::dataMsgHandler() - Sending \"" 
				<< packet->getMsg() << "\" to display->..\n";
	}
	user->addNewMessage(packet->getMsg());
	display->addNewMessage(user->getUsername(),user->getHostname(),
						  packet->getMsg());
	return 0;
}
	
int LP2PM_Client::messageMsgHandler(LP2PM_Packet* packet, LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::messageMsgHandler()" << endl;
	if(DEBUG){
		cout << "LP2PM_Client::messageMsgHandler() - Sending \""
		<< packet->getMsg() << "\" to display->..\n";
	}
	user->addNewMessage(packet->getMsg());
	display->addNewMessage(user->getUsername(),user->getHostname(),
						  packet->getMsg());
	return 0;
}

int LP2PM_Client::discontinueMsgHandler(LP2PM_User* user){
	if(DEBUG) cout << "LP2PM_Client::discontinueMsgHandler()" << endl;
	if(!user){
		cerr << "LP2PM_Client::discontinueMsgHandler() - No User found\n";
		return -1;
	}
	//close(user->TCP_socket);
	user->userDisconnected();
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
		sprintf(NEW_CONSOLE_LINE,"Broadcasted Discovery Message (%d Bytes)", r);
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE,"Broadcasted Closing Message (%d Bytes)", r);
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Reply Msg (%d Bytes) to %s@%s",
				r,user->username,user->hostname);
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Request Msg (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Accept Msg (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Decline Msg (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Data Msg (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Message (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
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
		sprintf(NEW_CONSOLE_LINE, "Sent Discontinue Msg (%d Bytes) to %s@%s",
				r,user->getUsername(),user->getHostname());
		display->addNewConsoleLine(NEW_CONSOLE_LINE);
	}
	if(r < 0){
		if(DEBUG){
			cout << "LP2PM_Client::sendDiscontinueComMsg() - sendTCPPacket "
			<< "returned -1 - closing user socket...";
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
	if(DEBUG) cout << "LP2PM_Client::sendUDPPacketUni()" << endl;
	return sendUDPPacketUni(user->outgoing_packet,user->hostname,user->UDP_port);
}

int LP2PM_Client::sendUDPPacketUni(LP2PM_Packet* packet,char* hostname,int port)
{	struct sockaddr_in udp_uni_connect;
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
	int result = sendto(UDP_socket, packet->getData(0), packet->getSize(), 0,
						(struct sockaddr*)&UDP_server_addr,
						sizeof(UDP_server_addr));
	if(result < 0)
		cerr << "[Error] P2P_Client::sendUDPPacketBroad() -"
				<< " sendto returned error\n";
	return result;
}

void LP2PM_Client::connectToRequestedClient(char* hosts,int ports){
	LP2PM_Packet packet;
	packet.createDiscoveryMsg(UDP_port, TCP_port, hostname, username);
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
