#include <panel.h>
#include <cstring>
#include <string>
#include <ctype.h>
#include <string>
#include <iostream>
#include <algorithm>
#include "LP2PM_Hostlist.h"
#include "LP2PM_Macros.h"

/*
 * compile: -lpanel -lncurses
 *
 */

using namespace std;

static char NEW_CONSOLE_LINE[MAX_CONSOLE_CHARS];

class LP2PM_Display{
private:
	static LP2PM_Display*	LP2PM_Display_Instance;
	char					hostname[MAX_HOSTNAME_LENGTH];
	char					username[MAX_USERNAME_LENGTH];
	char					toArguments[MAX_TO_LINE_ARGUS][MAX_TO_LINE_LEN];
	short					nToArgus;
	HostList				hostList;
	char					msg_history[MAX_MSGS_LINES][MAX_MSGS_CHARS];
	char					clear_msg_line[MAX_MSGS_CHARS];
	char					console_history[MAX_CONSOLE_LINES][MAX_CONSOLE_CHARS];
	char					clear_console_line[MAX_CONSOLE_CHARS];
	int						to_i;
	int						msg_i;
	string					to_line;
	string					msg_line;
	WINDOW*					header_window;
	WINDOW*					hosts_window;
	WINDOW*					messages_window;
	WINDOW*					to_window;
	WINDOW*					msg_window;
	WINDOW*					console_window;
	PANEL*					header_panel;
	PANEL*					hosts_panel;
	PANEL*					messages_panel;
	PANEL*					to_window_panel;
	PANEL*					msg_window_panel;
	PANEL*					console_panel;

	PANEL*					top;

	void printInMiddle( WINDOW *win, int starty, int startx, int width,
					   char *string, chtype color );
	
/* ---- Window Initalizers ---- */
	/*
	 *	initialize Header "window"/title
	 */
	void initializeHeader();
	
	/*
	 *	initialize Host Window
	 */
	void initializeHost();
	
	/*
	 *	initialize Messages Window
	 */
	void initializeMessages();
	
	/*
	 *	initialize To Window/Line
	 */
	void initializeTo();
	
	/*
	 *	initialize Message Window/Line
	 */
	void initializeMsg();
	
	/*
	 *	initialize Console Window
	 */
	void initializeConsole();

	
/* ---- Move Display Lines ---- */
	/*
	 *	Move Messages up the message history
	 */
	void moveMsgsUp();
	
	/*
	 *	Move Console Messages up the console history
	 */
	void moveConsoleUp();
	
	
/* ---- Update Displays ---- */
	/*
	 *	Update the Message display/window
	 */
	void updateMsgDisplay();
	
	/*
	 *	Update the Console display/window
	 */
	void updateConsoleDisplay();
	
	/*
	 *	Update the Host display/window
	 */
	void updateHostDisplay();
	
	
/* ---- Helper Functions ---- */
	/*
	 *	Clear the Host Display/window
	 */
	void clearHostDisplay();
	
	/*
	 *	Insert a line at the end of the message history
	 *
	 *	@param	const char*		line to add to end of message history
	 */
	void insertMsgLine( const char* );
	
	/*
	 *	Given a new message (with username and host combo included)
	 *	addNewLine() adds the message (formated) to the message window.
	 *
	 *	@param	const char*		New Message to add
	 */
	void addNewLine( const char* );
	
	/*
	 *	Parses the arguments entered in the To window/line
	 */
	void parseToLineArguments();
	
	/*
	 *	Given a cstring like [user]@[host], this function will parse it into
	 *	user and host
	 *
	 *	@param1	const char*		[user]@[host] cstring
	 *	@param2	char*			user buffer
	 *	@param3	char*			host buffer
	 *	@return	int				0 on success, -1 on fail/error
	 */
	int parseUserHost( const char* );
	
	/*
	 *	Called when the [backspace]/[delete] key is pressed
	 */
	void backspace();
	
	/*
	 *	Based on the given key, handles it accordingly
	 *
	 *	@param	char		key pressed
	 */
	void addCharacter( char );
	
	/*
	 *	Convert cstring to Uppercase
	 *	@param1	char*		cstring
	 */
	std::string toUpperCase( char* );
	/*
	 *	@param1	char*		cstring
	 *	@param2	int			# of chars to convert to uppercase
	 */
	std::string toUpperCase( char*, int );
	
	/*
	 *	Private for singleton purposes
	 */
	LP2PM_Display();
	~LP2PM_Display();
	
public:
	
	/*
	 *	Create or get LP2PM_Display instance
	 */
	static LP2PM_Display* Instance();
	
	/*
	 *	Destroy LP2PM_Display instance
	 */
	static void Destroy();
	
	/*
	 *	Initializes the LP2PM_Display
	 *	Should be called when ready to display the user interface
	 *
	 *	@param1	const char*		username
	 *	@param2	const char*		hostname
	 */
	void init( const char*, const char* );
	
	/*
	 *	Set the User's username (is displayed in the user interface)
	 *
	 *	@param	const char*		username
	 */

	void setUsername( const char* );
	
	/*
	 *	Set the User's hostname (is displayed in the user interface)
	 *
	 *	@param	const char*		hostname
	 */
	void setHostname( const char* );
	
	/*
	 *	Change the User's status
	 *
	 *	@param1	const char*		username
	 *	@param2	const char*		hostname
	 *	@param3	int				new status
	 */
	void changeUserStatus( const char*, const char*, int );
	
	/*
	 *	Add a new Host to the Host window & list
	 *
	 *	@param1	const char*		host's username
	 *	@param2 const char*		host's hostname
	 */
	void addNewHost( const char*, const char* );
	
	/*
	 *	Remove a Host from the Host window & list
	 *
	 *	@param1	const char*		host's username
	 *	@param2 const char*		host's hostname
	 */
	void removeHost( const char*, const char* );
	
	/*
	 *	Add a new message from a user to the Message Window
	 *
	 *	@param1	const char*		message sender's username
	 *	@param2	const char*		message sender's hostname
	 *	@param3	const char*		message
	 */
	void addNewMessage( const char*, const char*, const char* );
	
	/*
	 *	Add a new console message the the Console Window
	 *
	 *	@param1	const char*		new message/line to be added to console window
	 */
	void addNewConsoleLine(	const char* );
	
	/*
	 *	Adds a new message from the current user to another user
	 *
	 *	@param1	const char*		username of recipient
	 *	@param2	const char*		message
	 */
	void addNewUserMsg( const char*, const char* );
	
	/*
	 *	This is usually called from outside this class
	 *	It handles user keyboard input
	 *
	 *	@param	char			keyboard input
	 */
	void updateDisplay( char );
	
	/*
	 *	Store the To line and parses the arguments
	 *
	 *	@param1	char*		destination username
	 *	@param2	char*		destination hostname
	 *	@return	int			type of argument submitted:
	 *						Type 0: Message
	 *						Type 1: Request
	 *						Type 2:	Accept
	 *						Type 3:	Decline
	 *						Type 4:	Discontinue
	 *						Type 5:	Away (status change)
	 *						Type 6:	Here (status change)
	 */
	int	 getToLine( char*, char* ); // FIXIT
	
	/*
	 *	Store the Message line in given buffer
	 *
	 *	@param1	char*		buffer
	 *	@param2	int			size of @param1 buffer
	 */
	void getMsgLine( char*, int );
	
	/*
	 *	Clears the To and Message Line windows 
	 *	(Usually called after [return] is pressed
	 */
	void clearToAndMsgLines();
};
