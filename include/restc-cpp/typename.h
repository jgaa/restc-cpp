
#include "restc-cpp/config.h"


#ifdef RESTC_CPP_HAVE_BOOST_TYPEINDEX
    #include <boost/type_index.hpp>

    #define RESTC_CPP_TYPENAME(type) \
        boost::typeindex::type_id<type>().pretty_name()
#else
    #define RESTC_CPP_TYPENAME(type) \
        typeid(type).name()
#endif

