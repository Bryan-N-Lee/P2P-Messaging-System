#include "LP2PM_Hostlist.h"

HostList::HostList():
head(NULL),length(0){}

HostList::~HostList()
{	while(!isEmpty()) pop(); }

bool HostList::isEmpty(){ return !head;  }
int HostList::size()	{ return length; }

void HostList::push(const char* user, const char* host)
{	head = new HostNode(user,host,head); ++length; }

void HostList::pop(){
	if(!head) return;
	HostNode* t = head->next;
	delete head;
	head = t;
	--length;
}

void HostList::remove(const char* user, const char* host){
	if(!head) return;
	HostNode* t = head;
	if(strcmp(head->username,user) == 0 && strcmp(head->hostname,host) == 0){
		head = head->next;
		delete t;
		--length;
		return;
	}
	for(HostNode* p = head->next; p; p = p->next){
		if(strcmp(p->username,user) == 0 && strcmp(p->hostname,host) == 0){
			t->next = p->next;
			delete p;
			--length;
			return;
		}
		t = p;
	}
}

HostNode* HostList::retrieve(const char* user, const char* host){
	for(HostNode* p = head; p; p = p->next){
		if(strcmp(p->username,user) == 0 && strcmp(p->hostname,host) == 0)
			return p;
	}
	return NULL;
}

HostNode* HostList::get(int index){
	if(0 > index || index >= length) return NULL;
	HostNode* p = head;
	for(int i = 0; i < length && p; ++i, p = p->next)
		if(index == i) return p;
	return NULL;
}

void HostList::printHosts(WINDOW* win){
	int i = 1;
	char clear_host[MAX_HOST_CHARS];
	memset(clear_host,' ',MAX_HOST_CHARS);
	for(HostNode* p = head; p; p = p->next, ++i){
		mvwprintw(win,i+3,1,"%s",clear_host);
		mvwprintw(win,i+3,1,"%s@%s",p->username,p->hostname);
	}
}
