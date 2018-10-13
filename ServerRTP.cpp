#include <iostream>
#include "ServerRTP.hpp"
using namespace std;

vector<string> Tools::split(string s, char t){
    vector<string> v;
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

char *VideoStream::nextFrame(){
    int tam = 0;
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

bool Server::receiveRequest(){
    this->req.getHeader().resize(1000);
    // recebe a bloco do de requisição
    while(recv(remote_socket, (char *) req.getHeader().c_str(), req.getHeader().size(), 0) == SOCKET_ERROR);
    cout<<req.getHeader();
    return true;
}

void Server::processRequest(){
    auto req = Tools::split(this->req.getHeader(), '\n');
    auto line = Tools::split(req[0], ' ');
    string reqtype = line[0]; // tipo da requisição
    string filename = line[1]; //nome do arquivo
    auto seq = Tools::split(req[1], ' '); // numero da sequencia

    if(reqtype == "SETUP"){
        if(estado == INIT){
            stream = Stream(filename);
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
    else if(reqtype == TEARDOWN){
        mysend("200 OK", seq[1]);
        myclose();
    }
}

void Response::makeHeader(){
    this->header.append("RTSP/1.1 " + this->getStatus() + "\r\n");
    /*
    this->header.append("Server: " + this->getServer() +  "\r\n");
    this->header.append("Date: " + this->getDate() +  "\r\n");
    this->header.append("Content-Type: " + this->getContentType() +  "\r\n");
    this->header.append("Content-Length: " + this->getContentLength() +  "\r\n");
    this->header.append("Connection: " + this->getConnection() +  "\r\n");
    */
    this->header.append("CSeq: 2 \r\n");
    this->header.append("Public: SETUP, TEARDOWN, PLAY, PAUSE \r\n");
    this->header.append("\r\n");
}

bool Server::mysend(string status, string seq){
    string buffer = "RTSP/1.0 " + status + CRLF + "CSeq: " + seq + CRLF + "Session: " + session;
    //enviando
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
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
        //if(!this->res.Send(req.getRequestFileName(), this->remote_socket))  return Tools::ErrorS("Falha ao responder a requisicao", false, this->local_socket);
    }
    return true;
}

void Server::myclose(){
    WSACleanup();
    closesocket(this->remote_socket);
    closesocket(this->local_socket);
}