//
// classic_client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <string>
#include <thread>

#include <asio.hpp>
#include <asio/system_timer.hpp>

namespace asio::placeholders
{
    constexpr auto error = std::placeholders::_1;
    constexpr auto bytes_transferred = std::placeholders::_2;
}

#include <handler_allocator.h>

class stats
{
public:
  stats()
    : mutex_(),
      total_bytes_written_(0),
      total_bytes_read_(0)
  {
  }

  void add(size_t bytes_written, size_t bytes_read)
  {
    asio::detail::mutex::scoped_lock lock(mutex_);
    total_bytes_written_ += bytes_written;
    total_bytes_read_ += bytes_read;
  }

  void print()
  {
    asio::detail::mutex::scoped_lock lock(mutex_);
    std::cout << total_bytes_written_ << " total bytes written\n";
    std::cout << total_bytes_read_ << " total bytes read\n";
  }

private:
  asio::detail::mutex mutex_;
  size_t total_bytes_written_;
  size_t total_bytes_read_;
};

class session
{
public:
  session(asio::io_service& ios, size_t block_size, stats& s)
    : strand_(ios),
      socket_(ios),
      block_size_(block_size),
      read_data_(new char[block_size]),
      read_data_length_(0),
      write_data_(new char[block_size]),
      unwritten_count_(0),
      bytes_written_(0),
      bytes_read_(0),
      stats_(s)
  {
    for (size_t i = 0; i < block_size_; ++i)
      write_data_[i] = static_cast<char>(i % 128);
  }

  ~session()
  {
    stats_.add(bytes_written_, bytes_read_);

    delete[] read_data_;
    delete[] write_data_;
  }

  void start(asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    asio::async_connect(socket_, endpoint_iterator,
        strand_.wrap(std::bind(&session::handle_connect, this,
            asio::placeholders::error)));
  }

  void stop()
  {
    strand_.post(std::bind(&session::close_socket, this));
  }

private:
  void handle_connect(const asio::error_code& err)
  {
    if (!err)
    {
      asio::error_code set_option_err;
      asio::ip::tcp::no_delay no_delay(true);
      socket_.set_option(no_delay, set_option_err);
      if (!set_option_err)
      {
        ++unwritten_count_;
        async_write(socket_, asio::buffer(write_data_, block_size_),
            strand_.wrap(
              make_custom_alloc_handler(write_allocator_,
                  std::bind(&session::handle_write, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
        socket_.async_read_some(asio::buffer(read_data_, block_size_),
            strand_.wrap(
              make_custom_alloc_handler(read_allocator_,
                  std::bind(&session::handle_read, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
      }
    }
  }

  void handle_read(const asio::error_code& err, size_t length)
  {
    if (!err)
    {
      bytes_read_ += length;

      read_data_length_ = length;
      ++unwritten_count_;
      if (unwritten_count_ == 1)
      {
        std::swap(read_data_, write_data_);
        async_write(socket_, asio::buffer(write_data_, read_data_length_),
            strand_.wrap(
              make_custom_alloc_handler(write_allocator_,
                  std::bind(&session::handle_write, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
        socket_.async_read_some(asio::buffer(read_data_, block_size_),
            strand_.wrap(
              make_custom_alloc_handler(read_allocator_,
                  std::bind(&session::handle_read, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
      }
    }
  }

  void handle_write(const asio::error_code& err, size_t length)
  {
    if (!err && length > 0)
    {
      bytes_written_ += length;

      --unwritten_count_;
      if (unwritten_count_ == 1)
      {
        std::swap(read_data_, write_data_);
        async_write(socket_, asio::buffer(write_data_, read_data_length_),
            strand_.wrap(
              make_custom_alloc_handler(write_allocator_,
                  std::bind(&session::handle_write, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
        socket_.async_read_some(asio::buffer(read_data_, block_size_),
            strand_.wrap(
              make_custom_alloc_handler(read_allocator_,
                  std::bind(&session::handle_read, this,
                  asio::placeholders::error,
                  asio::placeholders::bytes_transferred))));
      }
    }
  }

  void close_socket()
  {
    socket_.close();
  }

private:
  asio::io_service::strand strand_;
  asio::ip::tcp::socket socket_;
  size_t block_size_;
  char* read_data_;
  size_t read_data_length_;
  char* write_data_;
  int unwritten_count_;
  size_t bytes_written_;
  size_t bytes_read_;
  stats& stats_;
  handler_allocator read_allocator_;
  handler_allocator write_allocator_;
};

class client
{
public:
  client(asio::io_service& ios,
      const asio::ip::tcp::resolver::iterator endpoint_iterator,
      size_t block_size, size_t session_count, int timeout)
    : io_service_(ios),
      stop_timer_(ios),
      sessions_(),
      stats_()
  {
    stop_timer_.expires_from_now(std::chrono::seconds(timeout));
    stop_timer_.async_wait(std::bind(&client::handle_timeout, this));

    for (size_t i = 0; i < session_count; ++i)
    {
      session* new_session = new session(io_service_, block_size, stats_);
      new_session->start(endpoint_iterator);
      sessions_.push_back(new_session);
    }
  }

  ~client()
  {
    while (!sessions_.empty())
    {
      delete sessions_.front();
      sessions_.pop_front();
    }

    stats_.print();
  }

  void handle_timeout()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
          std::mem_fn(&session::stop));
  }

private:
  asio::io_service& io_service_;
  asio::system_timer stop_timer_;
  std::list<session*> sessions_;
  stats stats_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 7)
    {
      std::cerr << "Usage: client <host> <port> <threads> <blocksize> ";
      std::cerr << "<sessions> <time>\n";
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

    client c(ios, iter, block_size, session_count, timeout);

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
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
