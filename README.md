# easy_dbus
It is a dbus wrapper library. simplified interface to dbus remote method calling and signal functionality.

System requirement for the library. (x86 ubuntu 20.04 tested)
=============================================================
1. dbus packages
2. libevent packages
3. glib library
4. build-essentials library

```console
sudo apt-get install -y libdbus-glib-1-dev libdbus-1-dev build-essential libevent-dev
```

you may have to install additional packages based on your system configuration but this should work for the most of the cases

library directory structure
===========================
```console
├── examples
│ ├── app1
│ │   └── app1.c
│ └── app2
│     └── app2.c
├── include
│  ├── dbus-client.h
│  └── logmodule.h
├── Makefile
├── README.md
└── src
    ├── dbus-client.c
    └── logmodule.c
```

src directory contains main dbus-client source and a logmodule library. log module directory is used to generate log files and it is a generic library that i used with my other projects. it stores logs with time stamp and it is useful while debugging  complex project.

the include directory contains the headers for dbus-client and logmodule libraries. these headers must be included in the source files in order to use this library.

examples directory contain two application source code that utilize the dbus-client library to talk each other. The app1 has a method which can be invoked from app2 and will return some data.also it emit signal with data continuously in every 10s. The app2 is subscribed to the signal emited by the app1 and it execute a callback function for every app1 signal, also app2 invoke app1 method every 2s.

Building
========
i have included a Makefile to build dbus-client library, logmodule library, app1 and app2 

execute the following code to build above target.

```console
cd easy_dbus
make
```

and this is the expected output

```console
$ make
mkdir -p build build/app1 build/app2
cc -shared -o build/liblogmodule.so src/logmodule.c -I ./include -fPIC -s
cc -shared -o build/libdbus-client.so src/dbus-client.c -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -ldbus-1 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -ldbus-glib-1 -ldbus-1 -lgobject-2.0 -lglib-2.0 -I ./include -llogmodule -Lbuild -levent -lpthread -fPIC -s
cc examples/app1/app1.c -o build/app1/app1 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -ldbus-1 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -ldbus-glib-1 -ldbus-1 -lgobject-2.0 -lglib-2.0 -I ./include -ldbus-client -levent -Lbuild -llogmodule -Lbuild -s
cc examples/app2/app2.c -o build/app2/app2  -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -ldbus-1 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -ldbus-glib-1 -ldbus-1 -lgobject-2.0 -lglib-2.0 -I ./include -ldbus-client -levent -Lbuild -llogmodule -Lbuild -s
```

Run example application
=======================
To run app1 goto a terminal and execute the following command

```console
cd easy_dbus
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/build
cd build/app1
./app1
```

Now open the another terminal and run the following code

```console
cd easy_dbus
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/build
cd build/app2
./app2
```

expected output
![dbus apps output](https://electricchant.files.wordpress.com/2022/10/image-1.png?w=616)

note: i have tested library to a some extend, fixed bugs that i found while testing. The library is opened for any extended testing and detailed profiling. so feel free to drop your comments/suggestions here in the comment section or at my git.