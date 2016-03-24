
#include <iostream>

#include "restc-cpp/restc-cpp.h"

using namespace std;
using namespace restc_cpp;

void SleepUntilDoomdsay()
{
    boost::asio::io_service io_service;

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM
    #ifdef SIGQUIT
    ,SIGQUIT
    #endif
    );
    signals.async_wait([](boost::system::error_code /*ec*/, int signo) {

        std::clog << "Reiceived signal " << signo << ". Shutting down" << endl;
    });

    std::clog << "Main thread going to sleep - waiting for shtudown signal" << endl;
    io_service.run();
    std::clog << "Main thread is awake" << endl;
}


const string http_url = "http://jsonplaceholder.typicode.com/posts";
const string https_url = "https://jsonplaceholder.typicode.com/posts";

void DoSomethingInteresting(Context& ctx) {

    try {

        // Asynchronously connect to server and fetch data.
        auto repl = ctx.Get(http_url);

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = repl->GetBodyAsString();

        // Just dump the data.
        clog << "Received GET data: " << json << endl;

        // Asynchronously connect to server and POST data.
        repl = ctx.Post(http_url, "{ 'test' : 'teste' }");

        // Asynchronously fetch the entire data-set and return it as a string.
        json = repl->GetBodyAsString();
        clog << "Received POST data: " << json << endl;

        // Try with https
        repl = ctx.Get(https_url);
        json = repl->GetBodyAsString();
        clog << "Received https GET data: " << json << endl;

    } catch (const exception& ex) {
        std::clog << "Process: Caught exception: " << ex.what() << endl;
    }
}


int main(int argc, char *argv[]) {

    try {
        auto rest_client = RestClient::Create();
        rest_client->Process(DoSomethingInteresting);
        SleepUntilDoomdsay();
        // TODO: Shut down the client and wait for the worker thread to exit.
    } catch (const exception& ex) {
        std::clog << "main: Caught exception: " << ex.what() << endl;
    }

    return 0;
}
