#include "LP2PM_Userlist.h"

UserList::UserList():
head(NULL),size(0){}

std::string LP2PM_Userlist::toUpperCase(char* cstring)
{	std::string caps = "";
	for(int i = 0; cstring[i]; ++i) caps += toupper(cstring[i]);
	return caps;
}

bool LP2PM_Userlist::compareUserHost(const char* c1, const char* c2)
{	return toUppercase(c1) == toUppercase(c2); }
	
bool UserList::isEmpty(){ return !head; }
	
int UserList::getSize(){ return size; }
	
int UserList::insert(LP2PM_User u){
	if(size >= LP2PM_DEFAULT_MAX_USERS) return -1;
	head = new UserNode(u,head);
	return ++size;
}

int UserList::remove(const char* username, const char* hostname){
	if(!head) return -1;
	UserNode* t = head;
	if(strcmp(head->user.getUsername(),username) == 0 &&
	   strcmp(head->user.getHostname(),hostname) == 0){
		head = head->next;
		delete t;
		--size;
		return 0;
	}
	for(UserNode* p = head->next; p; p = p->next){
		if(strcmp(p->user.getUsername(),username) == 0 &&
		   strcmp(p->user.getHostname(),hostname) == 0){
			t->next = p->next;
			delete p;
			--size;
			return 0;
		}
		t = p;
	}
	return -1;
}

void UserList::pop(){
	if(!head) return;
	UserNode* t = head->next;
	head->user.shutdown();
	delete head;
	head = t;
	--size;
}

LP2PM_User* UserList::retrieve(const char* username, const char* hostname){
	for(UserNode* p = head; p; p = p->next){
		if(compareUserHost(username,p->user.getUsername()) &&
		   compareUserHost(hostname,p->user.getHostname())) return &(p->user);
	}
	return NULL;
}

LP2PM_User* UserList::retrieveIPv4(const char* username, const char* ip){
	for(UserNode* p = head; p; p = p->next){
		if(compareUserHost(username,p->user.getUsername()) &&
		   strcmp(p->user.checkIPv4(ip))) return &(p->user);
	}
	return NULL;
}

LP2PM_User* UserList::getUser(int index){
	if(0 > index || index >= size) return NULL;
	UserNode* p = head;
	for(int i = 0; i < index && p; ++i, p = p->next)
		if(index == i) return &(p->user);
	return NULL;
}

LP2PM_User* UserList::findUser(sockaddr_in& a){
	for(UserNode* p = head; p; p = p->next){
		if(p->user.isSameUDPSockaddr(a) || p->user.isSameTCPSockaddr(a))
			return &(p->user);
	}
	return NULL;
}

UserNode* UserList::root() const {	return head; }

void UserList::clearList(){
	while(!isEmpty()){ pop(); }
}

UserList::~UserList(){
	while(!isEmpty()){ pop(); }
}