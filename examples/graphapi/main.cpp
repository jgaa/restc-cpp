/* Program to log external IP-changes on NAT networks
 *
 * I have an ISP from Hell, and one problem I have noticed recently is that
 * my external IP address change a lot. To monitor this problem, I wrote this little
 * example program who checks the external IP every 5 minutes, and log changes.
 *
 * The program can be run in the background:
 *
 *      logip >> /var/tmp/ip.log 2>&1 &
 */


#include <ctime>
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"

using namespace restc_cpp;
using namespace std;
using namespace std::string_literals;


int main(int argc, char *argv[]) {
    RESTC_CPP_TEST_LOGGING_SETUP("info");

    // Prevent restc from throwing on HTTP errors
    Request::Properties properties;
    properties.throwOnHttpError = false;

    auto client = RestClient::Create(properties);
    auto res = client->ProcessWithPromise([&](Context& ctx) {

        const auto url = "https://graph.facebook.com/v15.0/me"s;

        cout << "Calling: " << url << endl;

        auto reply = RequestBuilder(ctx)
                .Get(url)
                .Argument("access_token", "invalid")
                // Add some headers for good taste
                .Header("content-type","application/json; charset=UTF-8")
                .Header("User-Agent", "RESTC-CPP")
                .Header("Accept", "*/*")
                // Send the request
                .Execute();

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = reply->GetBodyAsString();

        // Just dump the data.
        cout << "HTTP response : " << reply->GetHttpResponse().status_code
            << ' ' << reply->GetHttpResponse().reason_phrase << endl;
        cout << "Received data: " << json << endl;
    });

    try {
        res.get();
    } catch(const exception& ex) {
        RESTC_CPP_LOG_ERROR_("Caught exception from coroutine: " << ex.what());
        return 1;
    }

    return 0;
}
