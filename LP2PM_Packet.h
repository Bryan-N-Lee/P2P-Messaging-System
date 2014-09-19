#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <ostream>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "LP2PM_Macros.h"
#include "LP2PM_Exception.h"

typedef uint8_t BYTE;
typedef uint16_t WORD;

using namespace std;

/* Packet Outlines

[--* UDP Introduction Packets *--]
 
[Discovery Packet]
 4 bytes     2 bytes    1 byte	    2 bytes      m bytes       n bytes
--------------------------------------------------------------------------
| Signature |  Type  |  UDP Port  |  TCP Port  |  Hostname  |  Username  |
--------------------------------------------------------------------------

[Reply Packet]
 4 bytes     2 bytes    1 byte	    2 bytes      m bytes       n bytes
--------------------------------------------------------------------------
| Signature |  Type  |  UDP Port  |  TCP Port  |  Hostname  |  Username  |
--------------------------------------------------------------------------

[Closing Packet]
 4 bytes     2 bytes    1 byte	    2 bytes      m bytes       n bytes
--------------------------------------------------------------------------
| Signature |  Type  |  UDP Port  |  TCP Port  |  Hostname  |  Username  |
--------------------------------------------------------------------------
 
 
[--* TCP Communication Messaging Packets *--]
 
[Request Communication Packet]
 4 bytes     2 bytes  n bytes
-----------------------------------
| Signature |  Type  |  Username  |
-----------------------------------
 
[Accept Communication Packet]
 4 bytes     2 bytes
----------------------
| Signature |  Type  |
----------------------
 
[Decline Communication Packet]
 4 bytes     2 bytes
----------------------
| Signature |  Type  |
----------------------
 
[Message Packet]
 4 bytes     2 bytes  n bytes
----------------------------------
| Signature |  Type  |  Message  |
----------------------------------
 
[Discontinue Communication Message]
 4 bytes     2 bytes
----------------------
| Signature |  Type  |
----------------------
*/

static ofstream log_file;
static int total_packets = 0;

class LP2PM_Packet{
private:
	int		packet_ID;
	int		packet_size;	// Overall size of the packet (signature + ...)
	unsigned char data[LP2PM_DEFAULT_MAX_PACKET_SIZE];	// Packet Buffer
	
	
	int		addSignature(	const char* );
	int		addType(		uint16_t );	// (Type)
	int		addUDP(			uint16_t );
	int		addTCP(			uint16_t );
	int		addHostname(	const char* );		// (Hostname, offset)
	int		addUsername(	const char* );		// (Username, offset)
	
	int		addByte(	BYTE b, int);
	int		addString(	const char* s, int);
	int		addMsg(		const char*);

public:
	LP2PM_Packet();
	~LP2PM_Packet();
	
	/*
	 *	Returns the size of the packet 
	 *	Includes: signature, type, payload, etc.
	 *
	 *	@return	int			packet size
	 */
	int		getSize();
	
	/*
	 *	Set the size of a packet
	 *	For packets that are created from incoming packets
	 *
	 *	@param	int			packet size
	 */
	void	setSize(int);
	
	/*
	 *	Prints the packet's contents (for debugging)
	 */
	void	printPacket();
	
	/*
	 *	Returns the packet's ID #
	 *
	 *	@return	int			packet ID
	 */
	int		getPID();
	
	/*
	 *	Clears the packet's contents/payload/buffer
	 */
	void	clearPacket();
	
	/*
	 *	Returns a Byte at the specified index
	 *	@param	int			Index
	 *	@return	BYTE		the byte
	 */
	BYTE	 getByte( int );
	
	/*
	 *	Returns the Type of the packet
	 *
	 *	@return	uint16_t	Packet's type
	 */
	uint16_t getType();
	
	/*
	 *	Returns the UDP port number
	 *
	 *	@return	uint16_t	UDP Port #
	 */
	uint16_t getUDPPort();
	
	/*
	 *	Returns the TCP port number
	 *
	 *	@return	uint16_t	TCP Port #
	 */
	uint16_t getTCPPort();
	
	/*
	 *	Returns the Hostname in the packet
	 *
	 *	@return	char*		Hostname contained in the packet
	 */
	char*	 getHostname();
	
	/*
	 *	Returns the Username in the packet
	 *
	 *	@return	char*		Username contained in the packet
	 */
	char*	 getUsername();
	
	/*
	 *	Returns a pointer to the beginning of the message in the packet
	 *
	 *	@return	char*		pointer to message
	 */
	char*	 getMsg();
	
	/*
	 *	Returns the message's size in the packet
	 *
	 *	@return	int			message's size
	 */
	int		 getMsgSize();
	
	/*
	 *	Returns a pointer to the position specified
	 *
	 *	@param	int				index
	 *	@return	unsigned char*	pointer to specified position
	 */
	unsigned char* getData( int );
	
	// Discovery Msg
	/*
	 *	@param1	const char*		Username
	 *	@param2	const char*		Hostname
	 *	@param3	uint16_t		UDP Port
	 *	@param4	uint16_t		TCP Port
	 *	@return	int				0 on success, -1 on fail
	 */
	int createDiscoveryMsg(	const char*, const char*, uint16_t, uint16_t );
	int createDisM(			const char*, const char*, uint16_t, uint16_t );
	
	// Reply Msg
	/*
	 *	@param1	const char*		Username
	 *	@param2	const char*		Hostname
	 *	@param3	uint16_t		UDP Port
	 *	@param4	uint16_t		TCP Port
	 *	@return	int				0 on success, -1 on fail
	 */
	int createReplyMsg(		const char*, const char*, uint16_t, uint16_t );
	int createRM(			const char*, const char*, uint16_t, uint16_t );
	
	// Closing Msg
	/*
	 *	@param1	const char*		Username
	 *	@param2	const char*		Hostname
	 *	@param3	uint16_t		UDP Port
	 *	@param4	uint16_t		TCP Port
	 *	@return	int				0 on success, -1 on fail
	 */
	int createClosingMsg(	const char*, const char*, uint16_t, uint16_t );
	int createCM(			const char*, const char*, uint16_t, uint16_t );
	
	// Request Communication Msg
	/*
	 *	@param	const char*		Username
	 *	@return	int				0 on success, -1 on fail
	 */
	int createRequestCom(	const char*); // (Username)
	int createRC(			const char*);
	
	// Accept Communication Msg
	/*
	 *	@return	int				0 on success, -1 on fail
	 */
	int createAcceptCom();
	int createAC();
	
	// Decline Communcation Msg
	/*
	 *	@return	int				0 on success, -1 on fail
	 */
	int createDeclineCom();
	int createDC();
	
	// Data Msg
	/*
	 *	@param	const char*		Payload
	 *	@return	int				0 on success, -1 on fail
	 */
	int createData(	const char*);
	int createDM(	const char*);
	
	// Message Msg
	/*
	 *	@param	const char*		Message
	 *	@return	int				0 on success, -1 on fail
	 */
	int createMessage(	const char*);
	int createMsg(		const char*);
	
	// Discontinue Communication Msg
	/*
	 *	@return	int				0 on success, -1 on fail
	 */
	int createDiscontinueCom();
	int createDisC();
	
	// Discovery Msg
	bool isDiscovery();
	bool isDisM();
	
	// Reply Msg
	bool isReply();
	bool isRM();
	
	// Closing Msg
	bool isClosing();
	bool isCM();
	
	// Request Communication Msg
	bool isRequestCom();
	bool isRC();
	
	// Accept Communication Msg
	bool isAcceptCom();
	bool isAC();
	
	// Decline Communication Msg
	bool isDeclineCom();
	bool isDC();
	
	// Data Msg
	bool isData();
	bool isDM();
	
	// Message Msg
	bool isMessage();
	bool isMsg();
	
	// Discontinue Communication Msg
	bool isDiscontinueCom();
	bool isDisC();

	friend std::ostream& operator<< (std::ostream &out, LP2PM_Packet &p){
		out << "----Packet ID: " << p.getPID() << "----\n";
		out << "Signature: " << *(p.getData(0)) << *(p.getData(1))
							<< *(p.getData(2)) << *(p.getData(3)) << "\n";
		out << "Type: " << p.getType() << "\n";
		if(p.isDisM() || p.isRM() || p.isCM()){
			out << "UDP Port: " << p.getUDPPort() << "\n"
				<< "TCP Port: " << p.getTCPPort() << "\n"
				<< "Hostname: " << p.getHostname() << "\n"
				<< "Username: " << p.getUsername() << "\n";
		}
		else if(p.isRC()) out << "Username: " << p.getUsername() << "\n";
		else if(p.isDM()) out << "Message: " << p.getMsg() << '\n';
		out << "---------------------\n";
		return out;
	}
};