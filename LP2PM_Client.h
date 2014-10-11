#include "LP2PM_user.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <curses.h>

class LP2PM_Client{
private:
	UserList			users;
	LP2PM_Display*		display;
	char				hostname[MAX_HOSTNAME_LENGTH];
	char				username[MAX_USERNAME_LENGTH];
	char				ip_addr[INET_ADDRSTRLEN];
	LP2PM_Packet		UDP_in_packet, UDP_out_packet;
	LP2PM_Packet		TCP_in_packet, TCP_out_packet;
	struct sockaddr_in	TCP_addr;	// for
	struct sockaddr_in	UDP_server_addr;
	int					UDP_socket, TCP_socket;
	int					UDP_port,	TCP_port;
	int					init_broadcast_timeout;
	int					current_broadcast_timeout;
	int					max_broadcast_timeout;
	int					user_availability_status;

	/*
	 *	Compare the two addresses
	 */
	bool checkSockaddr( sockaddr_in const &a, sockaddr_in const &b );
	
	/*
	 *	Store hostname/ip addr into given buffer (param1) and address (param2)
	 *
	 *	@param1	char*			buffer
	 *	@param2	struct in_addr*	address info stored
	 *	@return	int				0 on success, -1 on fail
	 */
	int	getLocalHostInfo( char* name, struct in_addr* addr );
	
	/*
	 *	Check that a received broadcast was not sent by me
	 *
	 *	@param1	char*			username
	 *	@param2	char*			hostname
	 *	@return	bool			if given username and hostname are same as
	 *							current user's username and hostname
	 */
	bool hostIsMe( char* user, char* host );
	
	/*
	 *	Setup UDP settings to handle incoming UDP messages
	 *
	 *	@return	int				0 on success, -1 on fail
	 */
	int	establishUDPserver(); //int establishUDP();
	
	/*
	 *	Setup sockaddr_in to send via broadcast
	 */
	void enableUDPbroadcast();
	
	/*
	 *	Setup sockaddr_in to send directly to given host
	 *
	 *	@param	const char*			hostname
	 */
	void enableUDPdirect( const char* );

	/*
	 *	Setup TCP communication (as a server)
	 *
	 *	@return	int				0 on success, -1 on fail
	 */
	int	establishTCPserver(); //int establishTCP();
	
	/*
	 *	Change user availability status
	 */
	void changeStatus( int );

public:
	/*
	 *	@param1	const char*			username
	 *	@param2	int					initial broadcast timeout
	 *	@param3	int					max broadcast timeout
	 *	@param4	int					UDP port #
	 *	@param5	int					TCP port #
	 */
	LP2PM_Client( const char*, int, int, int, int);
	LP2PM_Client( const char* );
	~LP2PM_Client();
	
	/*
	 *	Initialize Client by establishing UDP and TCP protocols
	 */
	int init();
	
	/*
	 *	Start client
	 */
	int start();
	
	/*
	 * 1.  On Start send Broadcast Msg
	 * 2.  Wait for response till 1st timeout time
	 * 3a. If Reply received, store host and goto 4
	 * 3b. If Timeout elapses, double timeout and goto 1
	 * 4.  New thread for listening for Discovery broadcasts from other hosts
	 */
	
	int listenForAction();

/* ---- LP2PM_Client Protocol Specific Message Handlers ---- */
	/*
	 *	Handles a UDP event
	 *	Reads and stores the incoming UDP packet
	 */
	void UDPMessageReceived();
	
	/*
	 *	Handles TCP events
	 *	Reads and stores the incoming TCP packet
	 */
	void TCPMessageReceived();
	
	/*
	 *	Handles a Keyboard event
	 *	If:
	 *		F1:			quit
	 *		[return]:	get info from LP2PM_Display
	 *		other:		send to LP2PM_Display
	 */
	int keyboardReceived();
	
	int  UDPHandler(LP2PM_Packet* );
	int  TCPHandler(LP2PM_Packet*, LP2PM_User* );

/* ---- LP2PM_Client UDP Message Handlers ---- */
	/*
	 *	Received a Discovery Message
	 *		1) collect user information from packet
	 *		2) I am acting as server now
	 *			2a) create [server] socket
	 *			2b) poll using socket
	 *			2c) accpet and save new socket to user
	 *		3) send Reply Message
	 */
	int discoveryMsgHandler(	LP2PM_Packet* );
	
	/*
	 *	Received a Reply Message
	 *		1) collect user information from packet
	 *		2) I am acting as client now
	 *			2a) create [client] socket
	 *			2b) connect
	 */
	int replyMsgHandler(		LP2PM_Packet* );
	int closingMsgHandler(		LP2PM_Packet* );

/* ---- LP2PM_Client TCP Message Handlers ---- */
	int requestMsgHandler(		const char*, const char*, LP2PM_User* );
	int acceptMsgHandler(		LP2PM_User* );
	int declineMsgHandler(		LP2PM_User* );
	int dataMsgHandler(			LP2PM_Packet*, LP2PM_User* );
	int messageMsgHandler(		LP2PM_Packet*, LP2PM_User* );
	int discontinueMsgHandler(	LP2PM_User* );
	
/* ---- LP2PM_Client Send UDP Messages ---- */
	int broadcastDiscoveryMsg();
	int broadcastClosingMsg();
	int sendReplyMsg( LP2PM_User* );

/* ---- LP2PM_Client Send TCP Messages ---- */
	int sendRequestComMsg(		LP2PM_User* );
	int sendAcceptComMsg(		LP2PM_User* );
	int sendDeclineMsg(			LP2PM_User* );
	int sendDataMsg(			LP2PM_User*, const char* );
	int sendMessage(			LP2PM_User*, const char* );
	int sendDiscontinueComMsg(	LP2PM_User* );

/* ---- LP2PM_Client Communication Systems ---- */
	int sendTCPPacket(LP2PM_User*);
	int sendUDPPacketUni(LP2PM_User*);
	int sendUDPPacketUni(LP2PM_Packet*,char*,int);
	int sendUDPPacketBroad(LP2PM_Packet*);

	//void updateUserStatus(LP2PM_User*);
	//UserList* getUserList();
	
	void connectToRequestedClient(char*, int);
	
	void closeClient();
};