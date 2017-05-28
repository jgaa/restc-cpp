
#include <ctime>
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"

using namespace restc_cpp;
using namespace std;

struct Data {
    string ip;
};

BOOST_FUSION_ADAPT_STRUCT(
    Data,
    (string, ip)
)

int main(int argc, char *argv[]) {

    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );

    string url = "https://api.ipify.org";

    auto client = RestClient::Create();
    client->Process([&](Context& ctx) {

        string current_ip;
        Data data;
        char date[32] = {};

        while(true) {
            SerializeFromJson(data, RequestBuilder(ctx)
                .Get(url)
                .Argument("format", "json")
                .Header("X-Client", "RESTC_CPP")
                .Execute());

            if (current_ip != data.ip) {
                auto now = time(NULL);
                strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&now));

                clog << date
                    << ' '
                    << data.ip
                    << endl;
                current_ip = data.ip;
            }

            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    });


    return 0;
}
