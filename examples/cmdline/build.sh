g++ -DBOOST_COROUTINE_NO_DEPRECATION_WARNING main.cpp \
	-lrestc-cpp -lz -lssl -lcrypto -lpthread \
	-lboost_system -lboost_program_options \
	-lboost_filesystem -lboost_date_time \
	-lboost_context -lboost_coroutine \
	-lboost_chrono -lboost_log -lboost_thread \
	-lboost_log_setup -lboost_regex \
	-lboost_atomic -lpthread
