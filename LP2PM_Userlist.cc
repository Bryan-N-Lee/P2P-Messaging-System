#include "LP2PM_Userlist.h"

UserList::UserList():
head(NULL),size(0){}
	
bool UserList::isEmpty(){ return !head; }
	
int UserList::getSize(){ return size; }
	
void UserList::insert(LP2PM_User u){
	if(size >= LP2PM_DEFAULT_MAX_USERS) return;
	head = new ListNode(u,head);
	++size;
}

int UserList::remove(const char* username, const char* hostname){
	if(!head) return -1;
	ListNode* t = head;
	if(strcmp(head->user.getUsername(),username) == 0 &&
	   strcmp(head->user.getHostname(),hostname) == 0){
		head = head->next;
		delete t;
		--size;
		return 0;
	}
	for(ListNode* p = head->next; p; p = p->next){
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
	ListNode* t = head->next;
	head->user.shutdown();
	delete head;
	head = t;
	--size;
}

LP2PM_User* UserList::retrieve(const char* username, const char* hostname){
	for(ListNode* p = head; p; p = p->next){
		if(strcmp(username,p->user.getUsername()) == 0 &&
		   strcmp(hostname,p->user.getHostname()) == 0)
			return &(p->user);
	}
	return NULL;
}

LP2PM_User* UserList::getUser(int index){
	if(0 > index || index >= size) return NULL;
	ListNode* p = head;
	for(int i = 0; i < index && p; ++i, p = p->next)
		if(index == i) return &(p->user);
	return NULL;
}

LP2PM_User* UserList::findUser(sockaddr_in& a){
	for(ListNode* p = head; p; p = p->next){
		if(p->user.isSameUDPSockaddr(a) || p->user.isSameTCPSockaddr(a))
			return &(p->user);
	}
	return NULL;
}

void UserList::clearList(){
	while(!isEmpty()){ pop(); }
}

UserList::~UserList(){
	while(!isEmpty()){ pop(); }
}