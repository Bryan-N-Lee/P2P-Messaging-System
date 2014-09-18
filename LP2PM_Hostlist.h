#include "LP2PM_Macros.h"

struct HostNode{
	char hostname[MAX_HOSTNAME_LENGTH];
	char username[MAX_USERNAME_LENGTH];
	int status; // 0 = unestablished, 1 = connected
	HostNode* next;
	HostNode(const char* u, const char* h, HostNode* n):
	next(n),status(0){
		strcpy(hostname,h);
		strcpy(username,u);
	}
};

class HostList{
private:
	HostNode* head;
	int length;
	
public:
	HostList();
	~HostList();
	
	/*
	 *	Returns if list is empty (True) or not (False)
	 */
	bool isEmpty();
	
	/*
	 *	Returns size of list
	 */
	int size();
	
	/*
	 *	Pushes a new HostNode to beginning of list
	 *
	 *	@param1	const char*		new host's username
	 *	@param2	const char*		new host's hostname
	 */
	void push(const char*, const char*);
	
	/*
	 *	Pops the first HostNode in the list
	 */
	void pop();
	
	/*
	 *	Removes the HostNode matching the given username and hostname
	 *
	 *	@param1	const char*		host's username
	 *	@param2	const char*		host's hostname
	 */
	void remove(const char*, const char*);
	
	/*
	 *	Returns the HostNode that matches the given username and hostname
	 *
	 *	@param1	const char*		host's username
	 *	@param2	const char*		host's hostname
	 *	@return	HostNode*		HostNode containing the username and hostname
	 */
	HostNode* retrieve(const char* user, const char* host);
	
	/*
	 *	Returns the HostNode at index i
	 *
	 *	@param1	int				index
	 *	@return	HostNode*		HostNode at index i
	 */
	HostNode* get(int index);
	
	/*	
	 *	A redundant function now, but formally has the same role as
	 *	updateHostDisplay() does now ( and updateHostDisplay() preferable )
	 */
	void printHosts(WINDOW* win);
};
