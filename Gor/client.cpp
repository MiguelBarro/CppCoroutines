//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <asio.hpp>
#include <asio/system_timer.hpp>

#include <asio_future_await.h>
#include <await_adapters.h>
#include <future_adapter.h>

class stats
{
public:
  stats()
    : total_bytes_written_(0)
    , total_bytes_read_(0)
  {
  }

  void add(size_t bytes_written, size_t bytes_read)
  {
    total_bytes_written_ += bytes_written;
    total_bytes_read_ += bytes_read;
  }

  void print()
  {
    std::cout << total_bytes_written_ << " total bytes written" << std::endl;
    std::cout << total_bytes_read_ << " total bytes read" << std::endl;
  }

private:
  size_t total_bytes_written_;
  size_t total_bytes_read_;
};

std::future<std::pair<size_t, size_t>>
session(asio::io_service& ios,
        asio::ip::tcp::resolver::iterator& endpoint_iterator,
        const size_t block_size,
        std::atomic_bool& stop)
{
    asio::ip::tcp::socket socket(ios);
    auto read_data = std::make_unique<char[]>(block_size);
    auto write_data = std::make_unique<char[]>(block_size);
    size_t bytes_written = 0;
    size_t bytes_read = 0;

    try
    {
        // Initialize the original client data
        for (size_t i = 0; i < block_size; ++i)
            write_data[i] = static_cast<char>(i % 128);

        // Connect to the server
        co_await async_connect(socket, endpoint_iterator);

        // Once connected loop endlessly
        while (!stop)
        {
            // Send data to the server
            bytes_written += co_await async_write(socket, asio::buffer(write_data.get(), block_size));
            // Receive data from the server
            bytes_read += co_await async_read_some(socket, asio::buffer(read_data.get(), block_size));
            // Swap the buffers
            std::swap(read_data, write_data);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // Close the socket
    socket.close();

    co_return std::pair<size_t, size_t>{bytes_written, bytes_read};
}

std::future<void>
client(asio::io_service& ios,
       asio::ip::tcp::resolver::iterator& endpoint_iterator,
       const size_t block_size,
       const size_t session_count,
       const int timeout)
{
    using session_future = std::future<std::pair<size_t, size_t>>;

    std::list<session_future> sessions;
    std::atomic_bool stop(false);
    stats stats;

    // Launch the sessions
    for (size_t i = 0; i < session_count; ++i)
    {
        sessions.push_back(session(ios, endpoint_iterator, block_size, stop));
    }

    // Wait the specified timeout
    asio::system_timer stop_timer(ios);
    co_await async_wait(stop_timer, std::chrono::seconds(timeout));

    // Stop the sessions
    stop = true;
    while (!sessions.empty())
    {
        auto times = co_await asio_future_awaiter(ios, std::move(sessions.front()));
        stats.add(times.first, times.second);
        sessions.pop_front();
    }

    // Show stats
    stats.print();
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 7)
    {
      std::cerr << "Usage: client <host> <port> <threads> <blocksize> "
                << "<sessions> <time>" << std::endl;
      return 1;
    }

    using namespace std; // For atoi.
    const char* host = argv[1];
    const char* port = argv[2];
    int thread_count = atoi(argv[3]);
    size_t block_size = atoi(argv[4]);
    size_t session_count = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    asio::io_service ios;

    asio::ip::tcp::resolver r(ios);
    asio::ip::tcp::resolver::iterator iter =
      r.resolve(asio::ip::tcp::resolver::query(host, port));

    client(ios, iter, block_size, session_count, timeout);

    std::list<std::thread*> threads;
    while (--thread_count > 0)
    {
      std::size_t (asio::io_service::* handler)() = &asio::io_service::run;
      std::thread* new_thread = new std::thread(
            std::bind(handler, &ios));
      threads.push_back(new_thread);
    }

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
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
