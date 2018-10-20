#ifndef SERVIDOR_RTP
#define SERVIDOR_RTP
#include <iostream>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#define CRLF "\r\n"
#define BACKLOG_MAX 5
#define PORT 8000

using namespace std;

typedef enum { INIT, READY, PLAYING, PAUSE } estado;
static int MJPEG_TYPE = 26; //RTP payload type for MJPEG video
static int FRAME_PERIOD = 100; //Frame period of the video to stream, in ms
static int VIDEO_LENGTH = 500;

unsigned __stdcall listenThread(void *);

class Tools{
public:
    static string convExt(string);
    static vector<string> split(string, char);
    template<class T> static T Error(string, T);
    template<class T> static T ErrorW(string, T);
    template<class T> static T ErrorS(string, T, int);
};

class Response{
private:
    string header, status, server, content_type, content_length, connection, buffer, date;
public:
    Response(){
        header = status = server = content_type = content_length = connection = buffer = "";
    }
    string getStatus(){
        return this->status;
    }
    string getServer(){
        return this->server;
    }
    string getContentType(){
        return this->content_type;
    }
    string getContentLength(){
        return this->content_length;
    }
    string getConnection(){
        return this->connection;
    }
    string getDate(){
        return this->date;
    }
    void setStatus(string status){
        this->status = status;
    }
    void setServer(string server){
        this->server = server;
    }
    void setContentType(string content_type){
        this->content_type = content_type;
    }
    void setContentLength(int content_length){
        this->content_length = to_string(content_length);
    }
    void setConnection(string connection){
        this->connection = connection;
    }
    void setDate(string date){
        this->date = date;
    }
    bool Send(string, int);
    void makeHeader();
};

class Request{
private:
    string header;
public:
    Request(){
        this->header = "";
    }
    string& getHeader(){
        return this->header;
    }
};

class VideoStream{
public:
    
    VideoStream(){
        filename = "";
    }

    VideoStream(string filename){
        this->filename = filename;
        this->file.open(this->filename, fstream::in | fstream::binary);
        this->frame = 0;
    }

    int frameN();
    int nextFrame(char *&);

    string filename;
    fstream file;
    int frame;
};

class Rtp{
public:

    Rtp(){
    }

    Rtp(int Ptype, int frameN, int time, char *data, int data_tam){
        version = 2;
        padding = extension = CC = marker = ssrc = 0;
    	payloadtype = Ptype;
        timestamp = time;
        sequencenumber = frameN;
        header.resize(headerSize);
        header[0] = (byte) (version << 6 | padding << 5 | extension << 4 | CC);
		header[1] = (byte) (marker << 7 | payloadtype);
		header[2] = (byte) (sequencenumber >> 8);
		header[3] = (byte) (sequencenumber & 0xFF);
		header[4] = (byte) (timestamp >> 24);
		header[5] = (byte) (timestamp >> 16);
		header[6] = (byte) (timestamp >> 8);
		header[7] = (byte) (timestamp & 0xFF);

		header[8] = (byte) (ssrc >> 24);
		header[9] = (byte) (ssrc >> 16);
		header[10] = (byte) (ssrc >> 8);
		header[11] = (byte) (ssrc & 0xFF);
        payload_size = data_tam;
        payload.resize(data_tam);
        memcpy((char *) payload.c_str(), data, data_tam);
    }

    Rtp(char *pack, int packsize){
        version = 2;
        padding = extension = CC = marker = ssrc = 0;

        if(packsize > headerSize){
            header.resize(headerSize);
            strncpy((char *) header.c_str(), pack, headerSize);

            payload_size = packsize - headerSize;
            payload.resize(payload_size);
            strncpy((char *) payload.c_str(), pack+headerSize, packsize);
            payloadtype = header[1] & 127;
            sequencenumber = unsigned(int(header[3])) + 256 * unsigned(int(header[2]));
            timestamp = unsigned(int(header[7])) + 256 * unsigned(int (header[6])) + 65536 *
                    unsigned(int(header[5])) + 16777216 * unsigned(int(header[4]));
        }
    }

    int getpayload_length(){
        return payload_size;
    }

    int getlength(){
        return payload_size + headerSize;
    }

    void print_header(){
        cout<<header;
    }

    char *getdata();
    char *getpayloaddata();


    const int headerSize = 12;
    int version, padding, extension, CC, marker, payloadtype, sequencenumber, timestamp, ssrc, payload_size;
    string payload, header;
};

class Server{
private:

    //thread send;
    int imagenb = 0; //image nb of the image currently transmitted
    int local_socket, remote_socket, udp_socket;
    unsigned short local_port, remote_port;
    struct sockaddr_in local_address, remote_address, udp_address;
    Request req;
    Response res;
    VideoStream stream;
    int estado = INIT;
    string session = "123456";
    HANDLE Thread; 
    WSADATA wsa_data;
public:
    Server(){
        local_socket = remote_socket = 0;
        local_port = remote_port = 0;
        memset(&local_address, 0, sizeof(local_address));
        memset(&remote_address, 0, sizeof(remote_address));
        memset(&udp_address, 0, sizeof(udp_address));
    }
    int getLocalPort(){
        return this->local_port;
    }
    int getRemotePort(){
        return this->remote_port;
    }
    void setLocalPort(int port){
        this->local_port = port;
    }
    void setRemotePort(int port){
        this->remote_port = port;
    }
    int getEstado(){
        return estado;
    }
    bool sendRtp();
    void prepareUdp(int);
    bool mylisten(int);
    bool mysend(string, string);
    bool sendOptions(string);
    bool sendDescribe(string, string);
    bool sendSetup(string, string, string);
    bool initServer();
    bool receiveRequest();
    void processRequest();
    void myclose();
    //void sendRtpThread(void *);

    int porta = PORT;
};

#endif