P2P Messaging System (C++)
==========================

P2P Messaging System is a more stable version of the Secure P2P Messaging Client application, but without the encryption of packets. (Future development may include encryption, but as of now the development will be centered around stability and non-local network communication)

Description: 
P2P Messaging System is a peer-to-peer instant messaging client that is capable of sending and receiving messages from users on the same LAN. It currently uses local network broadcasts to discover other users on the same network. Future development will include communication between users who are not on the same network through the use of a server to help users find targeted users. Users will not be able to discover all users connected to the server. Users will be able to request the availability of another user and if the user is online (connected to the server), then the IP address and port #'s will be sent to the requesting user inorder for the requesting user to send a discovery message to the other user (to initialize the user connection).  
