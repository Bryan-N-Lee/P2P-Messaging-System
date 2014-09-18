#include "LP2PM_Packet.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include "LP2PM_Macros.h"

class LP2PM_User{
private:
	char username[MAX_USERNAME_LENGTH];
	char hostname[MAX_HOSTNAME_LENGTH];
	int user_connected;		// -1 = disconnected, 0 = unknown, 1 = connected
	int user_established;	// -1 = unavailable,  0 = unknown, 1 = established
	char msg_history[MSG_HISTORY_LENGTH][MAX_MSG_LENGTH];
	struct sockaddr_in tcp_connection;
	struct sockaddr_in udp_connection;
	uint16_t UDP_port;
	uint16_t TCP_port;
	
	int establishUDP(const char*, int, int);// user's hostname, port #, socket
	int establishTCP(const char*, int);		// user's hostname, port #

public:
	LP2PM_Packet incoming_packet;
	LP2PM_Packet outgoing_packet;
	int TCP_socket;

	LP2PM_User();
	~LP2PM_User();
	
/* ---- LP2PM_User Setup ---- */
	/*
	 *	initialize the User
	 *
	 *	@param1	const char*		username
	 *	@param2	const char*		hostname
	 *	@param3	int				UDP port #
	 *	@param4 int				TCP port #
	 *	@return	int				0 on success
	 */
	int			init(		const char*, const char*, int, int, int);
	
	/*
	 *	Set the username
	 *
	 *	@param	const char*		username
	 *	@return int				0 on success, -1 on fail
	 */
	int			setUsername(const char*);
	
	/*
	 *	Set the hostname
	 *
	 *	@param	const char*		hostname
	 *	@return	int				0 on success, -1 on fail
	 */
	int			setHostname(const char*);
	
	/*
	 *	Set the UDP Port
	 *
	 *	@param	uint16_t		UDP Port #
	 */
	void		setUDPPort(uint16_t);
	
	/*
	 *	Set the TCP Port
	 *
	 *	@param	uint16_t		TCP Port #
	 */
	void		setTCPPort(uint16_t);
	
/* ---- LP2PM_User Status ---- */
	/*
	 *	Changes the user's connectivity status to 1 (connected)
	 */
	void		userConnected();
	
	/*
	 *	Changes the user's connectivity status to -1 (disconnected)
	 */
	void		userDisconnected();
	
	/*
	 *	Changes the user's established status to 1 (established)
	 */
	void		userEstablished();
	
	/*
	 *	Changes the user's established status to -1 (unavailable)
	 */
	void		userUnavailable();
	
	/*
	 *	Returns the connectivity status of the user
	 */
	bool		isUserConnected();
	
	/*
	 *	Returns the establish status of the user
	 */
	bool		isUserEstablished();
	

/* ---- LP2PM_User Shutdown ---- */
	/*
	 *	Clears everything in the LP2PM_User, so call when user has terminated
	 *
	 *	@return	int				status, 0 on completion
	 */
	int		shutdown();
	
/* ---- LP2PM_User misc ---- */
	const char*	getUsername();
	const char*	getHostname();
	int			getUDPPort() const;
	int			getTCPPort() const;
	
	/*
	 *	Checks wheither the given address is the same as this user based on UDP
	 *
	 *	@param	const sockaddr_in&	user address
	 *	@return	bool				same or not
	 */
	bool		isSameUDPSockaddr(	const sockaddr_in& a);
	
	/*
	 *	Checks wheither the given address is the same as this user based on TCP
	 *
	 *	@param	const sockaddr_in&	user address
	 *	@return	bool				same or not
	 */
	bool		isSameTCPSockaddr(	const sockaddr_in& a);

	/*
	 *	Changes the outgoing packet into a Replay Message Packet
	 *
	 *	@param1	const char*			username
	 *	@param2	const char*			hostname
	 *	@param3	const char*			UDP Port #
	 *	@param4	const char*			TCP Port #
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createReplyMsg(		const char*, const char*, int, int);
	
	/*
	 *	Changes the outgoing packet into a Request Communications Message Packet
	 *
	 *	@param1	const char*			username
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createRequestComMsg(const char*);
	
	/*
	 *	Changes the outgoing packet into a Accept Communications Message Packet
	 *
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createAcceptComMsg();
	
	/*
	 *	Changes the outgoing packet into a Decline Communications Message Packet
	 *
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createDeclineComMsg();
	
	/*
	 *	Changes the outgoing packet into a Data Message Packet
	 *
	 *	@param	const char*			payload
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createDataMsg(		const char*);
	
	/*
	 *	Changes the outgoing packet into a Message Packet
	 *
	 *	@param	const char*			Message
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createMessageMsg(	const char* );
	
	/*
	 *	Changes the outgoing packet into a Discontinue Communications Packet
	 *
	 *	@return	int					0 on success, -1 on fail
	 */
	int			createDiscontinueComMsg();

	/*
	 *	Add a new message from this user to this user's message history
	 *
	 *	@param	const char*			message
	 */
	void		addNewMessage(		const char*);
	
	//LP2PM_User operator = ( const LP2PM_User & );
};