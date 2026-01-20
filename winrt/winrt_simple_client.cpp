// cl /Zi /EHsc /nologo /std:c++20 winrt_simple_client.cpp /link windowsapp.lib
// set makeprg=cl\ /Zi\ /EHsc\ /nologo\ /std:c++20\ %\ /link\ windowsapp.lib

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <list>
#include <utility>
#include <vector>

#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Networking.Sockets.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Foundation.Collections.h"

class stats
{
    size_t total_bytes_written_;
    size_t total_bytes_read_;

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
};

winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<uint32_t>>
session(winrt::Windows::Networking::EndpointPair const& endpointPair,
        const uint32_t block_size,
        const std::atomic_bool& stop)
{
    using namespace std;
    using namespace winrt;

    Windows::Networking::Sockets::StreamSocket socket;
    Windows::Storage::Streams::Buffer buffer(block_size); // Capacity
    uint32_t bytes_written = 0,
             bytes_read = 0;

    try
    {
        // Initialization
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        // Initialize the original client data
        {
            buffer.Length(block_size); // Set Length
            auto data = buffer.data();
            for (size_t i = 0; i < block_size; ++i)
                data[i] = static_cast<uint8_t>((i % 57) + 65);
        }

        // Connect to the server
        co_await socket.ConnectAsync(endpointPair);

        // Once connected loop endlessly
        while (!stop && !cancel())
        {
            // Send data to the server
            bytes_written += co_await socket.OutputStream().WriteAsync(buffer);

            // Receive data from the server
            auto read_data = co_await socket.InputStream().ReadAsync(
                    buffer,
                    buffer.Capacity(),
                    Windows::Storage::Streams::InputStreamOptions::None);
            bytes_read += read_data.Length();

            // Swap the buffers, it relies on base.h:
            // friend void swap(IUnknown& left, IUnknown& right) noexcept
            swap(read_data, buffer);
        }
    }
    catch (winrt::hresult_error e)
    {
        wcerr << L"Error with HRESULT: " << e.code() << L" meaning: " << e.message().c_str() << endl;
    }
    catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
    }

    // Close the socket
    socket.Close();

    std::vector<uint32_t> measures;
    measures.push_back(bytes_written);
    measures.push_back(bytes_read);

    // Wrap as a WinRT IVector
    co_return single_threaded_vector<uint32_t>(std::move(measures));
}

winrt::Windows::Foundation::IAsyncAction
client(winrt::Windows::Networking::EndpointPair const& endpoints,
       const uint32_t block_size,
       const size_t session_count,
       const std::chrono::seconds timeout)
{
    using namespace std;
    using namespace winrt;

    // Launch sessions
    stats stats;
    atomic_bool stop(false);
    list<Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<uint32_t>>> sessions;
    for (size_t i = 0; i < session_count; ++i)
        sessions.push_back(session(endpoints, block_size, stop));

    // Wait the specified timeout
    co_await timeout;

    // Stop the sessions
    stop = true;
    while (!sessions.empty())
    {
        // Wait for session end
        auto& s = sessions.front();
        auto times = co_await s;
        s.Close();
        stats.add(times.GetAt(0), times.GetAt(1));
        sessions.pop_front();
    }

    // Show stats
    stats.print();
}

int wmain(int argc, wchar_t* argv[])
{
    using namespace std;
    using namespace winrt;

    try
    {
        init_apartment(apartment_type::multi_threaded);

        if (argc != 6)
        {
            wcerr << L"Usage: winrt_client <host> <port> <blocksize> "
                << L"<sessions> <time>" << endl;
            return 1;
        }

        // Parse arguments
        uint32_t block_size = stoi(argv[3]);
        size_t session_count = stoi(argv[4]);
        auto timeout = std::chrono::seconds(stoi(argv[5]));

        // Create the endpoint pair
        Windows::Networking::EndpointPair endpoints(
            nullptr,                                // localHostName
            L"",                                    // localServiceName
            Windows::Networking::HostName{argv[1]}, // remoteHostName
            argv[2]                                 // remoteServiceName
        );

        // Wait for the client to complete
        client(endpoints, block_size, session_count, timeout).get();

    }
    catch (winrt::hresult_error e)
    {
        wcerr << L"Error with HRESULT: " << e.code() << L" meaning: " << e.message().c_str() << endl;
    }
    catch (std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
    }

    uninit_apartment();

    return 0;
}
// vim: tabstop=8 softtabstop=4 smarttab shiftwidth=4 expandtab
