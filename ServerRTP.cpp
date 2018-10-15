#include <iostream>
#include "ServerRTP.hpp"
using namespace std;

vector<string> Tools::split(string s, char t){
    vector<string> v = vector<string>();
    while(!s.empty()){
        size_t p = s.find(t);
        if(p != -1){
            v.push_back(s.substr(0, p));
            s.erase(s.begin(), s.begin()+p+1);
        }
        else{
            v.push_back(s);
            s.clear();
        }
    }
    return v;
}

template<class T>
T Tools::Error(string msg, T r){
    cerr<<msg<<endl;
    return r;
}

template<class T>
T Tools::ErrorW(string msg, T r){
    cerr<<msg<<endl;
    WSACleanup();
    return r;
}

template<class T>
T Tools::ErrorS(string msg, T r, int s){
    cerr<<msg<<endl;
    WSACleanup();
    closesocket(s);
    return r;
}

int VideoStream::frameN(){
    return frame;
}

char *VideoStream::nextFrame(int &tam){
    tam = 0;
    string frametam;
    frametam.resize(5);
    file.read((char *) frametam.c_str(), 5);
    tam = stoi(frametam);
    char * data = (char *) malloc(tam);
    file.read(data, tam);
    return data;
}

char * Rtp::getdata(){
    char *data = (char *) malloc(payload_size + headerSize);
    strncpy(data, header.c_str(), headerSize);
    strncpy(data + headerSize, payload.c_str(), payload_size);
    return data;
}

char * Rtp::getpayloaddata(){
    char *data = (char *) malloc(payload_size);
    strncpy(data, payload.c_str(), payload_size);
    return data;
}

void Server::sendRtp(){
    imagenb++;
    int tam;
    char *data = stream.nextFrame(tam);
    Rtp rtp = Rtp(MJPEG_TYPE, imagenb, imagenb * FRAME_PERIOD, data, tam);
    
    int packlength = rtp.getlength();
    char * packdata = rtp.getdata();
    
    while(send(remote_socket, packdata, packlength, 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";

    rtp.print_header();
    free(data);
    free(packdata);
}

bool Server::receiveRequest(){
    //req.getHeader().clear();
    req.getHeader().resize(500);
    // recebe a bloco do de requisição
    while(recv(remote_socket, (char *) req.getHeader().c_str(), req.getHeader().size(), 0) == SOCKET_ERROR);
    //req.getHeader().shrink_to_fit();
    cout<<req.getHeader()<<endl;
    return true;
}

void Server::processRequest(){
    vector<string> request = Tools::split(req.getHeader(), '\n');
    vector<string> line = Tools::split(request[0], ' ');
    string reqtype = line[0]; // tipo da requisição
    string filename = Tools::split(line[1], '/')[3]; //nome do arquivo
    cout<<filename<<endl;
    auto seq = Tools::split(request[1], ' '); // numero da sequencia
    if(reqtype == "OPTIONS"){
        sendOptions(seq[1]);
    }
    else if(reqtype == "DESCRIBE"){
        sendDescribe(seq[1]);
    }
    else if(reqtype == "SETUP"){
        if(estado == INIT){
            stream = VideoStream(filename);
            estado = READY;
            mysend("200 OK", seq[1]);
            cout<<"estado SETUP";
        }
    }
    else if(reqtype == "PLAY"){
        if(estado == READY){
            estado = PLAYING;
            mysend("200 OK", seq[1]);
            cout<<"estado PLAYING";
            sendRtp();
        }
        else if(estado == PAUSE){
            estado = PLAYING;
        }
    }
    else if(reqtype == "PAUSE"){
        if(estado == PLAYING){
            estado = READY;
            mysend("200 OK", seq[1]);
        }
    }
    else if(reqtype == "TEARDOWN"){
        mysend("200 OK", seq[1]);
        myclose();
    }
}

bool Server::mysend(string status, string seq){
    string buffer = "RTSP/1.0 " + status + CRLF + "CSeq: " + seq + CRLF + "Session: " + session + CRLF + CRLF;
    //enviando
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    cout<<buffer<<endl;
    return true;
}

bool Server::sendOptions(string seq){
    string buffer = "";
    buffer.append("RTSP/1.0 200 OK" + string(CRLF));
    buffer.append("CSeq: " + seq + CRLF);
    buffer.append("Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE" + string(CRLF));
    buffer.append("\r\n");
    //enviando
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    cout<<buffer<<endl;
    return true;
}


bool Server::sendDescribe(string seq){
    string buffer = "";
    string describe = "";

    describe.append("v=0" + string(CRLF));
    describe.append("o=mhandley 2890844526 2890842807 IN IP4 126.16.64.4" + string(CRLF));
    describe.append("s=SDP Seminar" + string(CRLF));
    describe.append("i=A Seminar on the session description protocol" + string(CRLF));
    describe.append("u=http://www.cs.ucl.ac.uk/staff/M.Handley/sdp.03.ps" + string(CRLF));
    describe.append("e=mjh@isi.edu (Mark Handley)" + string(CRLF));
    describe.append("c=IN IP4 224.2.17.12/127" + string(CRLF));
    describe.append("t=2873397496 2873404696" + string(CRLF));
    describe.append("a=recvonly" + string(CRLF));
    describe.append("m=audio 3456 RTP/AVP 0" + string(CRLF));
    describe.append("m=video 2232 RTP/AVP 31" + string(CRLF));
    describe.append("m=whiteboard 32416 UDP WB" + string(CRLF));
    describe.append("a=orient:portrait" + string(CRLF));
    //describe.append(CRLF);

    buffer.append("RTSP/1.0 200 OK" + string(CRLF));
    buffer.append("CSeq: " + seq + CRLF);
    //buffer.append("Date: 23 Jan 1997 15:35:06 GMT" + string(CRLF));
    buffer.append("Content-Base: rtsp://localhost:8000/video.mjpeg" + string(CRLF));
    buffer.append("Content-Type: application/sdp" + string(CRLF));
    buffer.append("Content-Length: " + to_string(describe.size()) + string(CRLF));
    buffer.append("Server: /localhost:8000" + string(CRLF));
    buffer.append(CRLF);
    
    //enviando
    if(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    //cout<<buffer<<endl;
    if(send(remote_socket, (char *) describe.c_str(), describe.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    return true;
}

bool Server::initServer(){
    //inicio = clock();
    // inicia o Winsock 2.0 (DLL), Only for Windows
    if (WSAStartup(MAKEWORD(2, 0), &this->wsa_data) != 0) return Tools::Error("WSAStartup() failed", false);
    // criando o socket local para o servidor
    this->local_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->local_socket == INVALID_SOCKET) return Tools::ErrorW("socket() failed", false);

    this->local_address.sin_family = AF_INET; // internet address family
    this->local_address.sin_port = htons(this->local_port);// porta local
    this->local_address.sin_addr.s_addr = htonl(INADDR_ANY);  // endereco // inet_addr("127.0.0.1")

    // interligando o socket com o endereço (local)
    if (bind(local_socket, (struct sockaddr *) &local_address, sizeof(local_address)) == SOCKET_ERROR)
        return Tools::ErrorS("bind() failed", false, this->local_socket);

    return true;
}

bool Server::mylisten(int port){
    int remote_length = 0;
    this->local_port = port;
    this->initServer();
    // coloca o socket para escutar as conexoes
    if (listen(this->local_socket, BACKLOG_MAX) == SOCKET_ERROR) return Tools::ErrorS("listen() failed", false, this->local_socket);
    
    remote_length = sizeof(remote_address);
    cout<<"Aguardando Conexao..."<<endl;
    this->remote_socket = accept(local_socket, (struct sockaddr *) &remote_address, &remote_length);
    if(this->remote_socket == INVALID_SOCKET) return Tools::ErrorS("accept() failed", false, this->local_socket);
    cout<<"Conexao estabelecida com "<< inet_ntoa(remote_address.sin_addr)<<endl;
    cout<<"Aguardando requisicoes..."<<endl;
    while(true){
        if(!this->receiveRequest()) return Tools::ErrorS("Falha ao receber a requisicao", false, this->local_socket);
        processRequest();
        cout<<"Processada"<<endl;
    }
    return true;
}

void Server::myclose(){
    WSACleanup();
    stream.file.close();
    closesocket(this->remote_socket);
    closesocket(this->local_socket);
}