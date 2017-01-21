# Running the tests

In order to run the test, you must have Docker installed.
The docker containers will use localhost ports 3000, 3001, 3002, 3003. 

Create and start the Docker containers.

This will create three containers, one with a generic JSON api server,
one with nginx, configured to test authentication and redirects, and
one with squid to test proxy traversal.

Note that some of the tests use the containers, others use real
API mock services on the Internet.

```sh
create-and-run-containers.sh
```

Run the tests.

```sh
./tests/run-tests.sh
```

