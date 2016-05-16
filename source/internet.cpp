#include <network.h>
#include <errno.h>
#include <cstring>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/logging.h>

#include "internet.h"
#include "log.h"

Log* logp;

void sslLoggingWrapper(const int logLevel, const char* logMessage){
	logp->write(logMessage);
}

Internet::Internet(){
	log = new Log();
	logp = new Log("ssl.txt");
	wolfSSL_Debugging_ON();
	wolfSSL_SetLoggingCb(&sslLoggingWrapper);
	sslLoggingWrapper(0, "test");
	setState("Initializing netowrk...");			
	while (net_init() == -EAGAIN);		//Keep trying to connect
	if(if_config(localip,netmask,gateway,true)){
		setState("Failed to init network");	//See if it worked
	}
	else{
		setState("Netowrk ready");	
	}
}

Internet::~Internet(){
	net_close(socket);
	delete log;
	delete logp;
}

void Internet::sslReportError(){
	ssl = false;
	err = wolfSSL_get_error(sslSocket, 0);
	wolfSSL_ERR_error_string(err, errString);
	setState(errString);
}

bool Internet::sslSetup(){
	if (wolfSSL_Init() != SSL_SUCCESS) {
		sslReportError();
		return false;
	}
	setState("SSL Init");
	method = wolfSSLv23_client_method();
	if (method == NULL) {
		sslReportError();
		return false;
	}
	setState("SSL Method Set");

	ctx = wolfSSL_CTX_new(method);
	wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP160R1); //Send supported elliptical curves extension. Should be sent by default (tsk tsk)
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP192R1);
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP224R1);
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP256R1);
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP521R1);
	wolfSSL_CTX_UseSupportedCurve(ctx, WOLFSSL_ECC_SECP384R1);

	if (ctx == NULL) {
		sslReportError();
		return false;
	}
	setState("SSL Ctx Init");

	sslSocket = wolfSSL_new(ctx);

	if (sslSocket == NULL) {
		sslReportError();
		return false;
	}
	setState("SSL Socket Init");
	wolfSSL_set_fd(sslSocket, socket);

	if (sslSocket == NULL) {
		sslReportError();
		return false;
	}
	setState("SSL Socket connected to net socket");
	return true;
}

bool Internet::startSsl(){
	ssl = true;
	setState("Connecting SSL");
	if(!sslSetup()){
		return false;
	}
    if(!wolfSSL_connect(sslSocket) == SSL_SUCCESS){
		sslReportError();
		return false;
    }
    return true;
}

bool Internet::connect(char *server, int port, netProtocol_t protocol, bool _ssl){
	ssl = _ssl;
	setState("Connecting");
	if(!(server=addressToIp(server))){
		setState("Not a valid socket");
		return false;
	}
	ipaddr = inet_addr(server);		//Converts ip into standard u32
	struct sockaddr_in connect_addr;
	socket = net_socket(AF_INET, protocol == TCP ? SOCK_STREAM : SOCK_DGRAM,IPPROTO_IP);  //Create the socket
    if (socket < 0){			//Check for errors
    	setState("Failed to create socket");
		return false;
	}
	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = port;
	connect_addr.sin_addr.s_addr = ipaddr;
	
	if (net_connect(socket, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {    //Connect to the ip..
		net_close(socket);         //..And check for an error
		setState("Failed to connect");
		return false;
	}
	setState("Socket connected");
	if(ssl){
		startSsl();
    }
    return true;
}

void Internet::close(){
	if(ssl){
	    wolfSSL_free(sslSocket);
	    wolfSSL_CTX_free(ctx);
		wolfSSL_Cleanup();
	}
	net_close(socket);
	setState("Closed connection");
}

bool Internet::write(const char *data){
	if(ssl){
		if(wolfSSL_write(sslSocket, data, strlen(data))){
			setState(data);
			return true;
		}
		else{
			setState("Failed to write data");
			return false;
		}
	}
	else{
		if(net_write(socket, data, strlen(data))){
			setState(data);
			return true;
		}
		else{
			setState("Failed to write data");
			return false;
		}
	}
}

bool Internet::read(int bufferSize, char* buffer){
	int readData = 0;
	int totalRead = 0;

	if(buffer == NULL){
		setState("Buffer not allocated");
		return false;
	}

	if(ssl){
		if((readData = wolfSSL_read(sslSocket, buffer+totalRead, bufferSize-totalRead)) > 0){
			totalRead += readData;
		}
	}
	else{
		if((readData = net_read(socket, buffer+totalRead, bufferSize-totalRead)) > 0){
			totalRead += readData;
		}
	}

	buffer[totalRead] = '\0';
	setState(buffer);
	if(totalRead == 0){
		return false;
	}
	return true;
}

char* Internet::getIpByHostname(char *domain){
	struct hostent *host = NULL;
	host = net_gethostbyname(domain);
	if(host == NULL) {
		return NULL;
	}
	struct sockaddr_in tmp;
	memcpy(&tmp.sin_addr,host->h_addr_list[0],host->h_length);
	return inet_ntoa(tmp.sin_addr);
};


    // are we dealing with an ip or address
char* Internet::addressToIp(char *addressIn){
	for (unsigned int i=0;i<strlen(addressIn);i++) {
		char digit = addressIn[i];
		if((digit < 48 || digit > 57) && digit != 46) {
			return getIpByHostname(addressIn);
		}
	}
	return addressIn; //already ip
}


const char *Internet::getState(){
	return netstate;
}

void Internet::setState(const char* state){
	netstate = state;
	log->write(state);
}