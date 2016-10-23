This directory is for creating and running a docker container
with mock json data for testing.

We use this [docker-json-server](https://github.com/clue/docker-json-server) as
a base, and eventually run [json-server](https://github.com/typicode/json-server) 
in a container with data pre-geretared with [json-server-init](https://github.com/dfsq/json-server-init).

To generate the container run:
'''sh
$ docker build . -t jgaa/mockjsonserver
'''

To run the container:
'''sh
docker run --name mock-json  -v `pwd`/db.json:/data/db.json -d -p 3000:80 jgaa/mockjsonserver
'''

Or just run:
'''sh
./runmockserver
```

