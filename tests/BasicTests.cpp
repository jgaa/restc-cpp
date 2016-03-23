
#include <iostream>

#include "restc-cpp/restc-cpp.h"

using namespace std;
using namespace restc_cpp;


const string http_url = "http://jsonplaceholder.typicode.com/posts";

void Process(Context& ctx) {

    try {
        auto repl = ctx.Get(http_url);
        
    } catch (const exception& ex) {
        std::clog << "Process: Caught exception: " << ex.what() << endl;
    }
}


int main(int argc, char *argv[]) {

    try {
        auto rest_client = RestClient::Create();
        rest_client->Process(Process);
    } catch (const exception& ex) {
        std::clog << "main: Caught exception: " << ex.what() << endl;
    }


    return 0;
}
