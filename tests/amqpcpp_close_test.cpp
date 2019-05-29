//
// Created by Mario Bielert on 22.11.18.
//

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <chrono>
#include <iostream>

int main()
{
    boost::asio::io_service io_service;

    AMQP::LibBoostAsioHandler handler(io_service);
    AMQP::TcpConnection connection(&handler, AMQP::Address("amqp://localhost"));

    boost::asio::system_timer timer(io_service);
    timer.expires_from_now(std::chrono::seconds(10));
    timer.async_wait([&connection](auto error) {
        std::cerr << "Calling AMQP::TcpConnection::close()\n";
        connection.close();
        // after this lambda is executed, all async operations trigger by the main() code itself
        // will be removed from the io_service. So only AMQP could have operations left which delay
        // the shutdown.
    });

    std::cerr << "Start event loop and connection...\n";
    io_service.run();
}
