#include "LP2PM_User.h"
#include "LP2PM_Macros.h"

struct UserNode{
	LP2PM_User user;
	UserNode* next;
	UserNode(LP2PM_User u, UserNode* n):
	user(u),next(n){}
};

class UserList{
private:
	UserNode* head;
	int size;
public:
	UserList();
	~UserList();
	
	/*
	 *	returns True if list is empty
	 */
	bool isEmpty();
	
	/*
	 *	returns size of list
	 *
	 *	@return	int		size of list
	 */
	int getSize();
	
	
	/*
	 *	Insert given user
	 *
	 *	@param	LP2PM_User	The new user
	 *	@return	int			new size of list on success, -1 on fail
	 */
	int insert( LP2PM_User );
	
	/*
	 *	Remove user based on username and hostname
	 *
	 *	@param1	const char*		username
	 *	@param2 const char*		hostname
	 *	@return	int				0 on success, -1 on fail
	 */
	int remove( const char*, const char* );
	
	/*
	 *	removes the first node/user in the list
	 */
	void pop();
	
	
	/*
	 *	returns the user based on username and hostname
	 *
	 *	@param1	const char*		username
	 *	@param2	const char*		hostname
	 *	@return	LP2PM_User*		pointer to user
	 */
	LP2PM_User* retrieve( const char*, const char* );
	
	/*
	 *	returns user at specified index
	 *
	 *	@param1	int				index
	 *	@return	LP2PM_User*		pointer to user
	 */
	LP2PM_User* getUser( int );
	
	/*
	 *	returns user based on sockaddr_in
	 *
	 *	@param1	sockaddr_in&	address of user
	 *	@return	LP2PM_User*		pointer to user
	 */
	LP2PM_User* findUser( sockaddr_in& );
	
	/*
	 *	returns the root/head of the list
	 *
	 *	@return	UserNode*		head of list
	 */
	UserNode* root() const;
	
	/*
	 *	empties the list
	 */
	void clearList();
};