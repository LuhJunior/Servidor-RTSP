#ifndef SERVIDOR_HTTP
#define SERVIDOR_HTTP
#include <iostream>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>
#include <time.h>
#define RFC1123     "%a, %d %b %Y %H:%M:%S GMT" 
#define BACKLOG_MAX 5
#define PORT 8000

using namespace std;

typedef enum { INIT, READY, PLAYING } estado;
string CRLF = "\r\n";

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
    string getHeader(){
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
    char *nextFrame();

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
        
        sequencenumber = frameN;
        timestamp = time;
        payloadtype = Ptype;

        header.resize(headerSize);

        ((char *) header.c_str())[0] = version << 6;
		((char *) header.c_str())[0] = header.c_str()[0] | padding << 5;
		((char *) header.c_str())[0] = ((char *) header.c_str())[0] | extension << 4;
		((char *) header.c_str())[0] = ((char *) header.c_str())[0] | CC;
		((char *) header.c_str())[1] = marker << 7;
		((char *) header.c_str())[1] = ((char *) header.c_str())[1] | payloadtype;

		((char *) header.c_str())[2] = sequencenumber;
		((char *) header.c_str())[3] = sequencenumber;

		((char *) header.c_str())[4] = (timestamp >> 24) & 0xFF;
		((char *) header.c_str())[5] = (timestamp >> 16) & 0xFF;
		((char *) header.c_str())[6] = (timestamp >> 8) & 0xFF;
		((char *) header.c_str())[7] = timestamp & 0xFF;

		((char *) header.c_str())[8] = ssrc >> 24;
		((char *) header.c_str())[9] = ssrc >> 16;
		((char *) header.c_str())[10] = ssrc >> 8;
		((char *) header.c_str())[11] = ssrc;

        payload_size = data_tam;
        payload.resize(data_tam);
        strcpy((char *) payload.c_str(), data);
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
            sequencenumber = unsigned int(header[3]) + 256 * unsigned int(header[2]);
            timestamp = unsigned int(header[7]) + 256 * unsigned int (header[6]) + 65536 *
                    unsigned int(header[5]) + 16777216 * unsigned int(header[4]);
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
    int local_socket, remote_socket;
    unsigned short local_port, remote_port;
    struct sockaddr_in local_address, remote_address;
    Request req;
    Response res;
    VideoStream stream;
    int estado = INIT;
    //char clientData[1000];
    string session = "123456";
    WSADATA wsa_data;
public:
    Server(){
        local_socket = remote_socket = 0;
        local_port = remote_port = 0;
        memset(&local_address, 0, sizeof(local_address));
        memset(&remote_address, 0, sizeof(remote_address));
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
    bool mylisten(int);
    bool mysend(string, string);
    bool initServer();
    bool receiveRequest();
    void processRequest();
    void myclose();
};

#endif