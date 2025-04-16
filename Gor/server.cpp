//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <thread>

#include <asio.hpp>

#include <await_adapters.h>
#include <future_adapter.h>

std::future<void>
session(asio::ip::tcp::socket socket,
        const size_t block_size)
{
    char* read_data = new char[block_size];
    char* write_data = new char[block_size];

    try
    {
        // Initialization
        asio::error_code set_option_err;
        asio::ip::tcp::no_delay no_delay(true);
        socket.set_option(no_delay, set_option_err);
        if (set_option_err)
            throw std::runtime_error("Failed to set socket option");

        // loop endlessly
        for (;;)
        {
            // Receive data from the server
            co_await async_read_some(socket, asio::buffer(read_data, block_size));
            // Swap the buffers
            std::swap(read_data, write_data);
            // Send data to the server
            co_await async_write(socket, asio::buffer(write_data, block_size));
        }
    }
    catch (asio::system_error& e)
    {
        if (e.code() != asio::error::eof)
            std::cerr << "System error: " << e.what() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // Close the socket
    socket.close();

    // tidy up
    delete[] read_data;
    delete[] write_data;
}

std::future<void>
server(asio::io_service& ios,
       asio::ip::tcp::endpoint endpoint,
       const size_t block_size)
{
    asio::ip::tcp::acceptor acceptor(ios);

    acceptor.open(endpoint.protocol());
    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(1));
    acceptor.bind(endpoint);
    acceptor.listen();

    // loop accepting connections
    for (;;)
    {
        // Create a new socket
        asio::ip::tcp::socket socket(ios);
        // Accept a connection
        co_await async_accept(acceptor, socket);
        // Start the session
        session(std::move(socket), block_size);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 5)
        {
            std::cerr << "Usage: server <address> <port> <threads> <blocksize>" << std::endl;
            return 1;
        }

        using namespace std; // For atoi.
        asio::ip::address address = asio::ip::address::from_string(argv[1]);
        short port = static_cast<short>(atoi(argv[2]));
        int thread_count = atoi(argv[3]);
        size_t block_size = atoi(argv[4]);

        asio::io_service ios;

        server(ios, asio::ip::tcp::endpoint(address, port), block_size);

        // Threads not currently supported in this test.
        std::list<std::thread*> threads;
        while (--thread_count > 0)
        {
            std::size_t (asio::io_service::* handler)() = &asio::io_service::run;
            std::thread* new_thread = new std::thread(std::bind(handler, &ios));
            threads.push_back(new_thread);
        }

        // Handle user signals for loop interruption
        asio::signal_set signals(ios, SIGINT, SIGTERM);
        signals.async_wait([&ios](const std::error_code& error, int signal_number)
            {
                if (error || signal_number == SIGINT || signal_number == SIGTERM)
                    ios.stop();
            });

        // loop until interrupted
        ios.run();

        while (!threads.empty())
        {
            threads.front()->join();
            delete threads.front();
            threads.pop_front();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
