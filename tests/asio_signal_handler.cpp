//
// Created by Mario Bielert on 20.11.18.
//

#include <asio/basic_waitable_timer.hpp>
#include <asio/io_service.hpp>
#include <asio/signal_set.hpp>

#include <iostream>

int main()
{
    asio::io_service io_service;

    asio::signal_set signals(io_service, SIGINT, SIGTERM);

    signals.async_wait([](auto, auto signal) {
        if (!signal)
        {
            return;
        }
        std::cerr << "Caught signal " << signal << ". Shutdown." << std::endl;
    });

    io_service.run();
}