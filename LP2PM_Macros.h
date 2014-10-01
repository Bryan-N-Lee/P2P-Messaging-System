/* ---- LP2PM_Exception Types ---- */
// LP2PM_Display

// LP2PM_Packet

// LP2PM_User

// LP2PM_Client

/* ---- LP2PM_Display ---- */
#define		MAX_USERNAME_LENGTH				32
#define		MAX_HOSTNAME_LENGTH				32

#define		MAX_MSG_HISTORY					35
#define		NLINES							30
#define		NCOLS							60

#define		HOST_WIN_WIDTH					40
#define		HOST_WIN_HEIGHT					30
#define		MAX_HOST_LINES					(HOST_WIN_HEIGHT - 4)
#define		MAX_HOST_CHARS					(HOST_WIN_WIDTH - 2)

#define		MSGS_WIN_WIDTH					(NCOLS + (NCOLS - HOST_WIN_WIDTH))
#define		MSGS_WIN_HEIGHT					HOST_WIN_HEIGHT
#define		MAX_MSGS_LINES					(MSGS_WIN_HEIGHT - 4)
#define		MAX_MSGS_CHARS					(MSGS_WIN_WIDTH - 4)

#define		TO_WIN_WIDTH					(NCOLS*2)
#define		TO_WIN_HEIGHT					3
#define		MAX_TO_CHARS					(TO_WIN_WIDTH - 6)

#define		MSG_WIN_WIDTH					(NCOLS*2)
#define		MSG_WIN_HEIGHT					3
#define		MAX_MSG_CHARS					(MSG_WIN_WIDTH - 11)

#define		CONSOLE_WIDTH					(NCOLS*2)
#define		CONSOLE_HEIGHT					11
#define		MAX_CONSOLE_LINES				(CONSOLE_HEIGHT - 2)
#define		MAX_CONSOLE_CHARS				(CONSOLE_WIDTH - 2)

#define		MAX_TO_LINE_ARGUS				3
#define		MAX_TO_LINE_LEN					40

#define		TO_WIDTH						5
#define		MSG_WIDTH						10

#define		DEBUG							0
#define		CONSOLE_DEBUG					1

// Commands
#define		REQUEST_COMMAND					"REQUEST"
#define		ACCEPT_COMMAND					"ACCEPT"
#define		DECLINE_COMMAND					"DECLINE"
#define		DISCONTINUE_COMMAND				"DISCONTINUE"
#define		AWAY_COMMAND					"AWAY"
#define		HERE_COMMAND					"HERE"
#define		TO_TYPE_REQUEST					1
#define		TO_TYPE_ACCEPT					2
#define		TO_TYPE_DECLINE					3
#define		TO_TYPE_DISCONTINUE				4
#define		TO_TYPE_AWAY					5
#define		TO_TYPE_HERE					6
#define		TO_TYPE_MESSAGE					0

/* ---- LP2PM_Packet ---- */
#define		LP2PM_TYPE_DISCOVERY			1
#define		LP2PM_TYPE_REPLY				2
#define		LP2PM_TYPE_CLOSING				3
#define		LP2PM_TYPE_REQUEST				4
#define		LP2PM_TYPE_ACCEPT				5
#define		LP2PM_TYPE_DECLINE				6
#define		LP2PM_TYPE_DATA					9
#define		LP2PM_TYPE_MESSAGE				7
#define		LP2PM_TYPE_DISCONTINUE			10
#define		LP2PM_SIGNATURE					"LP2PM"

#define		LP2PM_TYPE_POS_L				5
#define		LP2PM_TYPE_POS_R				6
#define		LP2PM_UDP_POS_L					7
#define		LP2PM_UDP_POS_R					8
#define		LP2PM_TCP_POS_L					9
#define		LP2PM_TCP_POS_R					10
#define		LP2PM_MSG_POS					7
#define		LP2PM_HOSTNAME_POS				11
#define		LP2PM_USER_REQ_POS				7

#define		LP2PM_DEFAULT_MAX_PACKET_SIZE	512

/* ---- LP2PM_User ---- */
#define		MAX_MSG_LENGTH					(LP2PM_DEFAULT_MAX_PACKET_SIZE - 7)
#define		MSG_HISTORY_LENGTH				12

/* ---- LP2PM_Client ---- */
#define		LP2PM_DEFAULT_MAX_USERS			(HOST_WIN_HEIGHT)
#define		LP2PM_DEFAULT_INIT_TIMEOUT		5000		// 5 seconds
#define		LP2PM_DEFAULT_MAX_TIMEOUT		60000		// 50 seconds
#define		LP2PM_DEFAULT_UDP_PORT			50550
#define		LP2PM_DEFAULT_TCP_PORT			50551