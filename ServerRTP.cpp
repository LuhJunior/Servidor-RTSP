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

int VideoStream::nextFrame(char * &data){
    int tam = 0;
    string frametam;
    frametam.resize(5);
    file.read((char *) frametam.c_str(), 5);
    //cout<<"Tamanho do frame: "<<frametam<<endl;
    if(file.eof()) return 0;
    tam = stoi(frametam);
    data = (char *) malloc(tam);
    file.read(data, tam);
    //cout<<tam<<endl;
    return tam;
}

char * Rtp::getdata(){
    char *data = (char *) malloc(payload_size + headerSize);
    memcpy(data, header.c_str(), headerSize);
    memcpy(data + headerSize, payload.c_str(), payload_size);
    return data;
}

char * Rtp::getpayloaddata(){
    char *data = (char *) malloc(payload_size);
    memcpy(data, payload.c_str(), payload_size);
    return data;
}

bool Server::sendRtp(){
    if(stream.file.eof()){
        cout<<"fim do arquivo"<<endl;
        return false;
    }
    Sleep(50);
    imagenb++;
    char *data = nullptr;
    Rtp rtp = Rtp(MJPEG_TYPE, imagenb, imagenb * FRAME_PERIOD, data, stream.nextFrame(data));
    int packlength = rtp.getlength();
    char *packdata = rtp.getdata();
    while(sendto(udp_socket, packdata, packlength, 0, 
            (LPSOCKADDR) &udp_address, 
            sizeof(struct sockaddr)) == SOCKET_ERROR) 
            cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    free(data);
    free(packdata);
    //rtp.print_header();
    return true;
}

unsigned __stdcall sendRtpThread(void *server){
    while(((Server *) server)->getEstado() == PLAYING && ((Server *) server)->sendRtp());
    _endthreadex( 0 );  
}

unsigned __stdcall listenThread(void *server){
    ((Server *) server)->mylisten(((Server *) server)->porta);
    _endthreadex( 0 );  
}

void Server::prepareUdp(int port){
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    udp_address = remote_address;
    
    udp_address.sin_port = htons(port);
}

bool Server::receiveRequest(){
    //req.getHeader().clear();
    req.getHeader().resize(2000);
    // recebe a bloco do de requisição
    int tam = recv(remote_socket, (char *) req.getHeader().c_str(), req.getHeader().size(), 0);
    if(tam == SOCKET_ERROR) return false;
    req.getHeader().resize(tam);
    req.getHeader().shrink_to_fit();
    //req.getHeader().shrink_to_fit();
    cout<<req.getHeader()<<endl;
    return true;
}

void Server::processRequest(){
    //cout<<"Processando"<<endl;
    vector<string> request = Tools::split(req.getHeader(), '\n');
    vector<string> line = Tools::split(request[0], ' ');
    string reqtype = line[0]; // tipo da requisição
    string filename = line[1];
    //string filename = Tools::split(line[1], '/')[3]; //nome do arquivo
    //cout<<filename<<endl;
    auto seq = Tools::split(request[1], ' '); // numero da sequencia
    if(reqtype == "OPTIONS"){
        sendOptions(seq[1]);
    }
    else if(reqtype == "DESCRIBE"){
        sendDescribe(seq[1], filename);
    }
    else if(reqtype == "SETUP"){
        if(estado == INIT){
            stream = VideoStream(filename);
            estado = READY;
            auto transport = Tools::split(request[2], ';');
            auto porta = Tools::split(transport[transport.size()-1], '=');
            int port = stoi(Tools::split(porta[1], '-')[0]);
            cout<<port<<endl;
            prepareUdp(port);
            mysend("200 OK", seq[1]);
            //sendSetup("200 OK", seq[1], "Transport: RTP/AVP;unicast;server_port=9700-9801;");
            cout<<"req SETUP"<<endl;
        }
    }
    else if(reqtype == "PLAY"){
        cout<<"estado PLAY"<<endl;
        if(estado == READY){
            estado = PLAYING;
            mysend("200 OK", seq[1]);
            cout<<"estado PLAYING"<<endl;
            //_beginthread( &sendRtpThread, 0, this);
            Thread = (HANDLE)_beginthreadex( nullptr, 0, &sendRtpThread, (void *) this, 0, nullptr); 
        }
        else if(estado == PAUSE){
            estado = PLAYING;
            mysend("200 OK", seq[1]);
            //_beginthread( &sendRtpThread, 0, this);
            Thread = (HANDLE)_beginthreadex( nullptr, 0, &sendRtpThread, (void *) this, 0, nullptr); 
        }
    }
    else if(reqtype == "PAUSE"){
        cout<<"estado PAUSE"<<endl;
        if(estado == PLAYING){
            CloseHandle(Thread);
            mysend("200 OK", seq[1]);
            estado = READY;
        }
    }
    else if(reqtype == "TEARDOWN"){
        estado = -1;
        mysend("200 OK", seq[1]);
        CloseHandle(Thread);
        myclose();
    }
}

bool Server::mysend(string status, string seq){
    string buffer = "RTSP/1.0 " + status + "\r\n" + "CSeq: " + seq + "\r\n" + "Session: " + session + "\r\n";
    //string buffer = "RTSP/1.0 " + status + CRLF + "CSeq: " + seq + CRLF + "Session: " + session + CRLF + CRLF;
    //enviando
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    cout<<buffer<<endl;
    return true;
}


bool Server::sendSetup(string status, string seq, string transport){
    string buffer = "";
    buffer.append("RTSP/1.0 ");
    buffer.append(status);
    buffer.append(CRLF);
    buffer.append("CSeq: ");
    buffer.append(seq);
    buffer.append(CRLF);
    buffer.append(transport);
    buffer.append(CRLF);
    buffer.append("Session: "); 
    buffer.append(session);
    buffer.append(CRLF);
    cout<<buffer<<endl;
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    return true;
}

bool Server::sendOptions(string seq){
    string buffer = "";
    buffer.append("RTSP/1.0 200 OK" + string(CRLF));
    buffer.append("CSeq: " + seq + CRLF);
    buffer.append("Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE" + string(CRLF));
    buffer.append(CRLF);
    //enviando
    while(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
    cout<<buffer<<endl;
    return true;
}


bool Server::sendDescribe(string seq, string filename){
    string buffer = "";
    string describe = "";

    describe.append("v=0" + string(CRLF));
    describe.append("m=video " + to_string(PORT) + " RTP/AVP " + to_string(MJPEG_TYPE) + string(CRLF));
    describe.append("a=control:streamid=1234" + string(CRLF));
    describe.append("a=mimetype:string;\"video/MJPEG\"" + string(CRLF));

    buffer.append("RTSP/1.0 200 OK" + string(CRLF));
    buffer.append("CSeq: " + seq + CRLF);
    buffer.append("Content-Base: rtsp://localhost:8000/video.mjpeg" + string(CRLF));
    buffer.append("Content-Type: application/sdp" + string(CRLF));
    buffer.append("Content-Length: " + to_string(describe.length()) + string(CRLF));

    cout<<buffer<<endl;
    cout<<describe<<endl;
    //enviando
    if(send(remote_socket, (char *) buffer.c_str(), buffer.size(), 0) == SOCKET_ERROR) cout<<"Erro ao enviar resposta\nTentando enviar novamente\n";
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
        cout<<"Requisição recebida"<<endl;
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