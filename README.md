# restc-cpp

This is a minimalistic but still functioning and very fast
(at least, that is the intention) HTTP/HTTPS client library in C++.

It depends on C++14 with its standard libraries and boost.
It uses boost::asio for IO.

The library is written by Jarle (jgaa) Aase, an enthusiastic
C++ software developer since 1996. (Before that, I used C).

## Design Goal
The design goal of this project is to make external REST API's
simple to use in C++ projects, but still very fast (which is why
we use C++ in the first place, right?). Since it uses boost::asio,
it's a perfect match for client libraries and applications,
but also modern, powerful C++ servers, since these more and more
defaults to boost:asio for network IO.

Usually I use some version of GPL or LGPL for my projects. This
library however is so tiny and general that I have released it
under the more permissive MIT license.

## Current Status
The project is justs starting up. The code is incomplete.

## Supported development platforms:
- Linux (Debian stable and testing)
- Windows 10 (Latest "community" C++ compiler from Microsoft

## Suggested target platforms:
- Linux
- OS/X
- Android (via NDK)
- Windows Vista and later
- Windows mobile
