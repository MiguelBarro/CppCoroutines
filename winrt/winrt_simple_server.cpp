// cl /Zi /EHsc /nologo /std:c++20 winrt_simple_server.cpp /link windowsapp.lib
// set makeprg=cl\ /Zi\ /EHsc\ /nologo\ /std:c++20\ %\ /link\ windowsapp.lib

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <list>

#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Networking.Sockets.h"
#include "winrt/Windows.Storage.Streams.h"

namespace
{
    winrt::handle signal_event{WINRT_IMPL_CreateEventW(nullptr, true, false, nullptr)};

    void signal_handler(int /*signal*/)
    {
        WINRT_IMPL_SetEvent(signal_event.get());
    }
}

winrt::Windows::Foundation::IAsyncAction
session(winrt::Windows::Networking::Sockets::StreamSocket socket,
        const uint32_t block_size)
{
    using namespace std;
    using namespace winrt;

    Windows::Storage::Streams::Buffer spam(block_size); // Capacity
    // Set Length
    spam.Length(block_size);

    try
    {
        // Initialization
        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();

        // loop endlessly
        while (!cancel())
        {
            // Receive data from the client
            auto read_data = co_await socket.InputStream().ReadAsync(
                    spam,
                    spam.Capacity(),
                    Windows::Storage::Streams::InputStreamOptions::None);

            if (read_data.Length() == 0)
            {
                wcout << "Client closed the connection." << endl;
                throw winrt::hresult_canceled();
            }

            // Send data back to the client
            co_await socket.OutputStream().WriteAsync(read_data);
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
}

int wmain(int argc, wchar_t* argv[])
{
    using namespace std;
    using namespace winrt;

    try
    {
        init_apartment(apartment_type::multi_threaded);

        if (argc != 3)
        {
            wcerr << L"Usage: winrt_server <port> <blocksize>" << endl;
            return 1;
        }

        wstring_view service_name = argv[1];
        uint32_t block_size = stoi(argv[2]);

        // Create the tcp listener
        Windows::Networking::Sockets::StreamSocketListener listener;

        // Set up
        listener.Control().QualityOfService(
            Windows::Networking::Sockets::SocketQualityOfService::LowLatency);

        list<Windows::Foundation::IAsyncAction> sessions;

        auto revoker = listener.ConnectionReceived(
            auto_revoke,
            [block_size, &sessions](
                auto&&,
                Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs const& args)
            {
                sessions.push_back(session(args.Socket(), block_size));
            });

        listener.BindServiceNameAsync(service_name);

        // Wait for termination signal
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        WINRT_IMPL_WaitForSingleObject(signal_event.get(), 0xFFFFFFFF /*INFINITE*/);

        // Clear sessions
        for (auto& s : sessions)
        {
            // cancel returns at once
            s.Cancel();
            // Windows::Foundation::TimeSpan
            //  == std::chrono::duration<int64_t, std::ratio_multiply<std::ratio<100>, std::nano>>
            s.wait_for(std::chrono::minutes{1});
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

    uninit_apartment();

    return 0;
}
// vim: tabstop=8 softtabstop=4 smarttab shiftwidth=4 expandtab
