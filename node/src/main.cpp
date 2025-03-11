#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;





void load_and_thread(){
    std::string shared_obj_path = "./libexample.so";
    std::string kernel_name = "hello";

    void *handle = dlopen(shared_obj_path.c_str(), RTLD_LAZY);
    
    void *(*task)(void *) = (void *(*)(void *)) dlsym(handle, kernel_name.c_str());

    void *args;

    pthread_t t_id;
    pthread_create(&t_id, NULL, task, args);
}

#define LISTEN_PORT 4114
int main(){
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), LISTEN_PORT));

    tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::cout << "Client connected!\n";

    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        boost::system::error_code error;
        size_t length = socket.read_some(boost::asio::buffer(buffer), error);

        if (error == boost::asio::error::eof) {
            std::cout << "Client disconnected.\n";
            break;
        } else if (error) {
            throw boost::system::system_error(error);
        }

        std::cout << "Client: " << buffer << std::endl;
        boost::asio::write(socket, boost::asio::buffer(buffer, length));
    }

    return 0;
}