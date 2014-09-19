#include <string>

class LP2PM_Exception{
private:
	int error_type;
	std::string error_message;
public:
	LP2PM_Exception(int type, const char* msg):
		error_type(type),error_message(msg){};
	LP2PM_Exception(int type):error_type(type),error_message(""){}
	~LP2PM_Exception(){};
	
	int type()				const { return error_type; }
	std::string message()	const { return error_message; }
	std::string msg()		const { return error_message; }
	const char*	c_str()		const { return error_message.c_str(); }
	
	//friend std::ostream& operator<< (std::ostream &out, LP2PM_Exception &e)
	//{	return (out << error_message); }
};