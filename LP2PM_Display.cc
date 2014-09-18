#include "LP2PM_Display.h"

// TODO
//	[ ] Should be a singleton

LP2PM_Display::LP2PM_Display():
to_i(0),msg_i(0),nToArgus(0)
{	if(DEBUG) cout << "LP2PM_Display::LP2PM_Display()" << endl;
	bzero(hostname,MAX_HOSTNAME_LENGTH);
	bzero(username,MAX_USERNAME_LENGTH);
	//memset(hostname,0,MAX_HOSTNAME_LENGTH);
	//memset(username,0,MAX_USERNAME_LENGTH);
	
	to_line = "";
	msg_line = "";
	
	/* Init Message History */
	for(int i = 0; i < MAX_MSGS_LINES; ++i)
		bzero(msg_history[i],MAX_MSGS_CHARS);
		//memset(msg_history[i],0,MAX_MSGS_CHARS);
	/* Init Console History */
	for(int i = 0; i < MAX_CONSOLE_LINES; ++i)
		bzero(console_history[i],MAX_CONSOLE_CHARS);
		//memset(console_history[i],0,MAX_CONSOLE_CHARS);
	/* Init To Line */
	for(int i = 0; i < MAX_TO_LINE_ARGUS; ++i)
		bzero(toArguments[i],MAX_TO_LINE_LEN);
		//memset(toArguments[i],0,MAX_TO_LINE_LEN);
	
	memset(clear_msg_line,' ',MAX_MSGS_CHARS);
	clear_msg_line[MAX_MSGS_CHARS-1] = '\0';
	
	memset(clear_console_line,' ',MAX_CONSOLE_CHARS);
	clear_console_line[MAX_CONSOLE_CHARS-1] = '\0';
	if(DEBUG) cout << "LP2PM_Display::LP2PM_Display() - end" << endl;
}

LP2PM_Display::~LP2PM_Display()
{	if(DEBUG) cout << "LP2PM_Display::~LP2PM_Display()" << endl;
	endwin();
}

void LP2PM_Display::init(const char* u, const char* h)
{	setUsername(u);
	setHostname(h);
	
	/* Initialize curses */
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	
	/* Initialize all the colors */
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);
	
	initializeHeader();
	initializeHost();
	initializeMessages();
	initializeTo();
	initializeMsg();
	initializeConsole();
	
	/* Attach a panel to each window */
	header_panel =		new_panel(header_window);
	hosts_panel =		new_panel(hosts_window);
	messages_panel =	new_panel(messages_window);
	msg_window_panel =	new_panel(msg_window);
	console_panel =		new_panel(console_window);
	to_window_panel =	new_panel(to_window);
	
	/* Set up the user pointers to the next panel */
	set_panel_userptr(to_window_panel,msg_window_panel);
	set_panel_userptr(msg_window_panel,to_window_panel);
	
	/* show it on screen */
	attron(COLOR_PAIR(4));
	mvprintw(LINES - 4, 2, "Use [tab] to which between TO and MESSAGE");
	mvprintw(LINES - 3, 2, "(F1 to Exit)");
	attroff(COLOR_PAIR(4));
	
	attron(COLOR_PAIR(COLOR_GREEN));
	mvprintw(2,2,"User: %s",username);
	mvprintw(3,2,"Host: %s",hostname);
	attroff(COLOR_PAIR(COLOR_GREEN));
	
	top = to_window_panel;
	
	/* Update the stacking order. To Window panel will be on top */
	update_panels();
	doupdate();
}

void LP2PM_Display::printInMiddle(WINDOW *win, int starty, int startx,
								  int width, char *string, chtype color)
{	int length, x, y;
	float temp;
	
	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;
	
	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

/*
 *	Initailize the Header
 */
void LP2PM_Display::initializeHeader()
{	if(DEBUG) cout << "LP2PM_Display::initializeHeader()" << endl;
	char* label = (char*)"P2P IM Client";
	int x = 31, y = 1, window_height = 3, window_width = 60, color = 4;
	header_window = newwin(window_height,window_width,y,x);
	
	int startx, starty, height, width;
	getbegyx(header_window,starty,startx);
	getmaxyx(header_window,height,width);
	
	box(header_window,0,0);
	
	printInMiddle(header_window,1,0,width,label,COLOR_PAIR(color));
}

/*
 *	Initailize the Hosts Window
 */
void LP2PM_Display::initializeHost()
{	if(DEBUG) cout << "LP2PM_Display::initializeHost()" << endl;
	char* label = (char*)"Hosts/Users";
	int x = 2, y = 4, color = 3;
	hosts_window = newwin(HOST_WIN_HEIGHT,HOST_WIN_WIDTH,y,x);
	
	int startx, starty, height, width;
	getbegyx(hosts_window,starty,startx);
	getmaxyx(hosts_window,height,width);
	
	box(hosts_window,0,0);
	mvwaddch(hosts_window, 2, 0, ACS_LTEE);
	mvwhline(hosts_window, 2, 1, ACS_HLINE, width - 1);
	mvwaddch(hosts_window, 2, width - 1, ACS_RTEE);
	
	printInMiddle(hosts_window,1,0,width,label, COLOR_PAIR(color));
}

/*
 *	Initailize the Messages Window
 */
void LP2PM_Display::initializeMessages()
{	if(DEBUG) cout << "LP2PM_Display::initializeMessages()" << endl;
	char* label = (char*)"Messages";
	int x = 2 + HOST_WIN_WIDTH, y = 4, color = 2;
	messages_window = newwin(MSGS_WIN_HEIGHT,MSGS_WIN_WIDTH,y,x);
	
	int startx, starty, height, width;
	getbegyx(messages_window,starty,startx);
	getmaxyx(messages_window,height,width);
	
	box(messages_window,0,0);
	mvwaddch(messages_window, 2, 0, ACS_LTEE);
	mvwhline(messages_window, 2, 1, ACS_HLINE, width - 1);
	mvwaddch(messages_window, 2, width - 1, ACS_RTEE);
	
	printInMiddle(messages_window,1,0,width,label, COLOR_PAIR(color));
}

/*
 *	Initailize the To Window/Line
 */
void LP2PM_Display::initializeTo()
{	if(DEBUG) cout << "LP2PM_Display::initializeTo()" << endl;
	int x = 2, y = 4 + HOST_WIN_HEIGHT, color = 5;
	to_window = newwin(TO_WIN_HEIGHT,TO_WIN_WIDTH,y,x);
	
	int startx, starty, height, width;
	getbegyx(to_window,starty,startx);
	getmaxyx(to_window,height,width);
	
	box(to_window,0,0);
	mvwprintw(to_window,1,1,"%s","TO: ");
}

/*
 *	Initailize the Message Window/Line
 */
void LP2PM_Display::initializeMsg()
{	if(DEBUG) cout << "LP2PM_Display::initializeMsg()" << endl;
	int x = 2, y = 4 + HOST_WIN_HEIGHT + TO_WIN_HEIGHT, color = 5;
	msg_window = newwin(MSG_WIN_HEIGHT,MSG_WIN_WIDTH,y,x);
	
	int startx, starty, height, width;
	getbegyx(msg_window,starty,startx);
	getmaxyx(msg_window,height,width);
	
	box(msg_window,0,0);
	mvwprintw(msg_window,1,1,"%s","MESSAGE: ");
}

/*
 *	Initailize the Console Window
 */
void LP2PM_Display::initializeConsole()
{	if(DEBUG) cout << "LP2PM_Display::initializeConsole()" << endl;
	int x = 2, y = 4 + HOST_WIN_HEIGHT + TO_WIN_HEIGHT + MSG_WIN_HEIGHT,
	color = 5;
	console_window = newwin(CONSOLE_HEIGHT,CONSOLE_WIDTH,y,x);
	
	int startx, starty, height, width;
	getbegyx(console_window,starty,startx);
	getmaxyx(console_window,height,width);
	
	box(console_window,0,0);
}

/*
 *	Move Message History Up (leaving last line blank)
 */
void LP2PM_Display::moveMsgsUp()
{	for(int i = 0; i < MAX_MSGS_LINES - 1; ++i)
	{	memset(msg_history[i],' ',MAX_MSGS_CHARS);
		strcpy(msg_history[i],msg_history[i+1]);
		msg_history[i][strlen(msg_history[i+1])] = ' ';
		msg_history[i][MAX_MSGS_CHARS-1] = '\0';
	}
	memset(msg_history[MAX_MSGS_LINES - 1],' ',MAX_MSGS_CHARS);
	msg_history[MAX_MSGS_LINES-1][MAX_MSGS_CHARS-1] = '\0';
}

/*
 *	Move Console History Up (leaving last line blank)
 */
void LP2PM_Display::moveConsoleUp()
{	for(int i = 0; i < MAX_CONSOLE_LINES - 1; ++i)
	{	memset(console_history[i],' ',MAX_CONSOLE_CHARS);
		strcpy(console_history[i],console_history[i+1]);
		console_history[i][strlen(console_history[i+1])] = ' ';
		console_history[i][MAX_CONSOLE_CHARS-1] = '\0';
	}
	memset(console_history[MAX_CONSOLE_LINES - 1],' ',MAX_CONSOLE_CHARS);
	console_history[MAX_CONSOLE_LINES-1][MAX_CONSOLE_CHARS-1] = '\0';
}

/*
 *	Write Message History to Message window
 */
void LP2PM_Display::updateMsgDisplay()
{	for(int i = 0; i < MAX_MSGS_LINES; ++i)
	{	mvwprintw(messages_window,i+3,1,"%s",clear_msg_line);
		mvwprintw(messages_window,i+3,1,"%s",msg_history[i]);
	}
}

/*
 *	Write Console History to Console window
 */
void LP2PM_Display::updateConsoleDisplay()
{	for(int i = 0; i < MAX_CONSOLE_LINES; ++i)
	{	mvwprintw(console_window,i+1,1,"%s",clear_console_line);
		mvwprintw(console_window,i+1,1,"%s",console_history[i]);
	}
}

/*
 *	prints found hosts to Host Window
 */
void LP2PM_Display::updateHostDisplay()
{	int i = 1;
	for(HostNode* h = hostList.get(0); h; h = h->next, ++i)
	{	if(strlen(h->username)+strlen(h->hostname)+1 > MAX_HOST_CHARS)
		{	char temp[MAX_USERNAME_LENGTH+MAX_HOSTNAME_LENGTH+1];
			snprintf(temp,MAX_HOST_CHARS,"%s@%s",h->username,h->hostname);
			temp[MAX_HOST_CHARS-4] = '.'; temp[MAX_HOST_CHARS-3] = '.';
			temp[MAX_HOST_CHARS-2] = '.'; temp[MAX_HOST_CHARS-1] = '\0';
			mvwprintw(hosts_window,i+2,1,"%s",temp);
		}
		else mvwprintw(hosts_window,i+2,1,"%s@%s",h->username,h->hostname);
	}
}

/*
 *	Clears Hosts window
 */
void LP2PM_Display::clearHostDisplay()
{	char clear[MAX_HOST_CHARS];
	memset(clear,' ',MAX_HOST_CHARS);
	clear[MAX_HOST_CHARS-1] = '\0';
	for(int i = 1; i < hostList.size()+1; ++i)
		mvwprintw(hosts_window,i+2,1,"%s",clear);
}

/*
 *	Insert a line at the end of the message history
 *
 *	@param	msg		line to add to end of message history
 */
void LP2PM_Display::insertMsgLine(const char* msg)
{	moveMsgsUp();
	if(strlen(msg) > MAX_MSGS_CHARS)
		snprintf(msg_history[MAX_MSGS_LINES-1],MAX_MSGS_CHARS,"%s",msg);
	else sprintf(msg_history[MAX_MSGS_LINES-1],"%s",msg);
}

/*
 *	Given a new message (with username and host combo included)
 *	addNewLine() adds the message (formated) to the message window.
 *
 *	@param	line	New Message to add
 */
void LP2PM_Display::addNewLine(const char* line)
{	if(strlen(line) < MAX_MSGS_CHARS){ insertMsgLine(line); return; }
	string cur_word = "", sentence = "";
	for(int i = 0; i < MAX_MSGS_CHARS; ++i)
	{	if(line[i] != ' ') cur_word += line[i];
		else
		{	sentence += (cur_word + " ");
			cur_word = "";
		}
	}
	insertMsgLine(sentence.c_str());
	addNewLine(line+sentence.length());
}

/*
 *	Parse out the arguments given in the To Window/Line
 */
void LP2PM_Display::parseToLineArguments()
{	for(int i = 0; i < MAX_TO_LINE_ARGUS; ++i)
		bzero(toArguments[i],MAX_TO_LINE_LEN);
		//memset(toArguments[i],0,MAX_TO_LINE_LEN);
	string t = "";
	int k = 0;
	for(int i = 0; i < to_line.size(); ++i)
	{	if(to_line[i] == '@')
		{	strcpy(toArguments[k],t.c_str());
			++k; t = "";
		}
		else t += to_line[i];
	}
	strcpy(toArguments[k],t.c_str());
	nToArgus = k+1;
}

/*
 *	Handle [delete]/[backspace] correctly
 */
void LP2PM_Display::backspace()
{	if(top == to_window_panel && to_i > 0)
	{	mvwprintw(to_window,1,--to_i + TO_WIDTH," ");
		mvwprintw(to_window,1,to_i + TO_WIDTH,"");
		to_line.erase(to_i,1);
	}
	else if(top == msg_window_panel && msg_i > 0)
	{	mvwprintw(msg_window,1,--msg_i + MSG_WIDTH," ");
		mvwprintw(msg_window,1,msg_i + MSG_WIDTH,"");
		msg_line.erase(msg_i,1);
	}
}

/*
 *	Adds the given character to the top panel (To or Message)
 *
 *	@param	ch		character typed
 */
void LP2PM_Display::addCharacter(char ch)
{	if(isprint(ch))
	{	if(top == to_window_panel && to_line.length() < MAX_TO_CHARS)
		{	mvwprintw(to_window,1,TO_WIDTH + to_i++,"%c",ch);
			to_line += ch;
		}
		else if(top == msg_window_panel && msg_line.length() < MAX_MSG_CHARS)
		{	mvwprintw(msg_window,1,MSG_WIDTH + msg_i++,"%c",ch);
			msg_line += ch;
		}
	}
}

void LP2PM_Display::setHostname(const char* h) { strcpy(hostname,h); }

void LP2PM_Display::setUsername(const char* u) { strcpy(username,u); }

void LP2PM_Display::changeUserStatus(const char* user, const char* host,
									 int status)
{	HostNode* u = hostList.retrieve(user,host);
	if(u) u->status = status;
}

void LP2PM_Display::addNewHost(const char* user, const char* host)
{	clearHostDisplay();
	hostList.push(user,host);
	updateHostDisplay();
}

void LP2PM_Display::removeHost(const char* user, const char* host)
{	clearHostDisplay();
	hostList.remove(user,host);
	updateHostDisplay();
}

void LP2PM_Display::addNewMessage(const char* user, const char* host,
								  const char* msg)
{	if(DEBUG){
		cout << "LP2PM_Display::addNewMessage() Adding new message: \'"
		<< user << "@" << host << ": " << msg << endl;
	}
	string newLine = user;
	(((newLine += "@") += host) += ": ") += msg;
	addNewLine(newLine.c_str());
	updateMsgDisplay();
}

void LP2PM_Display::addNewConsoleLine(const char* newLine)
{	moveConsoleUp();
	if(strlen(newLine) > MAX_CONSOLE_CHARS)
		strncpy(console_history[MAX_CONSOLE_LINES-1],newLine,MAX_CONSOLE_CHARS);
	else strcpy(console_history[MAX_CONSOLE_LINES-1],newLine);
	
	console_history[MAX_CONSOLE_LINES-1][strlen(newLine)] = ' ';
	console_history[MAX_CONSOLE_LINES-1][MAX_CONSOLE_CHARS-1] = '\0';
	updateConsoleDisplay();
}

void LP2PM_Display::addNewUserMessage(const char* toUsername, const char* msg)
{	addNewMessage(username,toUsername,msg); }

// 0 = new character
// -1 = delete/backspace
void LP2PM_Display::updateDisplay(char ch)
{	switch(ch)
	{	case 9: // Tab
			top = (PANEL*)panel_userptr(top);
			top_panel(top);
			break;
		case 0x7F: // backspace/delete
			backspace();
			break;
		case 0x0D: // return / \n
		case 10:	// return
			addNewMessage(username,to_line.c_str(),msg_line.c_str());
			clearToAndMsgLines();
			break;
		case -1:	// Window sized changed
		case -102:
			break;
		default:
			addCharacter(ch);
			//char character[10];
			//sprintf(character,"Character Entered: \'%c\' or \'%d\'",ch,ch);
			//addNewConsoleLine((const char*)character);
	}
	update_panels();
	doupdate();
}

// 0 - single user
// 1 - establish
// 2 - disconnect
// 3 - all
// 4 - request list
// 5 - change user availiablity status
int LP2PM_Display::getToLine(char* dest_user, char* dest_host)
{	parseToLineArguments();
	if(strcmp(toArguments[0],(char*)"establish") == 0 ||
	   strcmp(toArguments[0],(char*)"Establish") == 0)
	{	strcpy(dest_user,toArguments[1]);
		strcpy(dest_host,toArguments[2]);
		return 1;
	}
	if(strcmp(toArguments[0],(char*)"disconnect") == 0 ||
	   strcmp(toArguments[0],(char*)"Disconnect") == 0)
	{	if(strcmp(toArguments[1],(char*)"all") == 0 ||
		   strcmp(toArguments[1],(char*)"All") == 0)
		{	strcpy(dest_user,toArguments[1]);
			return 2;
		}
		strcpy(dest_user,toArguments[1]);
		strcpy(dest_host,toArguments[2]);
		return 2;
	}
	if(strcmp(toArguments[0],(char*)"all") == 0 ||
	   strcmp(toArguments[0],(char*)"All") == 0)
	{	strcpy(dest_user,toArguments[0]);
		return 3;
	}
	if(strcmp(toArguments[0],(char*)"requestuserlist") == 0 ||
	   strcmp(toArguments[0],(char*)"Requestuserlist") == 0)
	{	strcpy(dest_user,toArguments[1]);
		strcpy(dest_host,toArguments[2]);
		return 4;
	}
	if(strcmp(toArguments[0],(char*)"changestaus") == 0) return 5;
	if(nToArgus <= 1)
	{	strcpy(dest_user,"");
		strcpy(dest_host,"");
		return -1;
	}
	strcpy(dest_user,toArguments[0]);
	strcpy(dest_host,toArguments[1]);
	return 0;
}

void LP2PM_Display::getMsgLine(char* buffer, int size)
{	if(size < msg_line.size()) strncpy(buffer,msg_line.c_str(),size);
	else strcpy(buffer,msg_line.c_str());
}

void LP2PM_Display::clearToAndMsgLines()
{	for(;to_i > 0;)
	{	mvwprintw(to_window,1,--to_i + TO_WIDTH," ");
		mvwprintw(to_window,1,to_i + TO_WIDTH,"");
	}
	for(;msg_i > 0;)
	{	mvwprintw(msg_window,1,--msg_i + MSG_WIDTH," ");
		mvwprintw(msg_window,1,msg_i + MSG_WIDTH,"");
	}
	to_line = "";
	msg_line = "";
}